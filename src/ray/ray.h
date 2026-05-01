#pragma once

#include "math/constants.h"

#include <glm/glm.hpp>

#include <cmath>
#include <vector>

namespace bht::ray {

struct Ray {
  // Polar state
  double r;
  double phi;
  double dr;
  double dphi;

  // Cartesian position (derived from polar)
  glm::dvec3 position{0.0, 0.0, 0.0};

  // Trail of positions for visualization
  std::vector<glm::dvec3> trail;

  Ray(double r, double phi, double dr, double dphi)
      : r(r), phi(phi), dr(dr), dphi(dphi) {
    updateCartesianFromPolar();
  }

  void updatePolarFromCartesian() {
    r = glm::length(glm::dvec2(position.x, position.y));
    if (r < 1e-15) {
      phi = 0.0;
    } else {
      phi = std::atan2(position.y, position.x);
      if (phi < 0.0)
        phi += bht::math::TWO_PI;
    }
  }

  void updateCartesianFromPolar() {
    position.x = r * std::cos(phi);
    position.y = r * std::sin(phi);
    position.z = 0.0;
  }

  void addToTrail() {
    trail.push_back(position);
  }
};

} // namespace bht::ray
