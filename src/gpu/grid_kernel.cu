#include "gpu/grid_kernel.h"
#include "gpu/cuda_math.cuh"

#include <cmath>

namespace bht::gpu {

// Gravity well visualization: flat far from the black hole, dipping down near it.
// Based on inverted Flamm's paraboloid for intuitive "rubber sheet" look.
// y_max = 2*sqrt(Rs * (rMax - Rs)) at the grid edge (reference flat level)
// y(r) = -(y_max - 2*sqrt(Rs * (r - Rs)))  for r >= Rs
__global__ void gridDeformKernel(float3* deformed, GridParams params) {
  int idx = blockIdx.x * blockDim.x + threadIdx.x;
  int total = params.resolution * params.resolution;
  if (idx >= total) return;

  int ix = idx % params.resolution;
  int iz = idx / params.resolution;

  float spacing = 2.0f * params.extent / (params.resolution - 1);
  float x = -params.extent + ix * spacing;
  float z = -params.extent + iz * spacing;

  float r = sqrtf(x * x + z * z);
  float Rs = params.Rs;

  // Reference height at grid edge (so edges are flat at y=0)
  float rMax = params.extent * 1.414f; // diagonal extent
  float yMax = 2.0f * sqrtf(Rs * (rMax - Rs));

  float y;
  if (r > Rs) {
    float flamm = 2.0f * sqrtf(Rs * (r - Rs));
    y = -(yMax - flamm); // inverted: 0 at edge, deep negative near BH
  } else {
    y = -yMax; // bottom of the well
  }

  deformed[idx] = {x, y, z};
}

void launchGridDeformKernel(float3* deformed, const GridParams& params) {
  int total = params.resolution * params.resolution;
  int blockSize = 256;
  int gridSize = (total + blockSize - 1) / blockSize;

  gridDeformKernel<<<gridSize, blockSize>>>(deformed, params);
  cudaDeviceSynchronize();
}

} // namespace bht::gpu
