#include "core/window.h"
#include "gpu/cuda_gl_interop.h"
#include "gpu/ray_kernel.h"
#include "scene/fps_camera.h"

#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include <cstdio>

int main() {
  const int WIDTH = 800;
  const int HEIGHT = 800;

  Window window;
  if (!window.init(WIDTH, HEIGHT, "BlackHoleTracer - CUDA")) {
    std::fprintf(stderr, "Failed to create window\n");
    return 1;
  }

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

  bht::scene::FPSCamera cam;
  cam.position = {0.0f, 2.0f, -20.0f};
  cam.speed = Rs * 0.5f;

  std::printf("CUDA Raytracer: %dx%d, real-time\n", WIDTH, HEIGHT);
  std::printf("Controls: WASD=move, QE=up/down, Mouse=look, ESC=exit\n");

  while (!window.shouldClose()) {
    window.pollEvents();
    cam.processInput(window.handle(), WIDTH, HEIGHT);

    auto devCam = cam.toDeviceCamera();

    uchar4* devPtr = interop.map();
    bht::gpu::launchRayKernel(devPtr, WIDTH, HEIGHT, devCam, bh, params);
    interop.unmap();

    glClear(GL_COLOR_BUFFER_BIT);
    interop.draw();
    window.swapBuffers();
  }

  return 0;
}
