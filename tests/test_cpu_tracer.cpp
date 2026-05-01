#include <gtest/gtest.h>

#include "render/cpu_tracer.h"
#include "physics/blackhole.h"
#include "scene/camera.h"

using namespace bht::render;
using namespace bht::physics;
using namespace bht::scene;

TEST(CpuTracer, RayDirectlyAtBlackHole_HitsRed) {
  // Camera looking straight at black hole at origin
  double Rs = 100.0;
  double mass = Rs * bht::math::C_SQUARED / (2.0 * bht::math::G);
  BlackHole bh(mass);

  Camera cam({0, 0, -500}, {0, 0, 0});

  // Trace a small image; center pixel should hit
  int w = 16, h = 16;
  std::vector<glm::vec3> pixels;
  TraceParams params;
  params.maxSteps = 5000;
  params.dt = 0.3;
  params.escapeRadius = 20.0;

  cpuTrace(pixels, w, h, cam, bh, params);

  // Center pixel (8,8) should be red (hit black hole)
  glm::vec3 center = pixels[8 * w + 8];
  EXPECT_GT(center.r, 0.5f);
  EXPECT_LT(center.g, 0.1f);
  EXPECT_LT(center.b, 0.1f);
}

TEST(CpuTracer, RayFarFromBlackHole_Escapes) {
  // Camera offset far, looking away from black hole
  double Rs = 100.0;
  double mass = Rs * bht::math::C_SQUARED / (2.0 * bht::math::G);
  BlackHole bh(mass);

  // Looking away from origin
  Camera cam({10000, 10000, -10000}, {20000, 20000, -20000});

  int w = 4, h = 4;
  std::vector<glm::vec3> pixels;
  TraceParams params;
  params.maxSteps = 1000;
  params.dt = 0.5;
  params.escapeRadius = 30.0;

  cpuTrace(pixels, w, h, cam, bh, params);

  // All pixels should be background (not red)
  for (auto& p : pixels) {
    EXPECT_LT(p.r, 0.5f); // not red
  }
}

TEST(CpuTracer, OutputSizeMatchesInput) {
  double Rs = 50.0;
  double mass = Rs * bht::math::C_SQUARED / (2.0 * bht::math::G);
  BlackHole bh(mass);
  Camera cam({0, 0, -200}, {0, 0, 0});

  int w = 32, h = 24;
  std::vector<glm::vec3> pixels;
  cpuTrace(pixels, w, h, cam, bh);

  EXPECT_EQ(static_cast<int>(pixels.size()), w * h);
}
