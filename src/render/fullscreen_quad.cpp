#include "render/fullscreen_quad.h"

namespace bht::render {

// Fullscreen triangle (covers entire screen with one triangle, no clipping needed)
static const float vertices[] = {
    // positions   // uvs
    -1.0f, -1.0f,  0.0f, 0.0f,
     3.0f, -1.0f,  2.0f, 0.0f,
    -1.0f,  3.0f,  0.0f, 2.0f,
};

FullscreenQuad::~FullscreenQuad() {
  if (vao_) glDeleteVertexArrays(1, &vao_);
  if (vbo_) glDeleteBuffers(1, &vbo_);
}

void FullscreenQuad::init() {
  glGenVertexArrays(1, &vao_);
  glGenBuffers(1, &vbo_);

  glBindVertexArray(vao_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // position attribute (location 0)
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  // uv attribute (location 1)
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
}

void FullscreenQuad::draw() const {
  glBindVertexArray(vao_);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glBindVertexArray(0);
}

} // namespace bht::render
