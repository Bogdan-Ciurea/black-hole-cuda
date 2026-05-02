#include "core/window.h"
#include "math/constants.h"
#include "math/mat4.h"
#include "physics/blackhole.h"
#include "render/blackhole_renderer.h"
#include "scene/orbit_camera.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <cstdio>

using bht::math::Mat4;

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
