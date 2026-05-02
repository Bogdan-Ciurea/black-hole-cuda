#pragma once

#include "math/constants.h"

#include <glm/glm.hpp>

namespace bht::physics {

struct BlackHole {
  glm::dvec3 position{0.0, 0.0, 0.0};
  double mass;
  double schwarzschildRadius;
  bool drawGeodesicGrid = false;

  explicit BlackHole(double mass, glm::dvec3 pos = {0.0, 0.0, 0.0})
      : position(pos), mass(mass), schwarzschildRadius(computeSchwarzschildRadius(mass)) {}

  BHT_HOST_DEVICE static double computeSchwarzschildRadius(double m) {
    return (2.0 * bht::math::G * m) / bht::math::C_SQUARED;
  }

  BHT_HOST_DEVICE bool isInsideHorizon(double r) const {
    return r < schwarzschildRadius;
  }

  BHT_HOST_DEVICE double photonSphereRadius() const {
    return 1.5 * schwarzschildRadius;
  }
};

} // namespace bht::physics
