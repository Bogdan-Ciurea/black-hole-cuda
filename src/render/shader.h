#pragma once

#include <glad/gl.h>

#include <string>

namespace bht::render {

class Shader {
public:
  Shader() = default;
  ~Shader();

  bool compile(const std::string& vertexSrc, const std::string& fragmentSrc);
  void use() const;
  GLuint id() const { return program_; }

  void setFloat(const char* name, float value) const;
  void setVec2(const char* name, float x, float y) const;

private:
  GLuint program_ = 0;

  GLuint compileStage(GLenum type, const std::string& source);
};

} // namespace bht::render
