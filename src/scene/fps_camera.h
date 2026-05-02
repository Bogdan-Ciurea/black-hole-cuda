#pragma once

#include "gpu/ray_kernel.h"

#include <GLFW/glfw3.h>

#include <cmath>

namespace bht::scene {

struct FPSCamera {
  float3 position = {0.0f, 2.0f, -20.0f};
  double yaw = 90.0;   // degrees, 90 = looking toward +Z
  double pitch = 0.0;  // degrees
  float speed = 1.0f;
  double sensitivity = 0.15;
  float fov = 70.0f;   // degrees

  // Compute camera basis vectors from yaw/pitch
  float3 getForward() const {
    float ry = static_cast<float>(yaw * 3.14159265 / 180.0);
    float rp = static_cast<float>(pitch * 3.14159265 / 180.0);
    float3 f = {cosf(rp) * cosf(ry), sinf(rp), cosf(rp) * sinf(ry)};
    float len = sqrtf(f.x * f.x + f.y * f.y + f.z * f.z);
    return {f.x / len, f.y / len, f.z / len};
  }

  float3 getRight() const {
    float3 f = getForward();
    float3 worldUp = {0.0f, 1.0f, 0.0f};
    float3 r = {f.y * worldUp.z - f.z * worldUp.y,
                f.z * worldUp.x - f.x * worldUp.z,
                f.x * worldUp.y - f.y * worldUp.x};
    float len = sqrtf(r.x * r.x + r.y * r.y + r.z * r.z);
    return {r.x / len, r.y / len, r.z / len};
  }

  float3 getUp() const {
    float3 f = getForward();
    float3 r = getRight();
    return {r.y * f.z - r.z * f.y,
            r.z * f.x - r.x * f.z,
            r.x * f.y - r.y * f.x};
  }

  // Poll mouse position and keyboard, update camera state.
  // Call once per frame. Expects cursor to be captured (GLFW_CURSOR_DISABLED).
  void processInput(GLFWwindow* w, int width, int height) {
    // Mouse look
    double mx, my;
    glfwGetCursorPos(w, &mx, &my);
    double centerX = width / 2.0, centerY = height / 2.0;
    yaw += (mx - centerX) * sensitivity;
    pitch -= (my - centerY) * sensitivity; // mouse up = look up (BH goes down)
    if (pitch > 89.0) pitch = 89.0;
    if (pitch < -89.0) pitch = -89.0;
    glfwSetCursorPos(w, centerX, centerY);

    // Movement on XZ plane (yaw only, no pitch component)
    float ry = static_cast<float>(yaw * 3.14159265 / 180.0);
    float3 flatFront = {cosf(ry), 0.0f, sinf(ry)};
    float3 flatRight = {-sinf(ry), 0.0f, cosf(ry)};

    if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) {
      position.x += flatFront.x * speed;
      position.z += flatFront.z * speed;
    }
    if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) {
      position.x -= flatFront.x * speed;
      position.z -= flatFront.z * speed;
    }
    if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) {
      position.x -= flatRight.x * speed;
      position.z -= flatRight.z * speed;
    }
    if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) {
      position.x += flatRight.x * speed;
      position.z += flatRight.z * speed;
    }
    if (glfwGetKey(w, GLFW_KEY_Q) == GLFW_PRESS)
      position.y += speed;
    if (glfwGetKey(w, GLFW_KEY_E) == GLFW_PRESS)
      position.y -= speed;
  }

  // Build a DeviceCamera for use with CUDA kernels
  bht::gpu::DeviceCamera toDeviceCamera() const {
    return {position, getForward(), getRight(), getUp(), fov};
  }
};

} // namespace bht::scene
