#pragma once

#include <cuda_runtime.h>
#include <glad/gl.h>

namespace bht::gpu {

// CUDA ↔ OpenGL interop via Pixel Buffer Object (PBO).
// CUDA writes uchar4 pixels to the PBO, which is then uploaded to a GL texture.
// No CPU copies involved — all transfers happen on the GPU.
class CudaGLInterop {
public:
  CudaGLInterop() = default;
  ~CudaGLInterop();

  // Initialize: creates PBO + texture + shader
  bool init(int width, int height);

  // Map PBO for CUDA writing. Returns device pointer to uchar4[width*height].
  uchar4* map();

  // Unmap PBO and copy to texture for display.
  void unmap();

  // Draw the texture as a fullscreen quad.
  void draw() const;

  int width() const { return width_; }
  int height() const { return height_; }

private:
  int width_ = 0;
  int height_ = 0;
  GLuint pbo_ = 0;
  GLuint texture_ = 0;
  GLuint vao_ = 0;
  GLuint shader_ = 0;
  cudaGraphicsResource_t cudaResource_ = nullptr;
};

} // namespace bht::gpu
