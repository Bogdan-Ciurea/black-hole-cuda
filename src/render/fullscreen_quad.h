#pragma once

#include <glad/gl.h>

namespace bht::render {

class FullscreenQuad {
public:
  FullscreenQuad() = default;
  ~FullscreenQuad();

  void init();
  void draw() const;

private:
  GLuint vao_ = 0;
  GLuint vbo_ = 0;
};

} // namespace bht::render
