#define STB_IMAGE_IMPLEMENTATION
#include "gpu/skybox.h"

#include <stb_image.h>

#include <cstdio>

namespace bht::gpu {

Skybox::~Skybox() {
  if (texObj_) cudaDestroyTextureObject(texObj_);
  if (cuArray_) cudaFreeArray(cuArray_);
}

bool Skybox::load(const std::string& filepath) {
  // Load image (force 4 channels: RGBA)
  int w, h, channels;
  unsigned char* data = stbi_load(filepath.c_str(), &w, &h, &channels, 4);
  if (!data) {
    std::fprintf(stderr, "Skybox: failed to load '%s': %s\n",
                 filepath.c_str(), stbi_failure_reason());
    return false;
  }

  std::printf("Skybox: loaded '%s' (%dx%d, %d channels)\n",
              filepath.c_str(), w, h, channels);

  width_ = w;
  height_ = h;

  // Create CUDA array
  cudaChannelFormatDesc desc = cudaCreateChannelDesc<uchar4>();
  cudaError_t err = cudaMallocArray(&cuArray_, &desc, w, h);
  if (err != cudaSuccess) {
    std::fprintf(stderr, "Skybox: cudaMallocArray failed: %s\n",
                 cudaGetErrorString(err));
    stbi_image_free(data);
    return false;
  }

  // Copy image data to CUDA array
  err = cudaMemcpy2DToArray(cuArray_, 0, 0, data, w * 4,
                            w * 4, h, cudaMemcpyHostToDevice);
  stbi_image_free(data);
  if (err != cudaSuccess) {
    std::fprintf(stderr, "Skybox: cudaMemcpy2DToArray failed: %s\n",
                 cudaGetErrorString(err));
    cudaFreeArray(cuArray_);
    cuArray_ = nullptr;
    return false;
  }

  // Create texture object with bilinear filtering and wrap addressing
  cudaResourceDesc resDesc = {};
  resDesc.resType = cudaResourceTypeArray;
  resDesc.res.array.array = cuArray_;

  cudaTextureDesc texDesc = {};
  texDesc.addressMode[0] = cudaAddressModeWrap;
  texDesc.addressMode[1] = cudaAddressModeClamp;
  texDesc.filterMode = cudaFilterModeLinear;
  texDesc.readMode = cudaReadModeNormalizedFloat;
  texDesc.normalizedCoords = 1;

  err = cudaCreateTextureObject(&texObj_, &resDesc, &texDesc, nullptr);
  if (err != cudaSuccess) {
    std::fprintf(stderr, "Skybox: cudaCreateTextureObject failed: %s\n",
                 cudaGetErrorString(err));
    cudaFreeArray(cuArray_);
    cuArray_ = nullptr;
    return false;
  }

  return true;
}

} // namespace bht::gpu
