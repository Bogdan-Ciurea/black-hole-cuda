#pragma once

#include "gpu/device_types.h"

#include <cuda_runtime.h>

namespace bht::scene {

// CPU-side scene object description used by both the renderer and the kernel.
struct SceneObject {
  uint32_t type;               // OBJ_BLACK_HOLE or OBJ_SPHERE
  float3 position;
  float schwarzschildRadius;   // BH only
  float visualRadius;          // render size
  float3 color;                // RGB [0,1]
  float emissionStrength;      // 0 = non-emissive
  float3 emissionColor;
  bool drawGeodesicGrid;       // BH only, CPU-side flag for GL grid overlay

  // Convert to device-side object for the CUDA kernel
  bht::gpu::DeviceSceneObject toDevice() const {
    return {type, position, schwarzschildRadius, visualRadius,
            color, emissionStrength, emissionColor};
  }
};

} // namespace bht::scene
