#include "render/shader.h"

#include <cstdio>
#include <vector>

namespace bht::render {

Shader::~Shader() {
  if (program_)
    glDeleteProgram(program_);
}

bool Shader::compile(const std::string& vertexSrc, const std::string& fragmentSrc) {
  GLuint vert = compileStage(GL_VERTEX_SHADER, vertexSrc);
  if (!vert) return false;

  GLuint frag = compileStage(GL_FRAGMENT_SHADER, fragmentSrc);
  if (!frag) {
    glDeleteShader(vert);
    return false;
  }

  program_ = glCreateProgram();
  glAttachShader(program_, vert);
  glAttachShader(program_, frag);
  glLinkProgram(program_);

  GLint success;
  glGetProgramiv(program_, GL_LINK_STATUS, &success);
  if (!success) {
    char log[512];
    glGetProgramInfoLog(program_, 512, nullptr, log);
    std::fprintf(stderr, "Shader link error: %s\n", log);
    glDeleteProgram(program_);
    program_ = 0;
  }

  glDeleteShader(vert);
  glDeleteShader(frag);
  return program_ != 0;
}

void Shader::use() const {
  glUseProgram(program_);
}

void Shader::setFloat(const char* name, float value) const {
  glUniform1f(glGetUniformLocation(program_, name), value);
}

void Shader::setVec2(const char* name, float x, float y) const {
  glUniform2f(glGetUniformLocation(program_, name), x, y);
}

GLuint Shader::compileStage(GLenum type, const std::string& source) {
  GLuint shader = glCreateShader(type);
  const char* src = source.c_str();
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char log[512];
    glGetShaderInfoLog(shader, 512, nullptr, log);
    std::fprintf(stderr, "Shader compile error: %s\n", log);
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}

} // namespace bht::render
