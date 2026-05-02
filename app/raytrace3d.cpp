#include "core/window.h"
#include "physics/blackhole.h"
#include "render/cpu_tracer.h"
#include "render/screen_quad.h"
#include "scene/fps_camera.h"

#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <glm/glm.hpp>

#include <cstdio>
#include <vector>

int main() {
  const int RENDER_W = 200;
  const int RENDER_H = 200;
  const int WINDOW_W = 800;
  const int WINDOW_H = 800;

  Window window;
  if (!window.init(WINDOW_W, WINDOW_H, "BlackHoleTracer - 3D CPU")) {
    std::fprintf(stderr, "Failed to create window\n");
    return 1;
  }

  glfwSetInputMode(window.handle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  bht::render::ScreenQuad quad;
  if (!quad.init(RENDER_W, RENDER_H)) {
    std::fprintf(stderr, "Failed to init screen quad\n");
    return 1;
  }

  // Black hole at origin
  double Rs = 2.0;
  double mass = Rs * bht::math::C_SQUARED / (2.0 * bht::math::G);
  bht::physics::BlackHole bh(mass);

  bht::render::TraceParams params;
  params.maxSteps = 3000;
  params.dt = 0.2;
  params.escapeRadius = 80.0;

  bht::scene::FPSCamera fpsCam;
  fpsCam.position = {0.0f, 2.0f, -20.0f};
  fpsCam.speed = static_cast<float>(Rs * 0.5);

  std::vector<glm::vec3> pixels;

  std::printf("Controls: WASD=move, QE=up/down, Mouse=look, ESC=exit\n");
  std::printf("Rendering at %dx%d, displayed at %dx%d\n", RENDER_W, RENDER_H, WINDOW_W, WINDOW_H);

  while (!window.shouldClose()) {
    window.pollEvents();
    fpsCam.processInput(window.handle(), WINDOW_W, WINDOW_H);

    // Build CPU camera from FPSCamera state
    float3 f = fpsCam.getForward();
    float3 r = fpsCam.getRight();
    float3 u = fpsCam.getUp();

    bht::scene::Camera cam;
    cam.position = {fpsCam.position.x, fpsCam.position.y, fpsCam.position.z};
    cam.forward = {f.x, f.y, f.z};
    cam.right = {r.x, r.y, r.z};
    cam.up = {u.x, u.y, u.z};
    cam.fov = fpsCam.fov;

    bht::render::cpuTrace(pixels, RENDER_W, RENDER_H, cam, bh, params);
    quad.upload(pixels);

    glClear(GL_COLOR_BUFFER_BIT);
    quad.draw();
    window.swapBuffers();
  }

  return 0;
}
