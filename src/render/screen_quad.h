#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <vector>

namespace bht::render {

class ScreenQuad {
public:
  ScreenQuad() = default;
  ~ScreenQuad();

  bool init(int width, int height);
  void upload(const std::vector<glm::vec3>& pixels);
  void draw() const;

  int width() const { return width_; }
  int height() const { return height_; }

private:
  int width_ = 0;
  int height_ = 0;
  GLuint texture_ = 0;
  GLuint vao_ = 0;
  GLuint vbo_ = 0;
  GLuint shader_ = 0;
};

} // namespace bht::render
