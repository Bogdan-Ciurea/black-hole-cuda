#include "gpu/bloom.h"

#include <cstdio>

namespace bht::gpu {

// Threshold kernel: extract bright pixels based on luminance.
__global__ void bloomThresholdKernel(const uchar4* __restrict__ src,
                                     uchar4* __restrict__ dst,
                                     int width, int height,
                                     float threshold) {
  int x = blockIdx.x * blockDim.x + threadIdx.x;
  int y = blockIdx.y * blockDim.y + threadIdx.y;
  if (x >= width || y >= height) return;

  int idx = y * width + x;
  uchar4 px = src[idx];

  // Luminance (perceptual weights)
  float lum = (0.299f * px.x + 0.587f * px.y + 0.114f * px.z) / 255.0f;

  if (lum > threshold) {
    // Keep the pixel, scaled by how far above threshold
    float factor = (lum - threshold) / (1.0f - threshold + 1e-6f);
    dst[idx] = {
      (unsigned char)(px.x * factor),
      (unsigned char)(px.y * factor),
      (unsigned char)(px.z * factor),
      0
    };
  } else {
    dst[idx] = {0, 0, 0, 0};
  }
}

// Horizontal Gaussian blur kernel.
__global__ void bloomBlurHKernel(const uchar4* __restrict__ src,
                                 uchar4* __restrict__ dst,
                                 int width, int height,
                                 int radius, float sigma) {
  int x = blockIdx.x * blockDim.x + threadIdx.x;
  int y = blockIdx.y * blockDim.y + threadIdx.y;
  if (x >= width || y >= height) return;

  float invSigma2 = -0.5f / (sigma * sigma);
  float sumR = 0.0f, sumG = 0.0f, sumB = 0.0f, sumW = 0.0f;

  for (int dx = -radius; dx <= radius; dx++) {
    int sx = min(max(x + dx, 0), width - 1);
    float w = expf(dx * dx * invSigma2);
    uchar4 px = src[y * width + sx];
    sumR += px.x * w;
    sumG += px.y * w;
    sumB += px.z * w;
    sumW += w;
  }

  float invW = 1.0f / sumW;
  int idx = y * width + x;
  dst[idx] = {
    (unsigned char)(sumR * invW),
    (unsigned char)(sumG * invW),
    (unsigned char)(sumB * invW),
    0
  };
}

// Vertical Gaussian blur kernel.
__global__ void bloomBlurVKernel(const uchar4* __restrict__ src,
                                 uchar4* __restrict__ dst,
                                 int width, int height,
                                 int radius, float sigma) {
  int x = blockIdx.x * blockDim.x + threadIdx.x;
  int y = blockIdx.y * blockDim.y + threadIdx.y;
  if (x >= width || y >= height) return;

  float invSigma2 = -0.5f / (sigma * sigma);
  float sumR = 0.0f, sumG = 0.0f, sumB = 0.0f, sumW = 0.0f;

  for (int dy = -radius; dy <= radius; dy++) {
    int sy = min(max(y + dy, 0), height - 1);
    float w = expf(dy * dy * invSigma2);
    uchar4 px = src[sy * width + x];
    sumR += px.x * w;
    sumG += px.y * w;
    sumB += px.z * w;
    sumW += w;
  }

  float invW = 1.0f / sumW;
  int idx = y * width + x;
  dst[idx] = {
    (unsigned char)(sumR * invW),
    (unsigned char)(sumG * invW),
    (unsigned char)(sumB * invW),
    0
  };
}

// Composite kernel: add blurred bloom on top of the original framebuffer.
__global__ void bloomCompositeKernel(uchar4* __restrict__ framebuffer,
                                     const uchar4* __restrict__ bloom,
                                     int width, int height,
                                     float intensity) {
  int x = blockIdx.x * blockDim.x + threadIdx.x;
  int y = blockIdx.y * blockDim.y + threadIdx.y;
  if (x >= width || y >= height) return;

  int idx = y * width + x;
  uchar4 orig = framebuffer[idx];
  uchar4 glow = bloom[idx];

  // Additive blend, preserve original alpha
  int r = orig.x + (int)(glow.x * intensity);
  int g = orig.y + (int)(glow.y * intensity);
  int b = orig.z + (int)(glow.z * intensity);

  framebuffer[idx] = {
    (unsigned char)(r > 255 ? 255 : r),
    (unsigned char)(g > 255 ? 255 : g),
    (unsigned char)(b > 255 ? 255 : b),
    orig.w  // preserve alpha
  };
}

// BloomPostProcess implementation

BloomPostProcess::~BloomPostProcess() {
  if (brightBuf_) cudaFree(brightBuf_);
  if (scratchBuf_) cudaFree(scratchBuf_);
}

bool BloomPostProcess::init(int width, int height) {
  width_ = width;
  height_ = height;
  size_t bytes = width * height * sizeof(uchar4);

  if (cudaMalloc(&brightBuf_, bytes) != cudaSuccess) {
    std::fprintf(stderr, "Bloom: failed to allocate bright buffer\n");
    return false;
  }
  if (cudaMalloc(&scratchBuf_, bytes) != cudaSuccess) {
    std::fprintf(stderr, "Bloom: failed to allocate scratch buffer\n");
    cudaFree(brightBuf_);
    brightBuf_ = nullptr;
    return false;
  }

  return true;
}

void BloomPostProcess::apply(uchar4* framebuffer, const BloomParams& params) {
  if (!params.enabled || !brightBuf_ || !scratchBuf_) return;

  dim3 block(16, 16);
  dim3 grid((width_ + block.x - 1) / block.x,
            (height_ + block.y - 1) / block.y);

  // 1. Extract bright pixels
  bloomThresholdKernel<<<grid, block>>>(framebuffer, brightBuf_,
                                        width_, height_, params.threshold);

  // 2. Horizontal blur: bright → scratch
  bloomBlurHKernel<<<grid, block>>>(brightBuf_, scratchBuf_,
                                    width_, height_,
                                    params.radius, params.sigma);

  // 3. Vertical blur: scratch → bright
  bloomBlurVKernel<<<grid, block>>>(scratchBuf_, brightBuf_,
                                    width_, height_,
                                    params.radius, params.sigma);

  // 4. Composite bloom onto original
  bloomCompositeKernel<<<grid, block>>>(framebuffer, brightBuf_,
                                        width_, height_, params.intensity);

  cudaDeviceSynchronize();
}

} // namespace bht::gpu
