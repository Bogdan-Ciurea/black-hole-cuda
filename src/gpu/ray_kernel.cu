#include "gpu/ray_kernel.h"
#include "gpu/cuda_math.cuh"

#include <cmath>

namespace bht::gpu {

// Generate ray direction for pixel (x, y)
__device__ float3 generateRay(int x, int y, int width, int height,
                              const DeviceCamera& cam) {
  float aspectRatio = (float)width / (float)height;
  float tanHalfFov = tanf(cam.fov * 3.14159265f / 360.0f);

  float ndcX = (2.0f * (x + 0.5f) / width - 1.0f) * aspectRatio * tanHalfFov;
  float ndcY = (1.0f - 2.0f * (y + 0.5f) / height) * tanHalfFov;

  float3 dir = cam.forward + ndcX * cam.right + ndcY * cam.up;
  return normalize3(dir);
}

__global__ void rayTraceKernel(uchar4* framebuffer, int width, int height,
                               DeviceCamera camera, DeviceBlackHole bh,
                               DeviceTraceParams params) {
  int x = blockIdx.x * blockDim.x + threadIdx.x;
  int y = blockIdx.y * blockDim.y + threadIdx.y;
  if (x >= width || y >= height) return;

  float3 dir = generateRay(x, y, width, height, camera);

  float3 pos = camera.position;
  float3 vel = dir;

  float Rs = bh.Rs;
  float dt = params.dt * Rs;
  float escapeR2 = (params.escapeRadius * Rs) * (params.escapeRadius * Rs);
  float captureR2 = Rs * Rs;

  uchar4 color = {0, 0, 13, 255}; // dark blue background

  for (int step = 0; step < params.maxSteps; step++) {
    float3 k1v = geodesicAccel3D(pos, Rs);
    float3 k1x = vel;

    float3 p2 = pos + 0.5f * dt * k1x;
    float3 v2 = vel + 0.5f * dt * k1v;
    float3 k2v = geodesicAccel3D(p2, Rs);
    float3 k2x = v2;

    float3 p3 = pos + 0.5f * dt * k2x;
    float3 v3 = vel + 0.5f * dt * k2v;
    float3 k3v = geodesicAccel3D(p3, Rs);
    float3 k3x = v3;

    float3 p4 = pos + dt * k3x;
    float3 v4 = vel + dt * k3v;
    float3 k4v = geodesicAccel3D(p4, Rs);
    float3 k4x = v4;

    float sixth = dt / 6.0f;
    pos = pos + sixth * (k1x + 2.0f * k2x + 2.0f * k3x + k4x);
    vel = vel + sixth * (k1v + 2.0f * k2v + 2.0f * k3v + k4v);

    float r2 = dot3(pos, pos);

    if (r2 < captureR2) {
      color = {204, 0, 0, 255};
      break;
    }

    if (r2 > escapeR2) {
      float3 normDir = normalize3(vel);
      float t = 0.5f * (normDir.y + 1.0f);
      unsigned char bg = (unsigned char)(26.0f + t * 13.0f);
      color = {bg, bg, (unsigned char)(bg + 26), 255};
      break;
    }
  }

  framebuffer[y * width + x] = color;
}

void launchRayKernel(uchar4* framebuffer, int width, int height,
                     const DeviceCamera& camera, const DeviceBlackHole& bh,
                     const DeviceTraceParams& params) {
  dim3 block(16, 16);
  dim3 grid((width + block.x - 1) / block.x, (height + block.y - 1) / block.y);

  rayTraceKernel<<<grid, block>>>(framebuffer, width, height, camera, bh, params);
  cudaDeviceSynchronize();
}

} // namespace bht::gpu
