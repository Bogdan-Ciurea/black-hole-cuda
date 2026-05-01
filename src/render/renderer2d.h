#pragma once

#include "physics/blackhole.h"
#include "ray/ray.h"
#include "render/fullscreen_quad.h"
#include "render/shader.h"

#include <glad/gl.h>

#include <cstring>
#include <vector>

namespace bht::render {

class Renderer2D {
public:
  Renderer2D() = default;
  ~Renderer2D();

  bool init(int width, int height);
  void render(const std::vector<bht::ray::Ray>& rays, const bht::physics::BlackHole& bh,
              double viewScale);
  void display();

private:
  int width_ = 0;
  int height_ = 0;

  // CPU pixel buffer (RGBA, 4 bytes per pixel)
  std::vector<uint8_t> pixels_;

  // OpenGL texture for displaying the pixel buffer
  GLuint texture_ = 0;
  FullscreenQuad quad_;
  Shader displayShader_;

  void clearBuffer();
  void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
  void drawCircle(int cx, int cy, int radius, uint8_t r, uint8_t g, uint8_t b);
  void drawTrail(const std::vector<glm::dvec3>& trail, double viewScale,
                 uint8_t r, uint8_t g, uint8_t b);
  void worldToScreen(double wx, double wy, double viewScale, int& sx, int& sy);
  void uploadTexture();
};

} // namespace bht::render
