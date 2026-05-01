#include <cstdio>

__global__ void helloKernel() {
  // Placeholder kernel — proves CUDA compilation works
}

void launchHelloKernel() {
  helloKernel<<<1, 1>>>();
  cudaDeviceSynchronize();
  std::printf("  [CUDA] helloKernel executed on GPU\n");
}
