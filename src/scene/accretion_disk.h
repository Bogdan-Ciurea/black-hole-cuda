#pragma once

#include <cuda_runtime.h>

namespace bht::scene {

// Flat accretion disk around a black hole, lying in the XZ plane.
// Color gradient from inner (hot) to outer (cool) edge.
struct AccretionDisk {
  float3 center;        // disk center (usually BH position)
  float innerRadius;    // inner edge (typically ~3 * Rs, ISCO)
  float outerRadius;    // outer edge
  float3 innerColor;    // hot inner color (e.g., white/yellow)
  float3 outerColor;    // cool outer color (e.g., red/orange)
  float emissionStrength = 1.0f;
};

} // namespace bht::scene
