#include "gpu/ray_kernel.h"

#include <cmath>

namespace bht::gpu {

__device__ float3 operator+(float3 a, float3 b) {
  return {a.x + b.x, a.y + b.y, a.z + b.z};
}

__device__ float3 operator-(float3 a, float3 b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

__device__ float3 operator*(float s, float3 v) {
  return {s * v.x, s * v.y, s * v.z};
}

__device__ float3 operator*(float3 v, float s) {
  return {s * v.x, s * v.y, s * v.z};
}

__device__ float dot(float3 a, float3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

__device__ float3 normalize(float3 v) {
  float len = sqrtf(dot(v, v));
  if (len < 1e-10f) return {0.0f, 0.0f, 0.0f};
  float inv = 1.0f / len;
  return {v.x * inv, v.y * inv, v.z * inv};
}

// 3D geodesic acceleration (Schwarzschild, post-Newtonian approx)
__device__ float3 geodesicAccel(float3 pos, float Rs) {
  float r2 = dot(pos, pos);
  float r = sqrtf(r2);
  float r3 = r2 * r;

  if (r < 1e-10f) return {0.0f, 0.0f, 0.0f};

  float factor = -0.5f * Rs / r3;
  float grCorrection = 1.5f * Rs / r;

  return (factor * (1.0f + grCorrection)) * pos;
}

// Generate ray direction for pixel (x, y)
__device__ float3 generateRay(int x, int y, int width, int height,
                              const DeviceCamera& cam) {
  float aspectRatio = (float)width / (float)height;
  float tanHalfFov = tanf(cam.fov * 3.14159265f / 360.0f);

  float ndcX = (2.0f * (x + 0.5f) / width - 1.0f) * aspectRatio * tanHalfFov;
  float ndcY = (1.0f - 2.0f * (y + 0.5f) / height) * tanHalfFov;

  float3 dir = cam.forward + ndcX * cam.right + ndcY * cam.up;
  return normalize(dir);
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
    // RK4 integration
    float3 k1v = geodesicAccel(pos, Rs);
    float3 k1x = vel;

    float3 p2 = pos + 0.5f * dt * k1x;
    float3 v2 = vel + 0.5f * dt * k1v;
    float3 k2v = geodesicAccel(p2, Rs);
    float3 k2x = v2;

    float3 p3 = pos + 0.5f * dt * k2x;
    float3 v3 = vel + 0.5f * dt * k2v;
    float3 k3v = geodesicAccel(p3, Rs);
    float3 k3x = v3;

    float3 p4 = pos + dt * k3x;
    float3 v4 = vel + dt * k3v;
    float3 k4v = geodesicAccel(p4, Rs);
    float3 k4x = v4;

    float sixth = dt / 6.0f;
    pos = pos + sixth * (k1x + 2.0f * k2x + 2.0f * k3x + k4x);
    vel = vel + sixth * (k1v + 2.0f * k2v + 2.0f * k3v + k4v);

    float r2 = dot(pos, pos);

    // Hit black hole
    if (r2 < captureR2) {
      color = {204, 0, 0, 255}; // red
      break;
    }

    // Escaped
    if (r2 > escapeR2) {
      float3 normDir = normalize(vel);
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
