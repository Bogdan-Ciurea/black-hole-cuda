#include "core/window.h"
#include "gpu/bloom.h"
#include "gpu/cuda_gl_interop.h"
#include "gpu/skybox.h"
#include "gpu/unified_kernel.h"
#include "math/mat4.h"
#include "render/scene_renderer.h"
#include "scene/fps_camera.h"
#include "scene/scene_object.h"

#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include <cmath>
#include <cstdio>
#include <vector>

using bht::gpu::OBJ_BLACK_HOLE;
using bht::gpu::OBJ_SPHERE;
using bht::math::Mat4;

struct SceneInputState {
  bool prevG = false;
  bool prevH = false;
  bool prevB = false;
  bool prevT = false;
  bool prevJ = false;
};

void processSceneInput(GLFWwindow* win,
                       std::vector<bht::scene::SceneObject>& objects,
                       std::vector<bht::gpu::DeviceSceneObject>& devObjects,
                       bht::gpu::BloomParams& bloomParams,
                       bht::gpu::UnifiedTraceParams& params,
                       SceneInputState& state) {
  // Toggle geodesic grid with G (edge-triggered)
  bool curG = glfwGetKey(win, GLFW_KEY_G) == GLFW_PRESS;
  if (curG && !state.prevG) {
    objects[0].drawGeodesicGrid = !objects[0].drawGeodesicGrid;
    std::printf("Geodesic grid: %s\n",
                objects[0].drawGeodesicGrid ? "ON" : "OFF");
  }
  state.prevG = curG;

  // Toggle accretion disk with H (edge-triggered)
  bool curH = glfwGetKey(win, GLFW_KEY_H) == GLFW_PRESS;
  if (curH && !state.prevH) {
    objects[0].hasAccretionDisk = !objects[0].hasAccretionDisk;
    devObjects[0] = objects[0].toDevice();
    std::printf("Accretion disk: %s\n",
                objects[0].hasAccretionDisk ? "ON" : "OFF");
  }
  state.prevH = curH;

  // Toggle bloom with B (edge-triggered)
  bool curB = glfwGetKey(win, GLFW_KEY_B) == GLFW_PRESS;
  if (curB && !state.prevB) {
    bloomParams.enabled = !bloomParams.enabled;
    std::printf("Bloom: %s\n", bloomParams.enabled ? "ON" : "OFF");
  }
  state.prevB = curB;

  // Toggle turbulence with T (edge-triggered)
  bool curT = glfwGetKey(win, GLFW_KEY_T) == GLFW_PRESS;
  if (curT && !state.prevT) {
    params.turbulenceEnabled = !params.turbulenceEnabled;
    std::printf("Turbulence: %s\n", params.turbulenceEnabled ? "ON" : "OFF");
  }
  state.prevT = curT;

  // Toggle jets with J (edge-triggered)
  bool curJ = glfwGetKey(win, GLFW_KEY_J) == GLFW_PRESS;
  if (curJ && !state.prevJ) {
    params.jetsEnabled = !params.jetsEnabled;
    std::printf("Jets: %s\n", params.jetsEnabled ? "ON" : "OFF");
  }
  state.prevJ = curJ;

  // Exposure control with +/- (continuous while held)
  if (glfwGetKey(win, GLFW_KEY_EQUAL) == GLFW_PRESS)
    params.exposure = std::fmin(params.exposure * 1.02f, 10.0f);
  if (glfwGetKey(win, GLFW_KEY_MINUS) == GLFW_PRESS)
    params.exposure = std::fmax(params.exposure * 0.98f, 0.1f);
}

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

  // Skybox (optional — falls back to procedural starfield if not found)
  bht::gpu::Skybox skybox;
  std::string assetsDir = std::string(BHT_PROJECT_DIR) + "/assets/";
  if (!skybox.load(assetsDir + "skybox.png"))
    skybox.load(assetsDir + "skybox.jpg");

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
  bh.diskHalfThickness = 0.4f;
  bh.hasJets = true;
  bh.jetLength = 50.0f;
  bh.jetHalfAngle = 8.0f;
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
  params.backgroundColor = {0.192f, 0.129f, 0.118f}; // #31211e
  params.skyboxTex = skybox.texture();

  // Scene renderer (OpenGL spheres + grids)
  bht::render::SceneRenderer renderer;
  if (!renderer.init()) {
    std::fprintf(stderr, "Failed to init SceneRenderer\n");
    return 1;
  }

  // Bloom post-process
  bht::gpu::BloomPostProcess bloom;
  if (!bloom.init(WIDTH, HEIGHT)) {
    std::fprintf(stderr, "Failed to init bloom\n");
    return 1;
  }
  bht::gpu::BloomParams bloomParams;

  // HDR buffer for tone mapping pipeline
  float4* hdrBuffer = nullptr;
  cudaMalloc(&hdrBuffer, WIDTH * HEIGHT * sizeof(float4));

  // Camera — slightly above disk plane for a cinematic angle
  bht::scene::FPSCamera cam;
  cam.position = {0.0f, 8.0f, -30.0f};
  cam.speed = 1.0f;

  glEnable(GL_DEPTH_TEST);

  std::printf("Controls: WASD=move, QE=up/down, Mouse=look, ESC=exit\n");
  std::printf("          G=grid, H=disk, B=bloom, T=turbulence, J=jets\n");
  std::printf("          +/-=exposure\n");

  SceneInputState inputState;

  while (!window.shouldClose()) {
    window.pollEvents();
    cam.processInput(window.handle(), WIDTH, HEIGHT);
    processSceneInput(window.handle(), objects, devObjects, bloomParams, params, inputState);

    auto devCam = cam.toDeviceCamera();
    params.time = static_cast<float>(std::fmod(glfwGetTime(), 1000.0));

    // CUDA ray trace → HDR buffer → tone map → LDR PBO
    uchar4* devPtr = interop.map();
    bht::gpu::launchUnifiedKernel(hdrBuffer, WIDTH, HEIGHT, devCam,
                                  devObjects.data(),
                                  static_cast<int>(devObjects.size()),
                                  params);
    bht::gpu::launchToneMapKernel(hdrBuffer, devPtr, WIDTH, HEIGHT, params.exposure);
    bloom.apply(devPtr, bloomParams);
    interop.unmap();

    // Draw background
    glClearColor(params.backgroundColor.x, params.backgroundColor.y,
                 params.backgroundColor.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw ray-traced image first (skybox fills background when active)
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    interop.draw();
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    // Draw OpenGL grid ON TOP of the ray-traced image
    float3 pos = cam.position;
    float3 fwd = cam.getForward();
    float3 right = cam.getRight();
    float3 up = cam.getUp();

    Mat4 proj = Mat4::perspective(cam.fov, (float)WIDTH / HEIGHT, 0.1f, 500.0f);
    Mat4 view = Mat4::lookAt(pos, fwd, right, up);
    Mat4 mvp = proj * view;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    renderer.render(mvp.m, objects);
    glDisable(GL_BLEND);

    window.swapBuffers();
  }

  cudaFree(hdrBuffer);
  return 0;
}
