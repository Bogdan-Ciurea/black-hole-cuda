#include "core/window.h"
#include "gpu/cuda_gl_interop.h"
#include "gpu/ray_kernel.h"

#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include <cmath>
#include <cstdio>

static double yaw = 90.0, pitch = 0.0;

void cudaDemoMouseCallback(GLFWwindow*, double xpos, double ypos) {
  // Cursor is captured and centered each frame, so offset from center = movement
  // This callback is unused — we poll manually in the loop
  (void)xpos;
  (void)ypos;
}

int main() {
  const int WIDTH = 800;
  const int HEIGHT = 800;

  Window window;
  if (!window.init(WIDTH, HEIGHT, "BlackHoleTracer - CUDA")) {
    std::fprintf(stderr, "Failed to create window\n");
    return 1;
  }

  glfwSetCursorPosCallback(window.handle(), cudaDemoMouseCallback);
  glfwSetInputMode(window.handle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  bht::gpu::CudaGLInterop interop;
  if (!interop.init(WIDTH, HEIGHT)) {
    std::fprintf(stderr, "Failed to init CUDA-GL interop\n");
    return 1;
  }

  // Black hole
  float Rs = 2.0f;
  bht::gpu::DeviceBlackHole bh{{0.0f, 0.0f, 0.0f}, Rs};

  bht::gpu::DeviceTraceParams params;
  params.maxSteps = 3000;
  params.dt = 0.2f;
  params.escapeRadius = 80.0f;

  float3 camPos = {0.0f, 2.0f, -20.0f};
  float speed = Rs * 0.5f;

  std::printf("CUDA Raytracer: %dx%d, real-time\n", WIDTH, HEIGHT);
  std::printf("Controls: WASD=move, QE=up/down, RMB=look, ESC=exit\n");

  while (!window.shouldClose()) {
    window.pollEvents();
    GLFWwindow* w = window.handle();

    // Mouse look — cursor is captured, poll offset from center
    double mx, my;
    glfwGetCursorPos(w, &mx, &my);
    double centerX = WIDTH / 2.0, centerY = HEIGHT / 2.0;
    double sensitivity = 0.15;
    yaw += (mx - centerX) * sensitivity;
    pitch += (my - centerY) * sensitivity;
    if (pitch > 89.0) pitch = 89.0;
    if (pitch < -89.0) pitch = -89.0;
    glfwSetCursorPos(w, centerX, centerY);

    // Camera direction
    float radYaw = static_cast<float>(yaw * 3.14159265 / 180.0);
    float radPitch = static_cast<float>(pitch * 3.14159265 / 180.0);
    float3 front = {cosf(radPitch) * cosf(radYaw), sinf(radPitch), cosf(radPitch) * sinf(radYaw)};
    float len = sqrtf(front.x * front.x + front.y * front.y + front.z * front.z);
    front = {front.x / len, front.y / len, front.z / len};

    float3 worldUp = {0.0f, 1.0f, 0.0f};
    float3 right = {front.y * worldUp.z - front.z * worldUp.y,
                    front.z * worldUp.x - front.x * worldUp.z,
                    front.x * worldUp.y - front.y * worldUp.x};
    len = sqrtf(right.x * right.x + right.y * right.y + right.z * right.z);
    right = {right.x / len, right.y / len, right.z / len};

    float3 up = {right.y * front.z - right.z * front.y, right.z * front.x - right.x * front.z,
                 right.x * front.y - right.y * front.x};

    // Movement (W/S along forward projected on XZ plane, A/D strafe)
    float3 flatFront = {cosf(radYaw), 0.0f, sinf(radYaw)};
    float3 flatRight = {-sinf(radYaw), 0.0f, cosf(radYaw)};
    if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) {
      camPos.x += flatFront.x * speed;
      camPos.z += flatFront.z * speed;
    }
    if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) {
      camPos.x -= flatFront.x * speed;
      camPos.z -= flatFront.z * speed;
    }
    if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) {
      camPos.x -= flatRight.x * speed;
      camPos.z -= flatRight.z * speed;
    }
    if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) {
      camPos.x += flatRight.x * speed;
      camPos.z += flatRight.z * speed;
    }
    if (glfwGetKey(w, GLFW_KEY_Q) == GLFW_PRESS) {
      camPos.y += speed;
    }
    if (glfwGetKey(w, GLFW_KEY_E) == GLFW_PRESS) {
      camPos.y -= speed;
    }

    // Build camera
    bht::gpu::DeviceCamera cam{camPos, front, right, up, 70.0f};

    // Render with CUDA
    uchar4* devPtr = interop.map();
    bht::gpu::launchRayKernel(devPtr, WIDTH, HEIGHT, cam, bh, params);
    interop.unmap();

    glClear(GL_COLOR_BUFFER_BIT);
    interop.draw();
    window.swapBuffers();
  }

  return 0;
}
