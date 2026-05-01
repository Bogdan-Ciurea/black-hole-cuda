#include "gpu/cuda_gl_interop.h"

#include <cuda_gl_interop.h>
#include <cstdio>

namespace bht::gpu {

static const char* vertSrc = R"(
#version 460 core
out vec2 uv;
void main() {
  uv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
  gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
}
)";

static const char* fragSrc = R"(
#version 460 core
in vec2 uv;
out vec4 fragColor;
uniform sampler2D screenTex;
void main() {
  fragColor = texture(screenTex, uv);
}
)";

static GLuint compileShader(GLenum type, const char* src) {
  GLuint s = glCreateShader(type);
  glShaderSource(s, 1, &src, nullptr);
  glCompileShader(s);
  GLint ok;
  glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[512];
    glGetShaderInfoLog(s, 512, nullptr, log);
    std::fprintf(stderr, "Shader error: %s\n", log);
  }
  return s;
}

CudaGLInterop::~CudaGLInterop() {
  if (cudaResource_) cudaGraphicsUnregisterResource(cudaResource_);
  if (pbo_) glDeleteBuffers(1, &pbo_);
  if (texture_) glDeleteTextures(1, &texture_);
  if (vao_) glDeleteVertexArrays(1, &vao_);
  if (shader_) glDeleteProgram(shader_);
}

bool CudaGLInterop::init(int width, int height) {
  width_ = width;
  height_ = height;

  // Create PBO (Pixel Buffer Object)
  glGenBuffers(1, &pbo_);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo_);
  glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * 4, nullptr, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

  // Register PBO with CUDA
  cudaError_t err = cudaGraphicsGLRegisterBuffer(
      &cudaResource_, pbo_, cudaGraphicsMapFlagsWriteDiscard);
  if (err != cudaSuccess) {
    std::fprintf(stderr, "CUDA GL register PBO failed: %s\n", cudaGetErrorString(err));
    return false;
  }

  // Create GL texture for display
  glGenTextures(1, &texture_);
  glBindTexture(GL_TEXTURE_2D, texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Empty VAO for fullscreen triangle
  glGenVertexArrays(1, &vao_);

  // Compile shader
  GLuint vs = compileShader(GL_VERTEX_SHADER, vertSrc);
  GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragSrc);
  shader_ = glCreateProgram();
  glAttachShader(shader_, vs);
  glAttachShader(shader_, fs);
  glLinkProgram(shader_);

  GLint ok;
  glGetProgramiv(shader_, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[512];
    glGetProgramInfoLog(shader_, 512, nullptr, log);
    std::fprintf(stderr, "Program link error: %s\n", log);
    return false;
  }

  glDeleteShader(vs);
  glDeleteShader(fs);

  return true;
}

uchar4* CudaGLInterop::map() {
  cudaGraphicsMapResources(1, &cudaResource_, 0);

  uchar4* devPtr = nullptr;
  size_t size = 0;
  cudaGraphicsResourceGetMappedPointer((void**)&devPtr, &size, cudaResource_);

  return devPtr;
}

void CudaGLInterop::unmap() {
  cudaGraphicsUnmapResources(1, &cudaResource_, 0);

  // Copy PBO → texture
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo_);
  glBindTexture(GL_TEXTURE_2D, texture_);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

void CudaGLInterop::draw() const {
  glUseProgram(shader_);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_);
  glBindVertexArray(vao_);
  glDrawArrays(GL_TRIANGLES, 0, 3);
}

} // namespace bht::gpu
