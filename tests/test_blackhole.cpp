#include "physics/blackhole.h"

#include <gtest/gtest.h>

using namespace bht::physics;

TEST(BlackHole, SchwarzschildRadius_SolarMass) {
  // Sun mass = 1.989e30 kg, expected Rs ≈ 2954 m
  BlackHole sun(1.989e30);
  EXPECT_NEAR(sun.schwarzschildRadius, 2954.0, 2.0);
}

TEST(BlackHole, SchwarzschildRadius_EarthMass) {
  // Earth mass = 5.972e24 kg, expected Rs ≈ 8.87 mm
  BlackHole earth(5.972e24);
  EXPECT_NEAR(earth.schwarzschildRadius, 8.87e-3, 1e-4);
}

TEST(BlackHole, IsInsideHorizon_Inside) {
  BlackHole bh(1.989e30);
  EXPECT_TRUE(bh.isInsideHorizon(bh.schwarzschildRadius * 0.5));
}

TEST(BlackHole, IsInsideHorizon_Outside) {
  BlackHole bh(1.989e30);
  EXPECT_FALSE(bh.isInsideHorizon(bh.schwarzschildRadius * 2.0));
}

TEST(BlackHole, PhotonSphere) {
  BlackHole bh(1.989e30);
  EXPECT_NEAR(bh.photonSphereRadius(), 1.5 * bh.schwarzschildRadius, 1e-6);
}

TEST(BlackHole, Position) {
  glm::dvec3 pos{10.0, 20.0, 30.0};
  BlackHole bh(1.0e30, pos);
  EXPECT_DOUBLE_EQ(bh.position.x, 10.0);
  EXPECT_DOUBLE_EQ(bh.position.y, 20.0);
  EXPECT_DOUBLE_EQ(bh.position.z, 30.0);
}
