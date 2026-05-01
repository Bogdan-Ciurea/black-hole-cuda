#pragma once

#include "physics/geodesic.h"
#include "physics/blackhole.h"
#include "ray/ray.h"

namespace bht::ray {

inline void stepEuler(Ray& ray, double dt, const bht::physics::BlackHole& bh) {
  // Compute accelerations from geodesic equations
  auto acc = bht::physics::computeGeodesicAcceleration(
      ray.r, ray.dr, ray.dphi, bh.schwarzschildRadius);

  // Update velocities
  ray.dr += acc.d2r * dt;
  ray.dphi += acc.d2phi * dt;

  // Update positions
  ray.r += ray.dr * dt;
  ray.phi += ray.dphi * dt;

  // Sync cartesian representation
  ray.updateCartesianFromPolar();
}

} // namespace bht::ray
