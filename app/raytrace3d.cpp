#include "core/window.h"
#include "physics/blackhole.h"
#include "render/cpu_tracer.h"
#include "render/screen_quad.h"
#include "scene/camera.h"

#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <glm/glm.hpp>

#include <cmath>
#include <cstdio>
#include <vector>

static double lastMouseX = 0, lastMouseY = 0;
static double yaw = 0.0, pitch = 0.0;
static bool firstMouse = true;
static bool mouseEnabled = false;

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
  if (!mouseEnabled) {
    firstMouse = true;
    return;
  }
  if (firstMouse) {
    lastMouseX = xpos;
    lastMouseY = ypos;
    firstMouse = false;
    return;
  }
  double dx = xpos - lastMouseX;
  double dy = lastMouseY - ypos; // inverted
  lastMouseX = xpos;
  lastMouseY = ypos;

  double sensitivity = 0.2;
  yaw += dx * sensitivity;
  pitch += dy * sensitivity;
  if (pitch > 89.0) pitch = 89.0;
  if (pitch < -89.0) pitch = -89.0;
}

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

  glfwSetCursorPosCallback(window.handle(), mouseCallback);

  bht::render::ScreenQuad quad;
  if (!quad.init(RENDER_W, RENDER_H)) {
    std::fprintf(stderr, "Failed to init screen quad\n");
    return 1;
  }

  // Black hole at origin
  double Rs = 2.0;
  double mass = Rs * bht::math::C_SQUARED / (2.0 * bht::math::G);
  bht::physics::BlackHole bh(mass);

  // Camera starts behind and to the side
  glm::dvec3 camPos = {0.0, 2.0, -20.0};
  yaw = 90.0; // looking toward +Z (toward origin)

  bht::render::TraceParams params;
  params.maxSteps = 3000;
  params.dt = 0.2;
  params.escapeRadius = 80.0;

  std::vector<glm::vec3> pixels;
  bool needsRedraw = true;

  std::printf("Controls: WASD=move, QE=up/down, RMB=look, ESC=exit\n");
  std::printf("Rendering at %dx%d, displayed at %dx%d\n", RENDER_W, RENDER_H, WINDOW_W, WINDOW_H);

  while (!window.shouldClose()) {
    window.pollEvents();
    GLFWwindow* w = window.handle();

    // Right mouse button enables mouse look
    mouseEnabled = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    if (mouseEnabled)
      glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    else
      glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // Compute camera direction from yaw/pitch
    double radYaw = glm::radians(yaw);
    double radPitch = glm::radians(pitch);
    glm::dvec3 front;
    front.x = std::cos(radPitch) * std::cos(radYaw);
    front.y = std::sin(radPitch);
    front.z = std::cos(radPitch) * std::sin(radYaw);
    front = glm::normalize(front);

    glm::dvec3 worldUp = {0.0, 1.0, 0.0};
    glm::dvec3 right = glm::normalize(glm::cross(front, worldUp));
    glm::dvec3 up = glm::cross(right, front);

    // Movement
    double speed = Rs * 0.5;
    bool moved = false;
    if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) { camPos += front * speed; moved = true; }
    if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) { camPos -= front * speed; moved = true; }
    if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) { camPos -= right * speed; moved = true; }
    if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) { camPos += right * speed; moved = true; }
    if (glfwGetKey(w, GLFW_KEY_Q) == GLFW_PRESS) { camPos += worldUp * speed; moved = true; }
    if (glfwGetKey(w, GLFW_KEY_E) == GLFW_PRESS) { camPos -= worldUp * speed; moved = true; }

    if (moved || mouseEnabled) needsRedraw = true;

    // Only re-trace when camera changes
    if (needsRedraw) {
      bht::scene::Camera cam;
      cam.position = camPos;
      cam.forward = front;
      cam.right = right;
      cam.up = up;
      cam.fov = 70.0;

      bht::render::cpuTrace(pixels, RENDER_W, RENDER_H, cam, bh, params);
      quad.upload(pixels);
      needsRedraw = false;
    }

    glClear(GL_COLOR_BUFFER_BIT);
    quad.draw();
    window.swapBuffers();
  }

  return 0;
}
