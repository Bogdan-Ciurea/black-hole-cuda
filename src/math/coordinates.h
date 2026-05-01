#pragma once

#include "math/constants.h"

#include <glm/glm.hpp>

#include <cmath>

namespace bht::math {

struct Polar {
  double r;
  double theta; // azimuth angle in XY plane [0, 2π)
  double phi;   // inclination from XY plane [-π/2, π/2]
};

BHT_HOST_DEVICE inline Polar cartesianToPolar(const glm::dvec3& pos) {
  double r = glm::length(pos);
  if (r < 1e-15)
    return {0.0, 0.0, 0.0};

  double theta = std::atan2(pos.y, pos.x);
  if (theta < 0.0)
    theta += TWO_PI;

  double phi = std::asin(glm::clamp(pos.z / r, -1.0, 1.0));

  return {r, theta, phi};
}

BHT_HOST_DEVICE inline glm::dvec3 polarToCartesian(double r, double theta, double phi) {
  double cosPhi = std::cos(phi);
  return {
      r * cosPhi * std::cos(theta),
      r * cosPhi * std::sin(theta),
      r * std::sin(phi),
  };
}

BHT_HOST_DEVICE inline double normalizeAngle(double angle) {
  angle = std::fmod(angle, TWO_PI);
  if (angle < 0.0)
    angle += TWO_PI;
  return angle;
}

} // namespace bht::math
