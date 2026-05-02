#pragma once

#include "scene/scene_object.h"
#include "gpu/grid_kernel.h"

#include <glad/gl.h>

#include <vector>

namespace bht::render {

// Unified OpenGL renderer for any scene object.
// Renders sphere meshes for all objects and optional geodesic grids for BHs.
class SceneRenderer {
public:
  SceneRenderer() = default;
  ~SceneRenderer();

  // Initialize GPU resources. Call after GL context is ready.
  bool init();

  // Render all objects: sphere meshes + geodesic grids where applicable.
  void render(const float* mvp,
              const std::vector<bht::scene::SceneObject>& objects) const;

private:
  // Sphere mesh (unit sphere, scaled per object via uniform)
  GLuint sphereVao_ = 0;
  GLuint sphereVbo_ = 0;
  GLuint sphereEbo_ = 0;
  GLuint sphereShader_ = 0;
  GLint sphereMvpLoc_ = -1;
  GLint sphereColorLoc_ = -1;
  GLint spherePosLoc_ = -1;
  GLint sphereRadiusLoc_ = -1;
  int sphereIndexCount_ = 0;

  // Grid mesh (recomputed per unique Rs)
  GLuint gridVao_ = 0;
  GLuint gridVbo_ = 0;
  GLuint gridEbo_ = 0;
  GLuint gridShader_ = 0;
  GLint gridMvpLoc_ = -1;
  GLint gridRsLoc_ = -1;
  GLint gridPosLoc_ = -1;
  int gridIndexCount_ = 0;
  float gridRs_ = 0.0f;

  bool initSphere();
  bool initGrid(float rs, float extent, int resolution);

  void renderSphere(const float* mvp,
                    const bht::scene::SceneObject& obj) const;
  void renderGrid(const float* mvp,
                  const bht::scene::SceneObject& obj) const;
};

} // namespace bht::render
