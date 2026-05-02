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
  float time = 0.0f;           // simulation time (for disk rotation, etc.)
  cudaTextureObject_t skyboxTex = 0; // equirectangular skybox (0 = use procedural)
  int turbulenceEnabled = 1;   // 1 = disk thickness/turbulence active
  int jetsEnabled = 1;         // 1 = particle jets visible
  float exposure = 1.0f;       // HDR exposure multiplier (applied before tone mapping)
};

// Launch the unified ray tracing kernel into an HDR float4 buffer.
// Objects must be in host memory — they are copied to constant memory internally.
void launchUnifiedKernel(float4* hdrBuffer, int width, int height,
                         const DeviceCamera& camera,
                         const DeviceSceneObject* objects, int objectCount,
                         const UnifiedTraceParams& params = {});

// Tone map HDR float4 buffer to LDR uchar4 using ACES filmic curve.
void launchToneMapKernel(const float4* hdrBuffer, uchar4* ldrBuffer,
                         int width, int height, float exposure);

} // namespace bht::gpu
