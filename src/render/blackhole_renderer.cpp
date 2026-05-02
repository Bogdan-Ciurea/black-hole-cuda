#include "render/blackhole_renderer.h"

#include <cuda_runtime.h>

#include <cmath>
#include <cstdio>
#include <vector>

namespace bht::render {

// --- Shaders ---

static const char* gridVertSrc = R"(
#version 460 core
layout(location = 0) in vec3 aPos;
uniform mat4 mvp;
out float vDist;
void main() {
  gl_Position = mvp * vec4(aPos, 1.0);
  vDist = length(aPos.xz);
}
)";

static const char* gridFragSrc = R"(
#version 460 core
in float vDist;
out vec4 fragColor;
uniform float Rs;
void main() {
  if (vDist < Rs) discard;
  float t = clamp(vDist / (Rs * 15.0), 0.0, 1.0);
  float brightness = mix(0.8, 0.3, t);
  fragColor = vec4(vec3(brightness), 1.0);
}
)";

static const char* sphereVertSrc = R"(
#version 460 core
layout(location = 0) in vec3 aPos;
uniform mat4 mvp;
void main() {
  gl_Position = mvp * vec4(aPos, 1.0);
}
)";

static const char* sphereFragSrc = R"(
#version 460 core
out vec4 fragColor;
void main() {
  fragColor = vec4(0.8, 0.0, 0.0, 1.0);
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

static GLuint linkProgram(const char* vSrc, const char* fSrc, GLint& mvpLoc) {
  GLuint vs = compileShader(GL_VERTEX_SHADER, vSrc);
  GLuint fs = compileShader(GL_FRAGMENT_SHADER, fSrc);
  GLuint prog = glCreateProgram();
  glAttachShader(prog, vs);
  glAttachShader(prog, fs);
  glLinkProgram(prog);
  glDeleteShader(vs);
  glDeleteShader(fs);

  GLint ok;
  glGetProgramiv(prog, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[512];
    glGetProgramInfoLog(prog, 512, nullptr, log);
    std::fprintf(stderr, "Program link error: %s\n", log);
  }
  mvpLoc = glGetUniformLocation(prog, "mvp");
  return prog;
}

// --- Destructor ---

BlackHoleRenderer::~BlackHoleRenderer() {
  if (gridVao_) glDeleteVertexArrays(1, &gridVao_);
  if (gridVbo_) glDeleteBuffers(1, &gridVbo_);
  if (gridEbo_) glDeleteBuffers(1, &gridEbo_);
  if (gridShader_) glDeleteProgram(gridShader_);
  if (sphereVao_) glDeleteVertexArrays(1, &sphereVao_);
  if (sphereVbo_) glDeleteBuffers(1, &sphereVbo_);
  if (sphereEbo_) glDeleteBuffers(1, &sphereEbo_);
  if (sphereShader_) glDeleteProgram(sphereShader_);
}

// --- Init ---

bool BlackHoleRenderer::init(int gridResolution, float gridExtent) {
  rs_ = static_cast<float>(bh_.schwarzschildRadius);
  return initGrid(gridResolution, gridExtent) && initSphere();
}

bool BlackHoleRenderer::initGrid(int resolution, float extent) {
  bht::gpu::GridParams gp;
  gp.extent = extent;
  gp.resolution = resolution;
  gp.Rs = rs_;

  int total = gp.resolution * gp.resolution;

  float3* d_grid = nullptr;
  cudaMalloc(&d_grid, total * sizeof(float3));
  bht::gpu::launchGridDeformKernel(d_grid, gp);

  std::vector<float3> h_grid(total);
  cudaMemcpy(h_grid.data(), d_grid, total * sizeof(float3), cudaMemcpyDeviceToHost);
  cudaFree(d_grid);

  // Build line indices
  std::vector<unsigned int> indices;
  int res = gp.resolution;
  for (int z = 0; z < res; z++)
    for (int x = 0; x < res - 1; x++) {
      indices.push_back(z * res + x);
      indices.push_back(z * res + x + 1);
    }
  for (int x = 0; x < res; x++)
    for (int z = 0; z < res - 1; z++) {
      indices.push_back(z * res + x);
      indices.push_back((z + 1) * res + x);
    }
  gridIndexCount_ = static_cast<int>(indices.size());

  glGenVertexArrays(1, &gridVao_);
  glGenBuffers(1, &gridVbo_);
  glGenBuffers(1, &gridEbo_);

  glBindVertexArray(gridVao_);
  glBindBuffer(GL_ARRAY_BUFFER, gridVbo_);
  glBufferData(GL_ARRAY_BUFFER, total * sizeof(float3), h_grid.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float3), nullptr);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gridEbo_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

  gridShader_ = linkProgram(gridVertSrc, gridFragSrc, gridMvpLoc_);
  gridRsLoc_ = glGetUniformLocation(gridShader_, "Rs");

  return true;
}

bool BlackHoleRenderer::initSphere() {
  // Generate UV sphere at origin with radius = Rs
  const int stacks = 20;
  const int slices = 30;

  std::vector<float> verts;
  std::vector<unsigned int> indices;

  for (int i = 0; i <= stacks; i++) {
    float phi = 3.14159265f * i / stacks;
    float y = rs_ * cosf(phi);
    float ringR = rs_ * sinf(phi);

    for (int j = 0; j <= slices; j++) {
      float theta = 2.0f * 3.14159265f * j / slices;
      float x = ringR * cosf(theta);
      float z = ringR * sinf(theta);
      verts.push_back(x);
      verts.push_back(y);
      verts.push_back(z);
    }
  }

  for (int i = 0; i < stacks; i++) {
    for (int j = 0; j < slices; j++) {
      int a = i * (slices + 1) + j;
      int b = a + slices + 1;
      indices.push_back(a);
      indices.push_back(b);
      indices.push_back(a + 1);
      indices.push_back(b);
      indices.push_back(b + 1);
      indices.push_back(a + 1);
    }
  }
  sphereIndexCount_ = static_cast<int>(indices.size());

  glGenVertexArrays(1, &sphereVao_);
  glGenBuffers(1, &sphereVbo_);
  glGenBuffers(1, &sphereEbo_);

  glBindVertexArray(sphereVao_);
  glBindBuffer(GL_ARRAY_BUFFER, sphereVbo_);
  glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEbo_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

  sphereShader_ = linkProgram(sphereVertSrc, sphereFragSrc, sphereMvpLoc_);

  return true;
}

// --- Render ---

void BlackHoleRenderer::render(const float* mvp) const {
  renderEventHorizon(mvp);
  if (bh_.drawGeodesicGrid)
    renderSpacetimeGrid(mvp);
}

void BlackHoleRenderer::renderSpacetimeGrid(const float* mvp) const {
  glUseProgram(gridShader_);
  glUniformMatrix4fv(gridMvpLoc_, 1, GL_FALSE, mvp);
  glUniform1f(gridRsLoc_, rs_);

  glBindVertexArray(gridVao_);
  glDrawElements(GL_LINES, gridIndexCount_, GL_UNSIGNED_INT, nullptr);
}

void BlackHoleRenderer::renderEventHorizon(const float* mvp) const {
  glUseProgram(sphereShader_);
  glUniformMatrix4fv(sphereMvpLoc_, 1, GL_FALSE, mvp);

  glBindVertexArray(sphereVao_);
  glDrawElements(GL_TRIANGLES, sphereIndexCount_, GL_UNSIGNED_INT, nullptr);
}

} // namespace bht::render
