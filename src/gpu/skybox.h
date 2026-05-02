#pragma once

#include <cuda_runtime.h>

#include <string>

namespace bht::gpu {

// Loads an equirectangular skybox image and creates a CUDA texture object.
// The texture can be sampled in the kernel using direction → UV mapping.
class Skybox {
public:
  Skybox() = default;
  ~Skybox();

  // Load an equirectangular image (HDR or LDR). Returns false on failure.
  bool load(const std::string& filepath);

  // Returns the CUDA texture object for kernel sampling. 0 if not loaded.
  cudaTextureObject_t texture() const { return texObj_; }

  bool isLoaded() const { return texObj_ != 0; }

  int width() const { return width_; }
  int height() const { return height_; }

private:
  cudaArray_t cuArray_ = nullptr;
  cudaTextureObject_t texObj_ = 0;
  int width_ = 0;
  int height_ = 0;
};

} // namespace bht::gpu
