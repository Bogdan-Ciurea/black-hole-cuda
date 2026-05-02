#include "core/window.h"
#include "gpu/cuda_gl_interop.h"
#include "gpu/unified_kernel.h"
#include "math/mat4.h"
#include "render/scene_renderer.h"
#include "scene/fps_camera.h"
#include "scene/scene_object.h"

#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include <cstdio>
#include <vector>

using bht::gpu::OBJ_BLACK_HOLE;
using bht::gpu::OBJ_SPHERE;
using bht::math::Mat4;

int main() {
  const int WIDTH = 1024;
  const int HEIGHT = 768;

  Window window;
  if (!window.init(WIDTH, HEIGHT, "BlackHoleTracer - Main Scene")) {
    std::fprintf(stderr, "Failed to create window\n");
    return 1;
  }

  glfwSetInputMode(window.handle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  // CUDA-GL interop for ray tracing output
  bht::gpu::CudaGLInterop interop;
  if (!interop.init(WIDTH, HEIGHT)) {
    std::fprintf(stderr, "Failed to init CUDA-GL interop\n");
    return 1;
  }

  // Scene objects
  std::vector<bht::scene::SceneObject> objects;

  // Black hole (black — light doesn't escape)
  bht::scene::SceneObject bh{};
  bh.type = OBJ_BLACK_HOLE;
  bh.position = {0.0f, 0.0f, 0.0f};
  bh.schwarzschildRadius = 2.0f;
  bh.visualRadius = 2.0f;
  bh.color = {0.0f, 0.0f, 0.0f};
  bh.drawGeodesicGrid = true;
  bh.hasAccretionDisk = true;
  bh.diskInnerRadius = 6.0f;              // 3 * Rs (ISCO)
  bh.diskOuterRadius = 18.0f;
  bh.diskInnerColor = {1.0f, 0.95f, 0.7f};  // hot white-yellow
  bh.diskOuterColor = {0.9f, 0.15f, 0.02f}; // deep red-orange outer edge
  bh.diskEmissionStrength = 1.0f;
  objects.push_back(bh);

  // Blue-white hot star (far behind BH — lensed)
  bht::scene::SceneObject star1{};
  star1.type = OBJ_SPHERE;
  star1.position = {40.0f, 8.0f, 70.0f};
  star1.schwarzschildRadius = 0.0f;
  star1.visualRadius = 12.0f;
  star1.color = {0.7f, 0.85f, 1.0f};
  objects.push_back(star1);

  // Warm orange giant (far left, partially lensed)
  bht::scene::SceneObject star2{};
  star2.type = OBJ_SPHERE;
  star2.position = {-35.0f, -5.0f, 55.0f};
  star2.schwarzschildRadius = 0.0f;
  star2.visualRadius = 9.0f;
  star2.color = {1.0f, 0.6f, 0.2f};
  objects.push_back(star2);

  // Build device objects for CUDA kernel
  std::vector<bht::gpu::DeviceSceneObject> devObjects;
  for (auto& obj : objects) devObjects.push_back(obj.toDevice());

  // Trace params: near-black background to make stars/disk pop
  bht::gpu::UnifiedTraceParams params;
  params.maxSteps = 3000;
  params.dt = 0.4f;
  params.escapeRadius = 160.0f;
  params.backgroundColor = {0.02f, 0.02f, 0.03f};

  // Scene renderer (OpenGL spheres + grids)
  bht::render::SceneRenderer renderer;
  if (!renderer.init()) {
    std::fprintf(stderr, "Failed to init SceneRenderer\n");
    return 1;
  }

  // Camera — slightly above disk plane for a cinematic angle
  bht::scene::FPSCamera cam;
  cam.position = {0.0f, 8.0f, -30.0f};
  cam.speed = 1.0f;

  glEnable(GL_DEPTH_TEST);

  std::printf("Main Scene: BH(red) + 2 stars(green), bg #31211e\n");
  std::printf("Controls: WASD=move, QE=up/down, Mouse=look, ESC=exit\n");

  while (!window.shouldClose()) {
    window.pollEvents();
    cam.processInput(window.handle(), WIDTH, HEIGHT);

    auto devCam = cam.toDeviceCamera();

    // CUDA ray trace (writes to PBO texture)
    uchar4* devPtr = interop.map();
    bht::gpu::launchUnifiedKernel(devPtr, WIDTH, HEIGHT, devCam,
                                  devObjects.data(),
                                  static_cast<int>(devObjects.size()),
                                  params);
    interop.unmap();

    // Draw background + OpenGL grid first
    glClearColor(params.backgroundColor.x, params.backgroundColor.y,
                 params.backgroundColor.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float3 pos = cam.position;
    float3 fwd = cam.getForward();
    float3 right = cam.getRight();
    float3 up = cam.getUp();

    Mat4 proj = Mat4::perspective(cam.fov, (float)WIDTH / HEIGHT, 0.1f, 500.0f);
    Mat4 view = Mat4::lookAt(pos, fwd, right, up);
    Mat4 mvp = proj * view;

    renderer.render(mvp.m, objects);

    // Composite ray-traced image on top (alpha blend: objects opaque, bg transparent)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    interop.draw();
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    window.swapBuffers();
  }

  return 0;
}
