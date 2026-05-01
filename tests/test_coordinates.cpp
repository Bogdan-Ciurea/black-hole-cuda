#include "math/coordinates.h"

#include <gtest/gtest.h>

#include <cmath>

using namespace bht::math;

TEST(Coordinates, CartesianToPolar_XAxis) {
  auto p = cartesianToPolar({1.0, 0.0, 0.0});
  EXPECT_NEAR(p.r, 1.0, 1e-10);
  EXPECT_NEAR(p.theta, 0.0, 1e-10);
  EXPECT_NEAR(p.phi, 0.0, 1e-10);
}

TEST(Coordinates, CartesianToPolar_YAxis) {
  auto p = cartesianToPolar({0.0, 1.0, 0.0});
  EXPECT_NEAR(p.r, 1.0, 1e-10);
  EXPECT_NEAR(p.theta, HALF_PI, 1e-10);
  EXPECT_NEAR(p.phi, 0.0, 1e-10);
}

TEST(Coordinates, CartesianToPolar_ZAxis) {
  auto p = cartesianToPolar({0.0, 0.0, 1.0});
  EXPECT_NEAR(p.r, 1.0, 1e-10);
  EXPECT_NEAR(p.theta, 0.0, 1e-10);
  EXPECT_NEAR(p.phi, HALF_PI, 1e-10);
}

TEST(Coordinates, CartesianToPolar_Origin) {
  auto p = cartesianToPolar({0.0, 0.0, 0.0});
  EXPECT_NEAR(p.r, 0.0, 1e-10);
}

TEST(Coordinates, RoundTrip) {
  glm::dvec3 original{3.0, 4.0, 5.0};
  auto polar = cartesianToPolar(original);
  auto back = polarToCartesian(polar.r, polar.theta, polar.phi);
  EXPECT_NEAR(back.x, original.x, 1e-10);
  EXPECT_NEAR(back.y, original.y, 1e-10);
  EXPECT_NEAR(back.z, original.z, 1e-10);
}

TEST(Coordinates, RoundTrip_NegativeCoords) {
  glm::dvec3 original{-2.0, -1.0, 3.5};
  auto polar = cartesianToPolar(original);
  auto back = polarToCartesian(polar.r, polar.theta, polar.phi);
  EXPECT_NEAR(back.x, original.x, 1e-10);
  EXPECT_NEAR(back.y, original.y, 1e-10);
  EXPECT_NEAR(back.z, original.z, 1e-10);
}

TEST(Coordinates, NormalizeAngle_Positive) {
  EXPECT_NEAR(normalizeAngle(3.0 * PI), PI, 1e-10);
}

TEST(Coordinates, NormalizeAngle_Negative) {
  EXPECT_NEAR(normalizeAngle(-HALF_PI), 1.5 * PI, 1e-10);
}
