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

// Segment-disk intersection: detect crossing of the XZ plane at y = diskY.
// Returns true if the segment crosses the plane within [innerR, outerR].
// Sets tHit to the parametric crossing point [0,1] along the segment.
__device__ bool segmentDiskIntersect(float3 p0, float3 p1,
                                     float3 center, float innerR, float outerR,
                                     float& tHit) {
  float dy = p1.y - p0.y;
  if (fabsf(dy) < 1e-10f) return false; // segment parallel to disk

  float t = (center.y - p0.y) / dy;
  if (t < 0.0f || t > 1.0f) return false; // crossing outside segment

  // Intersection point on the plane
  float hitX = p0.x + t * (p1.x - p0.x) - center.x;
  float hitZ = p0.z + t * (p1.z - p0.z) - center.z;
  float r2 = hitX * hitX + hitZ * hitZ;

  if (r2 >= innerR * innerR && r2 <= outerR * outerR) {
    tHit = t;
    return true;
  }
  return false;
}

// Compute disk color with radial gradient, Keplerian rotation, and Doppler beaming.
// camPos: camera position (observer) for Doppler shift calculation.
__device__ float3 diskColor(float3 hitPos, const DeviceSceneObject& obj,
                            float time, float3 camPos) {
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
  float rotatedTheta = theta + time * omega;

  // Spiral arm brightness modulation
  float spiralPhase = 2.0f * rotatedTheta - 3.0f * t;
  float spiralBright = 0.75f + 0.25f * sinf(spiralPhase);

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

  // Apply brightness after tint, then Reinhard tone map per-channel to preserve tint
  float3 hdr = {
    tintedColor.x * spiralBright * beaming,
    tintedColor.y * spiralBright * beaming,
    tintedColor.z * spiralBright * beaming
  };

  // Simple Reinhard: color / (1 + color) — preserves channel ratios
  float3 col = {
    hdr.x / (1.0f + hdr.x),
    hdr.y / (1.0f + hdr.y),
    hdr.z / (1.0f + hdr.z)
  };

  // Clamp to [0, 1]
  col.x = fminf(fmaxf(col.x, 0.0f), 1.0f);
  col.y = fminf(fmaxf(col.y, 0.0f), 1.0f);
  col.z = fminf(fmaxf(col.z, 0.0f), 1.0f);

  return col;
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

__global__ void unifiedTraceKernel(uchar4* framebuffer, int width, int height,
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
  uchar4 color = {
    (unsigned char)(bgColor.x * 255.0f),
    (unsigned char)(bgColor.y * 255.0f),
    (unsigned char)(bgColor.z * 255.0f),
    0  // transparent background — lets OpenGL grid show through
  };

  // Track closest approach to any BH's photon sphere (1.5 * Rs)
  float photonRingProximity = 1e10f; // min (r - 1.5*Rs) / Rs across all BHs

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
      color = {
        (unsigned char)(c.x * 255.0f),
        (unsigned char)(c.y * 255.0f),
        (unsigned char)(c.z * 255.0f),
        255
      };
      break;
    }

    // Check accretion disk intersections (disks are properties of BH objects)
    bool diskHit = false;
    for (int i = 0; i < c_objectCount; i++) {
      if (!c_objects[i].hasAccretionDisk) continue;
      float tHit;
      if (segmentDiskIntersect(prevPos, pos, c_objects[i].position,
                               c_objects[i].diskInnerRadius,
                               c_objects[i].diskOuterRadius, tHit)) {
        float3 hitPt = prevPos + tHit * (pos - prevPos);
        float3 dc = diskColor(hitPt, c_objects[i], params.time, camera.position);
        color = {
          (unsigned char)(dc.x * 255.0f),
          (unsigned char)(dc.y * 255.0f),
          (unsigned char)(dc.z * 255.0f),
          255
        };
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
        // Blue-white ring, intensity up to 0.8
        float ringIntensity = ringGlow * 0.8f;
        lc.x = fminf(lc.x + ringIntensity * 0.85f, 1.0f);
        lc.y = fminf(lc.y + ringIntensity * 0.9f, 1.0f);
        lc.z = fminf(lc.z + ringIntensity * 1.0f, 1.0f);
      }

      color = {
        (unsigned char)(fminf(lc.x, 1.0f) * 255.0f),
        (unsigned char)(fminf(lc.y, 1.0f) * 255.0f),
        (unsigned char)(fminf(lc.z, 1.0f) * 255.0f),
        (unsigned char)(params.skyboxTex != 0 ? 255 : 0)  // opaque if skybox, else transparent for grid
      };
      break;
    }
  }

  framebuffer[(height - 1 - y) * width + x] = color;
}

void launchUnifiedKernel(uchar4* framebuffer, int width, int height,
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

  unifiedTraceKernel<<<grid, block>>>(framebuffer, width, height,
                                      camera, params);
  cudaDeviceSynchronize();
}

} // namespace bht::gpu
