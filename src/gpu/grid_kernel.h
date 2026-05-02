#pragma once

#include <cuda_runtime.h>

namespace bht::gpu {

struct GridParams {
  float extent = 30.0f;    // half-size of grid in world units
  int resolution = 80;     // points per axis
  float Rs = 2.0f;         // Schwarzschild radius
  float dt = 0.05f;        // integration step
  int steps = 200;         // integration steps per point
};

// Launch kernel to compute deformed grid positions.
// Output: deformed[resolution * resolution] float3 positions
void launchGridDeformKernel(float3* deformed, const GridParams& params);

} // namespace bht::gpu
