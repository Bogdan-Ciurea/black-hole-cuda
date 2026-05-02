#pragma once

#include "gpu/device_types.h"

#include <cuda_runtime.h>

namespace bht::gpu {

// Device-side black hole
struct DeviceBlackHole {
  float3 position;
  float Rs; // Schwarzschild radius
};

// Trace parameters
struct DeviceTraceParams {
  int maxSteps = 3000;
  float dt = 0.2f;          // in units of Rs
  float escapeRadius = 80.0f; // in units of Rs
};

// Launch the ray tracing kernel
// Output: framebuffer (device pointer, width*height uchar4 RGBA)
void launchRayKernel(uchar4* framebuffer, int width, int height,
                     const DeviceCamera& camera, const DeviceBlackHole& bh,
                     const DeviceTraceParams& params = {});

} // namespace bht::gpu
