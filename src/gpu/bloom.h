#pragma once

#include <cuda_runtime.h>

namespace bht::gpu {

struct BloomParams {
  float threshold = 0.7f;   // luminance threshold for bright pixel extraction
  float intensity = 0.4f;   // bloom additive strength
  int radius = 8;           // Gaussian blur radius in pixels
  float sigma = 4.0f;       // Gaussian blur sigma
  bool enabled = true;
};

// CUDA-based bloom post-process.
// Runs on the GPU framebuffer: threshold → blur → composite.
class BloomPostProcess {
public:
  BloomPostProcess() = default;
  ~BloomPostProcess();

  // Allocate GPU scratch buffers. Call once at startup.
  bool init(int width, int height);

  // Apply bloom to the framebuffer in-place.
  // framebuffer must be a device pointer to uchar4[width*height].
  void apply(uchar4* framebuffer, const BloomParams& params);

private:
  int width_ = 0;
  int height_ = 0;
  uchar4* brightBuf_ = nullptr; // bright pixels extracted here
  uchar4* scratchBuf_ = nullptr; // intermediate blur buffer
};

} // namespace bht::gpu
