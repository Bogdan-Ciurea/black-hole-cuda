#include <gtest/gtest.h>

#include "scene/camera.h"

#include <cmath>

using namespace bht::scene;

TEST(Camera, DefaultLooksForward) {
  Camera cam;
  EXPECT_DOUBLE_EQ(cam.forward.z, 1.0);
  EXPECT_DOUBLE_EQ(cam.right.x, 1.0);
  EXPECT_DOUBLE_EQ(cam.up.y, 1.0);
}

TEST(Camera, LookAtConstructor) {
  Camera cam({0, 0, -10}, {0, 0, 0});
  // Should look along +Z
  EXPECT_NEAR(cam.forward.z, 1.0, 1e-10);
  EXPECT_NEAR(cam.forward.x, 0.0, 1e-10);
  EXPECT_NEAR(cam.forward.y, 0.0, 1e-10);
}

TEST(Camera, CenterRayMatchesForward) {
  Camera cam({0, 0, -10}, {0, 0, 0});
  // Center pixel of odd-sized image aligns exactly with forward
  auto dir = cam.generateRayDir(400, 400, 801, 801);
  EXPECT_NEAR(dir.x, 0.0, 1e-5);
  EXPECT_NEAR(dir.y, 0.0, 1e-5);
  EXPECT_NEAR(dir.z, 1.0, 1e-5);
}

TEST(Camera, CornersSymmetric) {
  Camera cam({0, 0, -10}, {0, 0, 0}, {0, 1, 0}, 90.0);
  auto topLeft = cam.generateRayDir(0, 0, 800, 800);
  auto topRight = cam.generateRayDir(799, 0, 800, 800);
  auto botLeft = cam.generateRayDir(0, 799, 800, 800);
  auto botRight = cam.generateRayDir(799, 799, 800, 800);

  // X symmetry: topLeft.x ≈ -topRight.x
  EXPECT_NEAR(topLeft.x, -topRight.x, 0.01);
  // Y symmetry: topLeft.y ≈ -botLeft.y
  EXPECT_NEAR(topLeft.y, -botLeft.y, 0.01);
  // Diagonal symmetry
  EXPECT_NEAR(topLeft.x, -botRight.x, 0.01);
  EXPECT_NEAR(topLeft.y, -botRight.y, 0.01);
}

TEST(Camera, AspectRatioWide) {
  Camera cam({0, 0, -10}, {0, 0, 0}, {0, 1, 0}, 60.0);
  // Wide image: horizontal spread > vertical spread
  auto right = cam.generateRayDir(1599, 400, 1600, 800);
  auto top = cam.generateRayDir(800, 0, 1600, 800);
  EXPECT_GT(std::abs(right.x), std::abs(top.y));
}
