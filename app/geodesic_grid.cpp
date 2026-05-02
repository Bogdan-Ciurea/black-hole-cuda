#include "core/window.h"
#include "math/constants.h"
#include "physics/blackhole.h"
#include "render/blackhole_renderer.h"
#include "scene/orbit_camera.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <cmath>
#include <cstdio>

// Simple 4x4 matrix helpers (column-major)
struct Mat4 {
  float m[16] = {};

  static Mat4 perspective(float fovDeg, float aspect, float near, float far) {
    Mat4 r{};
    float f = 1.0f / tanf(fovDeg * 3.14159265f / 360.0f);
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (far + near) / (near - far);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * far * near) / (near - far);
    return r;
  }

  static Mat4 lookAt(float3 pos, float3 fwd, float3 right, float3 up) {
    Mat4 r{};
    r.m[0] = right.x; r.m[4] = right.y; r.m[8]  = right.z;
    r.m[12] = -(right.x * pos.x + right.y * pos.y + right.z * pos.z);
    r.m[1] = up.x;    r.m[5] = up.y;    r.m[9]  = up.z;
    r.m[13] = -(up.x * pos.x + up.y * pos.y + up.z * pos.z);
    r.m[2] = -fwd.x;  r.m[6] = -fwd.y;  r.m[10] = -fwd.z;
    r.m[14] = (fwd.x * pos.x + fwd.y * pos.y + fwd.z * pos.z);
    r.m[3] = 0;       r.m[7] = 0;       r.m[11] = 0;       r.m[15] = 1.0f;
    return r;
  }

  Mat4 operator*(const Mat4& b) const {
    Mat4 r{};
    for (int c = 0; c < 4; c++)
      for (int row = 0; row < 4; row++)
        for (int k = 0; k < 4; k++)
          r.m[c * 4 + row] += m[k * 4 + row] * b.m[c * 4 + k];
    return r;
  }
};

int main() {
  const int WIDTH = 1024;
  const int HEIGHT = 768;

  Window window;
  if (!window.init(WIDTH, HEIGHT, "BlackHoleTracer - Geodesic Grid")) {
    std::fprintf(stderr, "Failed to create window\n");
    return 1;
  }

  glfwSetInputMode(window.handle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  // Create black hole with Rs = 2.0
  double Rs = 2.0;
  double mass = Rs * bht::math::C_SQUARED / (2.0 * bht::math::G);
  bht::physics::BlackHole bh(mass);
  bh.drawGeodesicGrid = true;

  // Create renderer linked to the black hole
  bht::render::BlackHoleRenderer bhRenderer(bh);
  if (!bhRenderer.init(80, 30.0f)) {
    std::fprintf(stderr, "Failed to init BlackHoleRenderer\n");
    return 1;
  }

  glEnable(GL_DEPTH_TEST);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

  float3 camPos = {0.0f, 25.0f, 50.0f};
  float3 target = {0.0f, 0.0f, 0.0f};
  bht::scene::OrbitCamera cam(camPos, target);

  std::printf("Geodesic grid: 80x80, Rs=%.1f\n", Rs);
  std::printf("Controls: Mouse=orbit, W/S=zoom, Q/E=height, ESC=exit\n");

  while (!window.shouldClose()) {
    window.pollEvents();
    cam.processInput(window.handle(), WIDTH, HEIGHT);

    float3 pos = cam.getPosition();
    float3 fwd = cam.getForward();
    float3 right = cam.getRight();
    float3 up = cam.getUp();

    Mat4 proj = Mat4::perspective(cam.fov, (float)WIDTH / HEIGHT, 0.1f, 500.0f);
    Mat4 view = Mat4::lookAt(pos, fwd, right, up);
    Mat4 mvp = proj * view;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    bhRenderer.render(mvp.m);
    window.swapBuffers();
  }

  return 0;
}
