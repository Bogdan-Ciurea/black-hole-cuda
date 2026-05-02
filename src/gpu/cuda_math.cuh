#pragma once

#include <cuda_runtime.h>
#include <cmath>

namespace bht::gpu {

// Common float3 operators for CUDA device code
inline __device__ float3 operator+(float3 a, float3 b) {
  return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline __device__ float3 operator-(float3 a, float3 b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline __device__ float3 operator*(float s, float3 v) {
  return {s * v.x, s * v.y, s * v.z};
}

inline __device__ float3 operator*(float3 v, float s) {
  return {s * v.x, s * v.y, s * v.z};
}

inline __device__ float dot3(float3 a, float3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline __device__ float3 normalize3(float3 v) {
  float len = sqrtf(dot3(v, v));
  if (len < 1e-10f) return {0.0f, 0.0f, 0.0f};
  float inv = 1.0f / len;
  return {v.x * inv, v.y * inv, v.z * inv};
}

// Schwarzschild geodesic acceleration (3D, post-Newtonian approx)
inline __device__ float3 geodesicAccel3D(float3 pos, float Rs) {
  float r2 = dot3(pos, pos);
  float r = sqrtf(r2);
  float r3 = r2 * r;

  if (r < 1e-10f) return {0.0f, 0.0f, 0.0f};

  float factor = -0.5f * Rs / r3;
  float grCorrection = 1.5f * Rs / r;

  return (factor * (1.0f + grCorrection)) * pos;
}

} // namespace bht::gpu
