#include "render/renderer2d.h"

#include <cmath>

namespace bht::render {

static const char* displayVert = R"(
#version 460 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
out vec2 uv;
void main() {
  uv = aUV;
  gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

static const char* displayFrag = R"(
#version 460 core
in vec2 uv;
out vec4 fragColor;
uniform sampler2D uTexture;
void main() {
  fragColor = texture(uTexture, uv);
}
)";

Renderer2D::~Renderer2D() {
  if (texture_)
    glDeleteTextures(1, &texture_);
}

bool Renderer2D::init(int width, int height) {
  width_ = width;
  height_ = height;
  pixels_.resize(width * height * 4, 0);

  // Create texture
  glGenTextures(1, &texture_);
  glBindTexture(GL_TEXTURE_2D, texture_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

  if (!displayShader_.compile(displayVert, displayFrag))
    return false;

  quad_.init();
  return true;
}

void Renderer2D::render(const std::vector<bht::ray::Ray>& rays,
                        const bht::physics::BlackHole& bh, double viewScale) {
  clearBuffer();

  // Draw black hole as a filled circle
  int cx, cy;
  worldToScreen(bh.position.x, bh.position.y, viewScale, cx, cy);
  int bhRadius = static_cast<int>(bh.schwarzschildRadius / viewScale * (width_ / 2.0));
  if (bhRadius < 3) bhRadius = 3;
  drawCircle(cx, cy, bhRadius, 20, 20, 30);

  // Draw photon sphere as a ring
  int psRadius = static_cast<int>(bh.photonSphereRadius() / viewScale * (width_ / 2.0));
  // Draw thin ring (just outline)
  for (int angle = 0; angle < 360; angle++) {
    double rad = angle * bht::math::PI / 180.0;
    int px = cx + static_cast<int>(psRadius * std::cos(rad));
    int py = cy + static_cast<int>(psRadius * std::sin(rad));
    setPixel(px, py, 60, 60, 80);
  }

  // Draw ray trails
  static const uint8_t colors[][3] = {
      {255, 100, 100}, {100, 255, 100}, {100, 100, 255},
      {255, 255, 100}, {255, 100, 255}, {100, 255, 255},
      {255, 200, 50},  {200, 50, 255},
  };
  int numColors = sizeof(colors) / sizeof(colors[0]);

  for (size_t i = 0; i < rays.size(); i++) {
    int ci = i % numColors;
    drawTrail(rays[i].trail, viewScale, colors[ci][0], colors[ci][1], colors[ci][2]);
  }

  uploadTexture();
}

void Renderer2D::display() {
  displayShader_.use();
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_);
  quad_.draw();
}

void Renderer2D::clearBuffer() {
  std::memset(pixels_.data(), 10, pixels_.size()); // dark background
  // Set alpha to 255
  for (size_t i = 3; i < pixels_.size(); i += 4)
    pixels_[i] = 255;
}

void Renderer2D::setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
  if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
  size_t idx = (y * width_ + x) * 4;
  pixels_[idx + 0] = r;
  pixels_[idx + 1] = g;
  pixels_[idx + 2] = b;
  pixels_[idx + 3] = 255;
}

void Renderer2D::drawCircle(int cx, int cy, int radius, uint8_t r, uint8_t g, uint8_t b) {
  for (int dy = -radius; dy <= radius; dy++) {
    for (int dx = -radius; dx <= radius; dx++) {
      if (dx * dx + dy * dy <= radius * radius)
        setPixel(cx + dx, cy + dy, r, g, b);
    }
  }
}

void Renderer2D::drawTrail(const std::vector<glm::dvec3>& trail, double viewScale,
                           uint8_t r, uint8_t g, uint8_t b) {
  for (const auto& pos : trail) {
    int sx, sy;
    worldToScreen(pos.x, pos.y, viewScale, sx, sy);
    setPixel(sx, sy, r, g, b);
    // Thicken the trail a bit
    setPixel(sx + 1, sy, r, g, b);
    setPixel(sx, sy + 1, r, g, b);
  }
}

void Renderer2D::worldToScreen(double wx, double wy, double viewScale, int& sx, int& sy) {
  // Map world coordinates to screen: center of screen = origin
  sx = static_cast<int>((wx / viewScale + 1.0) * 0.5 * width_);
  sy = static_cast<int>((wy / viewScale + 1.0) * 0.5 * height_);
}

void Renderer2D::uploadTexture() {
  glBindTexture(GL_TEXTURE_2D, texture_);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE,
                  pixels_.data());
}

} // namespace bht::render
