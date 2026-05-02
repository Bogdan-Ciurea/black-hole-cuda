#pragma once

#include "gpu/device_types.h"

#include <cuda_runtime.h>

namespace bht::gpu {

// Trace parameters for the unified kernel (all values in world-space units)
struct UnifiedTraceParams {
  int maxSteps = 3000;
  float dt = 0.4f;             // integration step (world units)
  float escapeRadius = 160.0f; // ray considered escaped beyond this distance
  float3 backgroundColor;      // RGB [0,1]
};

// Launch the unified ray tracing kernel.
// Objects must be in host memory — they are copied to constant memory internally.
// Black holes with hasAccretionDisk=1 will have their disks rendered automatically.
void launchUnifiedKernel(uchar4* framebuffer, int width, int height,
                         const DeviceCamera& camera,
                         const DeviceSceneObject* objects, int objectCount,
                         const UnifiedTraceParams& params = {});

} // namespace bht::gpu
