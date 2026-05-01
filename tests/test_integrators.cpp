#include "ray/integrator_euler.h"
#include "ray/integrator_rk4.h"
#include "physics/blackhole.h"

#include <gtest/gtest.h>

#include <cmath>

using namespace bht::ray;
using namespace bht::physics;

// Use geometric units for simpler test values
static BlackHole makeTestBH() {
  // Mass chosen so Rs = 2.0 in geometric-ish units
  // Rs = 2GM/c² → M = Rs*c²/(2G)
  double Rs = 2.0;
  double mass = Rs * bht::math::C_SQUARED / (2.0 * bht::math::G);
  return BlackHole(mass);
}

TEST(Integrator, Euler_StraightLine_NoBlackHole) {
  // A ray moving radially outward with no angular velocity
  // should stay on a straight line (phi unchanged)
  BlackHole bh(0.0); // zero mass = no gravity
  // Need to avoid division by zero in geodesic with r=0, use large r
  Ray ray(1000.0, 0.0, 1.0, 0.0);
  double initialPhi = ray.phi;

  for (int i = 0; i < 100; i++)
    stepEuler(ray, 0.01, bh);

  EXPECT_NEAR(ray.phi, initialPhi, 1e-10);
  EXPECT_GT(ray.r, 1000.0); // moved outward
}

TEST(Integrator, RK4_StraightLine_NoBlackHole) {
  BlackHole bh(0.0);
  Ray ray(1000.0, 0.0, 1.0, 0.0);
  double initialPhi = ray.phi;

  for (int i = 0; i < 100; i++)
    stepRK4(ray, 0.01, bh);

  EXPECT_NEAR(ray.phi, initialPhi, 1e-10);
  EXPECT_GT(ray.r, 1000.0);
}

TEST(Integrator, RK4_MoreAccurateThanEuler) {
  // Both integrators should conserve angular momentum L = r²*dphi
  // RK4 should drift less than Euler over many steps
  // Use geometric units (G=1, c=1) with Rs=2 for simplicity
  BlackHole bh = makeTestBH();
  double Rs = bh.schwarzschildRadius;
  double r0 = 10.0 * Rs;

  // Set tangential velocity for a mildly bound orbit
  // dphi such that L = r0² * dphi gives a reasonable orbit
  double dphi0 = bht::math::C / (r0 * 2.0); // sub-orbital

  double L0 = r0 * r0 * dphi0;

  Ray rayEuler(r0, 0.0, 0.0, dphi0);
  Ray rayRK4(r0, 0.0, 0.0, dphi0);

  // dt must be small relative to the orbital timescale
  double dt = Rs / (bht::math::C * 100.0);
  int steps = 500;

  for (int i = 0; i < steps; i++) {
    stepEuler(rayEuler, dt, bh);
    if (rayEuler.r < Rs) break;
  }
  for (int i = 0; i < steps; i++) {
    stepRK4(rayRK4, dt, bh);
    if (rayRK4.r < Rs) break;
  }

  // Only compare if both rays survived
  if (rayRK4.r > Rs && rayEuler.r > Rs) {
    double L_euler = rayEuler.r * rayEuler.r * rayEuler.dphi;
    double L_rk4 = rayRK4.r * rayRK4.r * rayRK4.dphi;

    double drift_euler = std::abs(L_euler - L0) / std::abs(L0);
    double drift_rk4 = std::abs(L_rk4 - L0) / std::abs(L0);

    EXPECT_LT(drift_rk4, drift_euler);
  }
}

TEST(Integrator, RK4_RayDeflectedByBlackHole) {
  // A ray passing near the black hole should be deflected (phi changes)
  BlackHole bh = makeTestBH();
  double Rs = bh.schwarzschildRadius;
  double r0 = 5.0 * Rs;
  double dphi0 = bht::math::C / (r0 * 3.0);

  Ray ray(r0, 0.0, 0.0, dphi0);
  double initialPhi = ray.phi;

  double dt = Rs / (bht::math::C * 100.0);
  for (int i = 0; i < 1000; i++) {
    stepRK4(ray, dt, bh);
    if (ray.r < Rs || ray.r > 100.0 * Rs) break;
  }

  // Phi should have changed significantly due to gravitational interaction
  EXPECT_GT(std::abs(ray.phi - initialPhi), 0.01);
}

TEST(Integrator, Euler_CartesianUpdated) {
  // After stepping, cartesian position should match polar
  BlackHole bh = makeTestBH();
  Ray ray(5.0 * bh.schwarzschildRadius, 0.5, 0.0, 0.01);

  stepEuler(ray, 0.01, bh);

  double expectedX = ray.r * std::cos(ray.phi);
  double expectedY = ray.r * std::sin(ray.phi);
  EXPECT_NEAR(ray.position.x, expectedX, 1e-10);
  EXPECT_NEAR(ray.position.y, expectedY, 1e-10);
}
