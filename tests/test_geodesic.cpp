#include "physics/geodesic.h"
#include "physics/blackhole.h"

#include <gtest/gtest.h>

#include <cmath>

using namespace bht::physics;
using namespace bht::math;

TEST(Geodesic, ZeroVelocity_OnlyGravity) {
  // A photon at rest radially (dr=0, dphi=0) should have no angular acceleration
  // and only gravitational radial acceleration (negative = inward)
  double Rs = 2953.0; // ~solar mass
  double r = 10000.0;
  auto acc = computeGeodesicAcceleration(r, 0.0, 0.0, Rs);

  double expected_d2r = -(C_SQUARED * Rs) / (2.0 * r * r);
  EXPECT_NEAR(acc.d2r, expected_d2r, 1e-3);
  EXPECT_LT(acc.d2r, 0.0); // gravity pulls inward
  EXPECT_NEAR(acc.d2phi, 0.0, 1e-15);
}

TEST(Geodesic, AngularAcceleration_NoRadialVelocity) {
  // With dr=0, angular acceleration should be zero regardless of dphi
  double Rs = 2953.0;
  double r = 5000.0;
  double dphi = 0.01;
  auto acc = computeGeodesicAcceleration(r, 0.0, dphi, Rs);

  EXPECT_NEAR(acc.d2phi, 0.0, 1e-15);
}

TEST(Geodesic, AngularAcceleration_WithRadialVelocity) {
  // d2phi = -(2/r) * dr * dphi
  double Rs = 2953.0;
  double r = 5000.0;
  double dr = 100.0;
  double dphi = 0.01;
  auto acc = computeGeodesicAcceleration(r, dr, dphi, Rs);

  double expected_d2phi = -(2.0 / r) * dr * dphi;
  EXPECT_NEAR(acc.d2phi, expected_d2phi, 1e-15);
}

TEST(Geodesic, PhotonSphere_CircularOrbit) {
  // At the photon sphere (r = 1.5*Rs), a photon with the right tangential
  // velocity should have near-zero net radial acceleration.
  // For circular orbit: r*dphi² = gravitational term
  // => dphi² = (c²*Rs) / (2*r³)  at r=1.5*Rs
  BlackHole bh(1.989e30);
  double r = bh.photonSphereRadius();
  double Rs = bh.schwarzschildRadius;

  // Circular orbit condition: centrifugal = gravity
  // r*dphi² = (c²*Rs)/(2r²)  =>  dphi² = (c²*Rs)/(2r³)
  double dphi = std::sqrt((C_SQUARED * Rs) / (2.0 * r * r * r));

  auto acc = computeGeodesicAcceleration(r, 0.0, dphi, Rs);

  // Net radial acceleration: gravity (inward) + centrifugal (outward)
  // At photon sphere these balance: net d2r = 0
  double gravity = -(C_SQUARED * Rs) / (2.0 * r * r);
  double centrifugal = r * dphi * dphi;
  EXPECT_NEAR(acc.d2r, gravity + centrifugal, 1.0);
  // They should cancel (ratio of magnitudes ≈ 1)
  EXPECT_NEAR(std::abs(gravity), centrifugal, centrifugal * 1e-10);
}
