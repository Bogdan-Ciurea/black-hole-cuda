#include "core/window.h"
#include "math/constants.h"
#include "physics/blackhole.h"
#include "render/blackhole_renderer.h"

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

  static Mat4 lookAt(float ex, float ey, float ez, float cx, float cy, float cz) {
    float fx = cx - ex, fy = cy - ey, fz = cz - ez;
    float fl = sqrtf(fx * fx + fy * fy + fz * fz);
    fx /= fl; fy /= fl; fz /= fl;

    float ux = 0.0f, uy = 1.0f, uz = 0.0f;
    float rx = fy * uz - fz * uy;
    float ry = fz * ux - fx * uz;
    float rz = fx * uy - fy * ux;
    float rl = sqrtf(rx * rx + ry * ry + rz * rz);
    rx /= rl; ry /= rl; rz /= rl;

    float uux = ry * fz - rz * fy;
    float uuy = rz * fx - rx * fz;
    float uuz = rx * fy - ry * fx;

    Mat4 r{};
    r.m[0] = rx;  r.m[4] = ry;  r.m[8]  = rz;  r.m[12] = -(rx * ex + ry * ey + rz * ez);
    r.m[1] = uux; r.m[5] = uuy; r.m[9]  = uuz; r.m[13] = -(uux * ex + uuy * ey + uuz * ez);
    r.m[2] = -fx; r.m[6] = -fy; r.m[10] = -fz; r.m[14] = (fx * ex + fy * ey + fz * ez);
    r.m[3] = 0;   r.m[7] = 0;   r.m[11] = 0;   r.m[15] = 1.0f;
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

  // Create black hole with Rs = 2.0
  double Rs = 2.0;
  double mass = Rs * bht::math::C_SQUARED / (2.0 * bht::math::G);
  bht::physics::BlackHole bh(mass);

  // Create renderer linked to the black hole
  bht::render::BlackHoleRenderer bhRenderer(bh);
  if (!bhRenderer.init(80, 30.0f)) {
    std::fprintf(stderr, "Failed to init BlackHoleRenderer\n");
    return 1;
  }

  glEnable(GL_DEPTH_TEST);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

  float angle = 0.0f;
  float camHeight = 25.0f;
  float camDist = 50.0f;

  std::printf("Geodesic grid: 80x80, Rs=%.1f\n", Rs);
  std::printf("Controls: A/D=orbit, Q/E=height, W/S=zoom, ESC=exit\n");

  while (!window.shouldClose()) {
    window.pollEvents();
    GLFWwindow* w = window.handle();

    if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) angle -= 0.02f;
    if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) angle += 0.02f;
    if (glfwGetKey(w, GLFW_KEY_Q) == GLFW_PRESS) camHeight += 0.3f;
    if (glfwGetKey(w, GLFW_KEY_E) == GLFW_PRESS) camHeight -= 0.3f;
    if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) camDist -= 0.3f;
    if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) camDist += 0.3f;
    if (camDist < 5.0f) camDist = 5.0f;

    float ex = cosf(angle) * camDist;
    float ez = sinf(angle) * camDist;

    Mat4 proj = Mat4::perspective(60.0f, (float)WIDTH / HEIGHT, 0.1f, 500.0f);
    Mat4 view = Mat4::lookAt(ex, camHeight, ez, 0.0f, 0.0f, 0.0f);
    Mat4 mvp = proj * view;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    bhRenderer.renderSpacetimeGrid(mvp.m);
    bhRenderer.renderEventHorizon(mvp.m);
    window.swapBuffers();
  }

  return 0;
}
