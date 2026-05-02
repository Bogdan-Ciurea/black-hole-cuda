#pragma once

#include "gpu/ray_kernel.h"

#include <GLFW/glfw3.h>

#include <cmath>

namespace bht::scene {

struct OrbitCamera {
  float3 target = {0.0f, 0.0f, 0.0f};
  float distance = 50.0f;
  double azimuth = 0.0;    // degrees, horizontal angle around target
  double elevation = 30.0; // degrees, vertical angle above target
  float fov = 60.0f;
  float zoomSpeed = 0.3f;
  float verticalSpeed = 0.3f;
  double sensitivity = 0.15;

  OrbitCamera() = default;

  OrbitCamera(float3 camPos, float3 lookAt) : target(lookAt) {
    float dx = camPos.x - lookAt.x;
    float dy = camPos.y - lookAt.y;
    float dz = camPos.z - lookAt.z;
    distance = sqrtf(dx * dx + dy * dy + dz * dz);
    float horizDist = sqrtf(dx * dx + dz * dz);
    elevation = atan2f(dy, horizDist) * 180.0f / 3.14159265f;
    azimuth = atan2f(dz, dx) * 180.0f / 3.14159265f;
  }

  float3 getPosition() const {
    float radAz = static_cast<float>(azimuth * 3.14159265 / 180.0);
    float radEl = static_cast<float>(elevation * 3.14159265 / 180.0);
    float horizDist = distance * cosf(radEl);
    return {target.x + horizDist * cosf(radAz),
            target.y + distance * sinf(radEl),
            target.z + horizDist * sinf(radAz)};
  }

  float3 getForward() const {
    float3 pos = getPosition();
    float3 f = {target.x - pos.x, target.y - pos.y, target.z - pos.z};
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

  // Poll mouse + keyboard, update orbit state.
  // Expects cursor captured (GLFW_CURSOR_DISABLED).
  void processInput(GLFWwindow* w, int width, int height) {
    // Mouse drag orbits around target
    double mx, my;
    glfwGetCursorPos(w, &mx, &my);
    double centerX = width / 2.0, centerY = height / 2.0;
    azimuth += (mx - centerX) * sensitivity;
    elevation += (my - centerY) * sensitivity; // mouse up = higher elevation
    if (elevation > 89.0) elevation = 89.0;
    if (elevation < -89.0) elevation = -89.0;
    glfwSetCursorPos(w, centerX, centerY);

    // W/S = zoom in/out
    if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) distance -= zoomSpeed;
    if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) distance += zoomSpeed;
    if (distance < 1.0f) distance = 1.0f;

    // Q/E = move target up/down
    if (glfwGetKey(w, GLFW_KEY_Q) == GLFW_PRESS) target.y += verticalSpeed;
    if (glfwGetKey(w, GLFW_KEY_E) == GLFW_PRESS) target.y -= verticalSpeed;
  }

  bht::gpu::DeviceCamera toDeviceCamera() const {
    return {getPosition(), getForward(), getRight(), getUp(), fov};
  }
};

} // namespace bht::scene
