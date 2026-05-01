#pragma once

#include "math/constants.h"

namespace bht::physics {

struct GeodesicAcceleration {
  double d2r;
  double d2phi;
};

// Compute radial and angular acceleration for a photon
// in the Schwarzschild metric (equatorial plane).
//
// d2r   = (c² * Rs) / (2 * r²) + r * dphi²
// d2phi = -(2 / r) * dr * dphi
BHT_HOST_DEVICE inline GeodesicAcceleration computeGeodesicAcceleration(
    double r, double dr, double dphi, double Rs) {
  double d2r = (bht::math::C_SQUARED * Rs) / (2.0 * r * r) + r * dphi * dphi;
  double d2phi = -(2.0 / r) * dr * dphi;
  return {d2r, d2phi};
}

} // namespace bht::physics
