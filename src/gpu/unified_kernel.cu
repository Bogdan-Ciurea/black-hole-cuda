#include "gpu/unified_kernel.h"
#include "gpu/cuda_math.cuh"

#include <cmath>
#include <cstdio>

namespace bht::gpu {

// Constant memory for scene objects (broadcast to all threads)
__constant__ DeviceSceneObject c_objects[MAX_SCENE_OBJECTS];
__constant__ int c_objectCount;

// Generate ray direction for pixel (x, y)
__device__ float3 unifiedGenerateRay(int x, int y, int width, int height,
                                     const DeviceCamera& cam) {
  float aspectRatio = (float)width / (float)height;
  float tanHalfFov = tanf(cam.fov * 3.14159265f / 360.0f);

  float ndcX = (2.0f * (x + 0.5f) / width - 1.0f) * aspectRatio * tanHalfFov;
  float ndcY = (1.0f - 2.0f * (y + 0.5f) / height) * tanHalfFov;

  float3 dir = cam.forward + ndcX * cam.right + ndcY * cam.up;
  return normalize3(dir);
}

// Segment-sphere intersection test.
// Tests whether the line segment from p0 to p1 intersects a sphere.
// Returns true if intersection found, sets tHit to parametric distance [0,1].
__device__ bool segmentSphereIntersect(float3 p0, float3 p1,
                                       float3 center, float radius,
                                       float& tHit) {
  float3 d = p1 - p0;
  float3 oc = p0 - center;

  float a = dot3(d, d);
  float b = 2.0f * dot3(oc, d);
  float c = dot3(oc, oc) - radius * radius;

  float disc = b * b - 4.0f * a * c;
  if (disc < 0.0f) return false;

  float sqrtDisc = sqrtf(disc);
  float inv2a = 0.5f / a;

  float t0 = (-b - sqrtDisc) * inv2a;
  float t1 = (-b + sqrtDisc) * inv2a;

  // Find earliest intersection within segment [0, 1]
  if (t0 >= 0.0f && t0 <= 1.0f) { tHit = t0; return true; }
  if (t1 >= 0.0f && t1 <= 1.0f) { tHit = t1; return true; }

  // Ray starts inside sphere (t0 < 0 and t1 > 0)
  if (t0 < 0.0f && t1 > 0.0f) { tHit = 0.0f; return true; }

  return false;
}

// Hash-based 2D noise for disk turbulence (2 octaves).
// Returns value in [-1, 1].
__device__ float diskNoise(float x, float z, float time) {
  // Animate: slowly shift noise pattern
  x += time * 0.15f;
  z += time * 0.1f;

  auto hash2d = [](float px, float pz) -> float {
    int ix = __float2int_rd(px);
    int iz = __float2int_rd(pz);
    unsigned int h = (unsigned int)(ix * 374761393 + iz * 668265263 + 1013904223);
    h = (h ^ (h >> 13)) * 1274126177u;
    return (float)(h & 0x7fffff) / (float)0x7fffff; // [0, 1]
  };

  auto smoothNoise = [&](float px, float pz) -> float {
    float fx = px - floorf(px);
    float fz = pz - floorf(pz);
    // Smoothstep interpolation
    fx = fx * fx * (3.0f - 2.0f * fx);
    fz = fz * fz * (3.0f - 2.0f * fz);
    float a = hash2d(px, pz);
    float b = hash2d(px + 1.0f, pz);
    float c = hash2d(px, pz + 1.0f);
    float d = hash2d(px + 1.0f, pz + 1.0f);
    return a * (1 - fx) * (1 - fz) + b * fx * (1 - fz) +
           c * (1 - fx) * fz + d * fx * fz;
  };

  // 2 octaves
  float n = smoothNoise(x * 1.5f, z * 1.5f) * 0.65f;
  n += smoothNoise(x * 3.5f, z * 3.5f) * 0.35f;
  return n * 2.0f - 1.0f; // remap to [-1, 1]
}

// Segment-disk intersection with optional thickness slab.
// When halfThickness > 0, tests against a band [center.y - h, center.y + h]
// where h varies with radius (thinner at inner edge).
__device__ bool segmentDiskIntersect(float3 p0, float3 p1,
                                     float3 center, float innerR, float outerR,
                                     float halfThickness, float time,
                                     int turbulenceOn,
                                     float& tHit) {
  float dy = p1.y - p0.y;
  if (fabsf(dy) < 1e-10f) return false;

  float t = (center.y - p0.y) / dy;
  if (t < 0.0f || t > 1.0f) return false;

  float hitX = p0.x + t * (p1.x - p0.x) - center.x;
  float hitZ = p0.z + t * (p1.z - p0.z) - center.z;
  float r2 = hitX * hitX + hitZ * hitZ;

  if (r2 < innerR * innerR || r2 > outerR * outerR) return false;

  if (halfThickness > 0.0f && turbulenceOn) {
    // Thickness scales with radius: thin at inner edge, thick at outer
    float r = sqrtf(r2);
    float tRadial = (r - innerR) / (outerR - innerR);
    float localH = halfThickness * (0.3f + 0.7f * tRadial);

    // Noise-based vertical displacement
    float disp = diskNoise(hitX * 0.4f, hitZ * 0.4f, time) * localH * 0.6f;
    float surfaceY = center.y + disp;

    // Check if the ray crossing is within the displaced surface ± tolerance
    float hitY = p0.y + t * dy;
    if (fabsf(hitY - surfaceY) > localH) return false;
  }

  tHit = t;
  return true;
}

// Compute disk color with radial gradient, Keplerian rotation, Doppler beaming,
// and optional turbulence texture.
__device__ float3 diskColor(float3 hitPos, const DeviceSceneObject& obj,
                            float time, float3 camPos, int turbulenceOn) {
  float dx = hitPos.x - obj.position.x;
  float dz = hitPos.z - obj.position.z;
  float r = sqrtf(dx * dx + dz * dz);
  float t = (r - obj.diskInnerRadius) / (obj.diskOuterRadius - obj.diskInnerRadius);
  t = fminf(fmaxf(t, 0.0f), 1.0f);

  // Base radial gradient (inner=hot, outer=cool)
  float3 baseColor = {
    obj.diskInnerColor.x * (1.0f - t) + obj.diskOuterColor.x * t,
    obj.diskInnerColor.y * (1.0f - t) + obj.diskOuterColor.y * t,
    obj.diskInnerColor.z * (1.0f - t) + obj.diskOuterColor.z * t
  };

  // Keplerian differential rotation: omega = sqrt(M / r³), M = Rs/2
  float M = obj.schwarzschildRadius * 0.5f;
  float omega = sqrtf(M / (r * r * r));
  float theta = atan2f(dz, dx);
  float rotatedTheta = theta + time * omega * 10.0f;

  // Spiral arm brightness modulation
  float spiralPhase = 2.0f * rotatedTheta - 3.0f * t;
  float spiralBright = 0.75f + 0.25f * sinf(spiralPhase);

  // Turbulence: noise-based brightness variation for clumps and filaments
  if (turbulenceOn) {
    float turb = diskNoise(dx * 0.5f, dz * 0.5f, time * 0.8f);
    // Fine detail layer
    float detail = diskNoise(dx * 1.2f, dz * 1.2f, time * 1.2f);
    float turbFactor = 0.7f + 0.3f * (turb * 0.6f + detail * 0.4f + 0.5f);
    spiralBright *= turbFactor;
  }

  // Doppler beaming: orbital velocity in the XZ plane (counter-clockwise)
  float v = omega * r; // tangential speed (units of c when M = Rs/2)
  float beta = fminf(fmaxf(v, 0.0f), 0.95f); // cap at 0.95c
  float3 vOrbital = {-sinf(theta) * v, 0.0f, cosf(theta) * v};

  // Direction from hit point toward observer (flat-space approximation)
  float3 toObs = normalize3(camPos - hitPos);
  float betaLos = dot3(vOrbital, toObs); // line-of-sight velocity
  betaLos = fminf(fmaxf(betaLos, -0.95f), 0.95f);

  // Relativistic Doppler factor: D = 1 / (gamma * (1 - beta_los))
  float gamma = 1.0f / sqrtf(1.0f - beta * beta);
  float D = 1.0f / (gamma * (1.0f - betaLos));
  D = fminf(fmaxf(D, 0.25f), 4.0f);

  // Color tint FIRST (before brightness), using lerp to target color
  // This ensures tint is visible even after brightness scaling
  float tintStrength = fminf(fabsf(D - 1.0f) * 2.0f, 0.7f);
  float3 tintedColor = baseColor;
  if (D > 1.0f) {
    // Approaching: lerp toward blue-white
    tintedColor.x = baseColor.x * (1.0f - tintStrength) + 0.7f * tintStrength;
    tintedColor.y = baseColor.y * (1.0f - tintStrength) + 0.85f * tintStrength;
    tintedColor.z = baseColor.z * (1.0f - tintStrength) + 1.0f * tintStrength;
  } else {
    // Receding: lerp toward deep red-orange
    tintedColor.x = baseColor.x * (1.0f - tintStrength) + 1.0f * tintStrength;
    tintedColor.y = baseColor.y * (1.0f - tintStrength) + 0.3f * tintStrength;
    tintedColor.z = baseColor.z * (1.0f - tintStrength) + 0.1f * tintStrength;
  }

  // Brightness: D² (not D³ — avoids blowout in LDR pipeline)
  float beaming = D * D;

  // Gravitational redshift: light loses energy climbing out of the gravity well
  // factor = sqrt(1 - Rs/r) → 0 at horizon, 1 at infinity
  float Rs = obj.schwarzschildRadius;
  float gFactor = sqrtf(fmaxf(1.0f - Rs / r, 0.01f));
  // Differential per-channel: blue/green shift more than red
  float gR = 0.6f + 0.4f * gFactor; // red least affected
  float gG = 0.3f + 0.7f * gFactor; // green moderately affected
  float gB = 0.1f + 0.9f * gFactor; // blue most affected

  // Apply brightness, beaming, and gravitational redshift (HDR — no clamping)
  return {
    tintedColor.x * spiralBright * beaming * gR,
    tintedColor.y * spiralBright * beaming * gG,
    tintedColor.z * spiralBright * beaming * gB
  };
}

// Procedural starfield: hash-based pseudo-random stars on the sky sphere.
// Returns a star brightness [0,1] based on the ray's final direction.
__device__ float starField(float3 dir) {
  // Map direction to a grid on the unit sphere
  float theta = atan2f(dir.z, dir.x); // [-pi, pi]
  float phi = asinf(fminf(fmaxf(dir.y, -1.0f), 1.0f)); // [-pi/2, pi/2]

  // Grid cell for star placement
  float gridScale = 80.0f;
  float cellX = floorf(theta * gridScale);
  float cellY = floorf(phi * gridScale);

  // Hash function for pseudo-random star placement
  unsigned int h = (unsigned int)(cellX * 7919.0f + cellY * 6271.0f);
  h = (h ^ (h >> 13)) * 1274126177u;
  h = h ^ (h >> 16);
  float rand = (float)(h & 0xFFFF) / 65535.0f;

  // Only ~8% of cells have a star
  if (rand > 0.08f) return 0.0f;

  // Star brightness varies
  float brightness = 0.4f + 0.6f * ((float)((h >> 8) & 0xFF) / 255.0f);
  return brightness;
}

// Compute jet glow contribution for a point inside a BH's jet cone.
// Returns additive RGB glow (0 if point is outside the jet).
__device__ float3 jetGlow(float3 pos, const DeviceSceneObject& obj) {
  if (!obj.hasJets) return {0.0f, 0.0f, 0.0f};

  float3 rel = pos - obj.position;
  float dy = rel.y;
  float absDy = fabsf(dy);
  if (absDy < 0.1f || absDy > obj.jetLength) return {0.0f, 0.0f, 0.0f};

  // Distance from Y axis (jet axis)
  float axisDistSq = rel.x * rel.x + rel.z * rel.z;
  float dist = sqrtf(axisDistSq + dy * dy);

  // Cone test: cos(angle from Y axis) must exceed jetHalfAngleCos
  float cosAngle = absDy / dist;
  if (cosAngle < obj.jetHalfAngleCos) return {0.0f, 0.0f, 0.0f};

  // Radial falloff: brighter near axis
  float axisDist = sqrtf(axisDistSq);
  float coneRadius = absDy * sqrtf(1.0f / (obj.jetHalfAngleCos * obj.jetHalfAngleCos) - 1.0f);
  float radialFalloff = 1.0f - axisDist / fmaxf(coneRadius, 0.01f);
  radialFalloff = fmaxf(radialFalloff, 0.0f);
  radialFalloff *= radialFalloff; // squared for sharper edge

  // Length falloff: fade toward tip
  float lengthT = absDy / obj.jetLength;
  float lengthFalloff = 1.0f - lengthT * lengthT;

  // Brightness knots: sinusoidal variation along jet
  float knots = 0.7f + 0.3f * sinf(absDy * 1.2f);

  float intensity = radialFalloff * lengthFalloff * knots * 0.015f;

  // Blue-white jet color
  return {intensity * 0.5f, intensity * 0.65f, intensity * 1.0f};
}

// Sample equirectangular skybox texture using a direction vector.
// Returns RGB [0,1]. Falls back to background color if no texture.
__device__ float3 sampleSkybox(cudaTextureObject_t tex, float3 dir) {
  // Direction → equirectangular UV
  float theta = atan2f(dir.z, dir.x); // [-pi, pi]
  float phi = asinf(fminf(fmaxf(dir.y, -1.0f), 1.0f)); // [-pi/2, pi/2]

  float u = theta / (2.0f * 3.14159265f) + 0.5f; // [0, 1]
  float v = phi / 3.14159265f + 0.5f;             // [0, 1]

  float4 texel = tex2D<float4>(tex, u, v);
  return {texel.x, texel.y, texel.z};
}

// Compute lensing-based color for escaped rays.
// bendAngle: angle between initial and final ray direction (radians).
// finalDir: ray's final direction (used for starfield/skybox lookup).
__device__ float3 lensingColor(float3 bgColor, float3 finalDir,
                               float bendAngle,
                               cudaTextureObject_t skyboxTex) {
  // Lensing amplification: rays bent more → brighter (gravitational focusing)
  float amplification = 1.0f + 2.0f * bendAngle * bendAngle;
  amplification = fminf(amplification, 4.0f);

  float3 col;

  if (skyboxTex != 0) {
    // Sample skybox texture
    col = sampleSkybox(skyboxTex, finalDir);
    // Apply lensing amplification
    col.x *= amplification;
    col.y *= amplification;
    col.z *= amplification;
  } else {
    // Procedural fallback: background + starfield
    col = bgColor;

    float star = starField(finalDir);
    if (star > 0.0f) {
      float s = star * amplification;
      col.x = fminf(col.x + s, 1.0f);
      col.y = fminf(col.y + s, 1.0f);
      col.z = fminf(col.z + s * 0.9f, 1.0f);
    }
  }

  // Add lensing glow (bluish-white shift for strongly bent rays)
  float lensGlow = fminf(bendAngle * 0.8f, 0.5f);
  col.x += lensGlow * 0.3f;
  col.y += lensGlow * 0.4f;
  col.z += lensGlow * 0.6f;

  return col;
}

__global__ void unifiedTraceKernel(float4* hdrBuffer, int width, int height,
                                   DeviceCamera camera,
                                   UnifiedTraceParams params) {
  int x = blockIdx.x * blockDim.x + threadIdx.x;
  int y = blockIdx.y * blockDim.y + threadIdx.y;
  if (x >= width || y >= height) return;

  float3 dir = unifiedGenerateRay(x, y, width, height, camera);

  float3 pos = camera.position;
  float3 vel = dir;
  float dt = params.dt;
  float escapeR2 = params.escapeRadius * params.escapeRadius;

  float3 bgColor = params.backgroundColor;
  float4 hdrColor = {bgColor.x, bgColor.y, bgColor.z, 0.0f}; // alpha 0 = transparent

  // Track closest approach to any BH's photon sphere (1.5 * Rs)
  float photonRingProximity = 1e10f; // min (r - 1.5*Rs) / Rs across all BHs

  // Accumulate jet glow along the ray
  float3 jetAccum = {0.0f, 0.0f, 0.0f};

  for (int step = 0; step < params.maxSteps; step++) {
    float3 prevPos = pos;

    // Sum gravitational acceleration from all black holes
    float3 accel = {0.0f, 0.0f, 0.0f};
    for (int i = 0; i < c_objectCount; i++) {
      if (c_objects[i].type == OBJ_BLACK_HOLE) {
        float3 rel = pos - c_objects[i].position;
        accel = accel + geodesicAccel3D(rel, c_objects[i].schwarzschildRadius);
      }
    }

    // RK4 integration
    float3 k1v = accel;
    float3 k1x = vel;

    float3 p2 = pos + 0.5f * dt * k1x;
    float3 v2 = vel + 0.5f * dt * k1v;

    float3 a2 = {0.0f, 0.0f, 0.0f};
    for (int i = 0; i < c_objectCount; i++) {
      if (c_objects[i].type == OBJ_BLACK_HOLE) {
        float3 rel = p2 - c_objects[i].position;
        a2 = a2 + geodesicAccel3D(rel, c_objects[i].schwarzschildRadius);
      }
    }
    float3 k2v = a2;
    float3 k2x = v2;

    float3 p3 = pos + 0.5f * dt * k2x;
    float3 v3 = vel + 0.5f * dt * k2v;

    float3 a3 = {0.0f, 0.0f, 0.0f};
    for (int i = 0; i < c_objectCount; i++) {
      if (c_objects[i].type == OBJ_BLACK_HOLE) {
        float3 rel = p3 - c_objects[i].position;
        a3 = a3 + geodesicAccel3D(rel, c_objects[i].schwarzschildRadius);
      }
    }
    float3 k3v = a3;
    float3 k3x = v3;

    float3 p4 = pos + dt * k3x;
    float3 v4 = vel + dt * k3v;

    float3 a4 = {0.0f, 0.0f, 0.0f};
    for (int i = 0; i < c_objectCount; i++) {
      if (c_objects[i].type == OBJ_BLACK_HOLE) {
        float3 rel = p4 - c_objects[i].position;
        a4 = a4 + geodesicAccel3D(rel, c_objects[i].schwarzschildRadius);
      }
    }
    float3 k4v = a4;
    float3 k4x = v4;

    float sixth = dt / 6.0f;
    pos = pos + sixth * (k1x + 2.0f * k2x + 2.0f * k3x + k4x);
    vel = vel + sixth * (k1v + 2.0f * k2v + 2.0f * k3v + k4v);

    // Track closest approach to any BH's photon sphere
    for (int i = 0; i < c_objectCount; i++) {
      if (c_objects[i].type == OBJ_BLACK_HOLE) {
        float3 rel = pos - c_objects[i].position;
        float r = sqrtf(dot3(rel, rel));
        float Rs = c_objects[i].schwarzschildRadius;
        float proximity = (r - 1.5f * Rs) / Rs; // normalized distance from photon sphere
        if (proximity < photonRingProximity)
          photonRingProximity = proximity;
      }
    }

    // Accumulate jet glow if jets enabled
    if (params.jetsEnabled) {
      for (int i = 0; i < c_objectCount; i++) {
        float3 jg = jetGlow(pos, c_objects[i]);
        jetAccum.x += jg.x;
        jetAccum.y += jg.y;
        jetAccum.z += jg.z;
      }
    }

    // Check all objects for hits using segment intersection (prevPos → pos)
    float bestT = 2.0f; // > 1.0 means no hit
    int hitIdx = -1;

    for (int i = 0; i < c_objectCount; i++) {
      float radius = (c_objects[i].type == OBJ_BLACK_HOLE)
                         ? c_objects[i].schwarzschildRadius
                         : c_objects[i].visualRadius;
      float tHit;
      if (segmentSphereIntersect(prevPos, pos, c_objects[i].position,
                                 radius, tHit)) {
        if (tHit < bestT) {
          bestT = tHit;
          hitIdx = i;
        }
      }
    }

    if (hitIdx >= 0) {
      float3 c = c_objects[hitIdx].color;
      hdrColor = {c.x, c.y, c.z, 1.0f};
      break;
    }

    // Check accretion disk intersections (disks are properties of BH objects)
    bool diskHit = false;
    for (int i = 0; i < c_objectCount; i++) {
      if (!c_objects[i].hasAccretionDisk) continue;
      float tHit;
      if (segmentDiskIntersect(prevPos, pos, c_objects[i].position,
                               c_objects[i].diskInnerRadius,
                               c_objects[i].diskOuterRadius,
                               c_objects[i].diskHalfThickness,
                               params.time, params.turbulenceEnabled,
                               tHit)) {
        float3 hitPt = prevPos + tHit * (pos - prevPos);
        float3 dc = diskColor(hitPt, c_objects[i], params.time, camera.position,
                              params.turbulenceEnabled);
        hdrColor = {dc.x, dc.y, dc.z, 1.0f};
        diskHit = true;
        break;
      }
    }
    if (diskHit) break;

    // Escape check: distance from origin — apply lensing shading
    if (dot3(pos, pos) > escapeR2) {
      float3 finalDir = normalize3(vel);
      float cosAngle = dot3(dir, finalDir);
      cosAngle = fminf(fmaxf(cosAngle, -1.0f), 1.0f);
      float bendAngle = acosf(cosAngle);

      float3 lc = lensingColor(bgColor, finalDir, bendAngle, params.skyboxTex);

      // Photon ring: bright glow for rays that grazed the photon sphere (1.5 * Rs)
      // proximity < 0 means ray went inside photon sphere but escaped (very bent)
      // proximity ~0 means ray skimmed the photon sphere (brightest ring)
      if (photonRingProximity < 1.0f) {
        float ringGlow = expf(-photonRingProximity * photonRingProximity * 4.0f);
        float ringIntensity = ringGlow * 0.8f;
        lc.x += ringIntensity * 0.85f;
        lc.y += ringIntensity * 0.9f;
        lc.z += ringIntensity * 1.0f;
      }

      float alpha = (params.skyboxTex != 0) ? 1.0f : 0.0f;
      hdrColor = {lc.x, lc.y, lc.z, alpha};
      break;
    }
  }

  // Add accumulated jet glow (additive, semi-transparent)
  if (jetAccum.x > 0.0f || jetAccum.y > 0.0f || jetAccum.z > 0.0f) {
    hdrColor.x += jetAccum.x;
    hdrColor.y += jetAccum.y;
    hdrColor.z += jetAccum.z;
    if (hdrColor.w < 0.01f) {
      float jetTotal = jetAccum.x + jetAccum.y + jetAccum.z;
      if (jetTotal > 0.02f)
        hdrColor.w = fminf(jetTotal * 10.0f, 1.0f);
    }
  }

  hdrBuffer[(height - 1 - y) * width + x] = hdrColor;
}

void launchUnifiedKernel(float4* hdrBuffer, int width, int height,
                         const DeviceCamera& camera,
                         const DeviceSceneObject* objects, int objectCount,
                         const UnifiedTraceParams& params) {
  if (objectCount > MAX_SCENE_OBJECTS) {
    std::fprintf(stderr, "Too many scene objects (%d > %d)\n",
                 objectCount, MAX_SCENE_OBJECTS);
    objectCount = MAX_SCENE_OBJECTS;
  }

  // Copy objects to constant memory
  cudaMemcpyToSymbol(c_objects, objects,
                     objectCount * sizeof(DeviceSceneObject));
  cudaMemcpyToSymbol(c_objectCount, &objectCount, sizeof(int));

  dim3 block(16, 16);
  dim3 grid((width + block.x - 1) / block.x,
            (height + block.y - 1) / block.y);

  unifiedTraceKernel<<<grid, block>>>(hdrBuffer, width, height,
                                      camera, params);
  cudaDeviceSynchronize();
}

// ACES filmic tone mapping: cinematic S-curve with warm highlights
__device__ float3 acesFilmic(float3 x) {
  float3 a = {x.x * (2.51f * x.x + 0.03f),
              x.y * (2.51f * x.y + 0.03f),
              x.z * (2.51f * x.z + 0.03f)};
  float3 b = {x.x * (2.43f * x.x + 0.59f) + 0.14f,
              x.y * (2.43f * x.y + 0.59f) + 0.14f,
              x.z * (2.43f * x.z + 0.59f) + 0.14f};
  return {fminf(fmaxf(a.x / b.x, 0.0f), 1.0f),
          fminf(fmaxf(a.y / b.y, 0.0f), 1.0f),
          fminf(fmaxf(a.z / b.z, 0.0f), 1.0f)};
}

__global__ void toneMapKernel(const float4* hdr, uchar4* ldr,
                              int width, int height, float exposure) {
  int x = blockIdx.x * blockDim.x + threadIdx.x;
  int y = blockIdx.y * blockDim.y + threadIdx.y;
  if (x >= width || y >= height) return;

  int idx = y * width + x;
  float4 pixel = hdr[idx];

  // Apply exposure
  float3 col = {pixel.x * exposure, pixel.y * exposure, pixel.z * exposure};

  // ACES filmic tone map
  col = acesFilmic(col);

  // Gamma correction (linear → sRGB)
  col.x = powf(col.x, 1.0f / 2.2f);
  col.y = powf(col.y, 1.0f / 2.2f);
  col.z = powf(col.z, 1.0f / 2.2f);

  ldr[idx] = {
    (unsigned char)(col.x * 255.0f),
    (unsigned char)(col.y * 255.0f),
    (unsigned char)(col.z * 255.0f),
    (unsigned char)(fminf(fmaxf(pixel.w, 0.0f), 1.0f) * 255.0f) // alpha passthrough
  };
}

void launchToneMapKernel(const float4* hdrBuffer, uchar4* ldrBuffer,
                         int width, int height, float exposure) {
  dim3 block(16, 16);
  dim3 grid((width + block.x - 1) / block.x,
            (height + block.y - 1) / block.y);
  toneMapKernel<<<grid, block>>>(hdrBuffer, ldrBuffer, width, height, exposure);
  cudaDeviceSynchronize();
}

} // namespace bht::gpu
