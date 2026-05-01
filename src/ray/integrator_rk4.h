#pragma once

#include "physics/geodesic.h"
#include "physics/blackhole.h"
#include "ray/ray.h"

namespace bht::ray {

// State vector for RK4: (r, phi, dr, dphi)
struct RayState4 {
  double r, phi, dr, dphi;
};

// Derivative of state: (dr, dphi, d2r, d2phi)
struct RayDeriv4 {
  double dr, dphi, d2r, d2phi;
};

inline RayDeriv4 computeDerivatives(const RayState4& s, double Rs) {
  auto acc = bht::physics::computeGeodesicAcceleration(s.r, s.dr, s.dphi, Rs);
  return {s.dr, s.dphi, acc.d2r, acc.d2phi};
}

inline RayState4 addScaled(const RayState4& s, const RayDeriv4& d, double scale) {
  return {
      s.r + d.dr * scale,
      s.phi + d.dphi * scale,
      s.dr + d.d2r * scale,
      s.dphi + d.d2phi * scale,
  };
}

inline void stepRK4(Ray& ray, double dt, const bht::physics::BlackHole& bh) {
  double Rs = bh.schwarzschildRadius;
  RayState4 s{ray.r, ray.phi, ray.dr, ray.dphi};

  // k1 = f(s)
  RayDeriv4 k1 = computeDerivatives(s, Rs);

  // k2 = f(s + k1 * dt/2)
  RayState4 s2 = addScaled(s, k1, dt * 0.5);
  RayDeriv4 k2 = computeDerivatives(s2, Rs);

  // k3 = f(s + k2 * dt/2)
  RayState4 s3 = addScaled(s, k2, dt * 0.5);
  RayDeriv4 k3 = computeDerivatives(s3, Rs);

  // k4 = f(s + k3 * dt)
  RayState4 s4 = addScaled(s, k3, dt);
  RayDeriv4 k4 = computeDerivatives(s4, Rs);

  // Combine: y_next = y + (k1 + 2*k2 + 2*k3 + k4) * dt/6
  double dt6 = dt / 6.0;
  ray.r += (k1.dr + 2.0 * k2.dr + 2.0 * k3.dr + k4.dr) * dt6;
  ray.phi += (k1.dphi + 2.0 * k2.dphi + 2.0 * k3.dphi + k4.dphi) * dt6;
  ray.dr += (k1.d2r + 2.0 * k2.d2r + 2.0 * k3.d2r + k4.d2r) * dt6;
  ray.dphi += (k1.d2phi + 2.0 * k2.d2phi + 2.0 * k3.d2phi + k4.d2phi) * dt6;

  ray.updateCartesianFromPolar();
}

} // namespace bht::ray
