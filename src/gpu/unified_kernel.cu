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
    255
  };

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

    // Escape check: distance from origin
    if (dot3(pos, pos) > escapeR2) break;
  }

  framebuffer[y * width + x] = color;
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
