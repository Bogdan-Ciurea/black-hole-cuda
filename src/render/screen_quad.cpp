#include "render/screen_quad.h"

#include <cstdio>

namespace bht::render {

static const char* vertSrc = R"(
#version 460 core
out vec2 uv;
void main() {
  // Fullscreen triangle trick
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

ScreenQuad::~ScreenQuad() {
  if (texture_) glDeleteTextures(1, &texture_);
  if (vao_) glDeleteVertexArrays(1, &vao_);
  if (vbo_) glDeleteBuffers(1, &vbo_);
  if (shader_) glDeleteProgram(shader_);
}

bool ScreenQuad::init(int width, int height) {
  width_ = width;
  height_ = height;

  // Create texture
  glGenTextures(1, &texture_);
  glBindTexture(GL_TEXTURE_2D, texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
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

void ScreenQuad::upload(const std::vector<glm::vec3>& pixels) {
  if (static_cast<int>(pixels.size()) != width_ * height_) return;

  glBindTexture(GL_TEXTURE_2D, texture_);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, GL_RGB, GL_FLOAT, pixels.data());
}

void ScreenQuad::draw() const {
  glUseProgram(shader_);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_);
  glBindVertexArray(vao_);
  glDrawArrays(GL_TRIANGLES, 0, 3);
}

} // namespace bht::render
