#pragma once

#include "math/constants.h"

#include <glm/glm.hpp>

#include <cmath>

namespace bht::scene {

struct Camera {
  glm::dvec3 position{0.0, 0.0, -10.0};
  glm::dvec3 forward{0.0, 0.0, 1.0};
  glm::dvec3 right{1.0, 0.0, 0.0};
  glm::dvec3 up{0.0, 1.0, 0.0};
  double fov = 60.0; // degrees

  Camera() = default;

  Camera(glm::dvec3 pos, glm::dvec3 lookAt, glm::dvec3 worldUp = {0.0, 1.0, 0.0}, double fovDeg = 60.0)
      : position(pos), fov(fovDeg) {
    forward = glm::normalize(lookAt - pos);
    right = glm::normalize(glm::cross(forward, worldUp));
    up = glm::cross(right, forward);
  }

  // Generate a ray direction for pixel (x, y) in an image of (width x height)
  // Returns normalized direction vector
  glm::dvec3 generateRayDir(double x, double y, int width, int height) const {
    double aspectRatio = static_cast<double>(width) / height;
    double tanHalfFov = std::tan(fov * bht::math::PI / 360.0);

    // Map pixel to [-1, 1] range (center of pixel)
    double ndcX = (2.0 * (x + 0.5) / width - 1.0) * aspectRatio * tanHalfFov;
    double ndcY = (1.0 - 2.0 * (y + 0.5) / height) * tanHalfFov;

    glm::dvec3 dir = forward + ndcX * right + ndcY * up;
    return glm::normalize(dir);
  }
};

} // namespace bht::scene
