#pragma once

#include "physics/blackhole.h"
#include "gpu/grid_kernel.h"

#include <glad/gl.h>

namespace bht::render {

// Renders visual representations of a BlackHole (spacetime grid, event horizon, etc.)
class BlackHoleRenderer {
public:
  explicit BlackHoleRenderer(const bht::physics::BlackHole& bh) : bh_(bh) {}
  ~BlackHoleRenderer();

  // Initialize GPU resources. Must be called after GL context is ready.
  bool init(int gridResolution = 80, float gridExtent = 30.0f);

  // Render the spacetime curvature grid.
  void renderSpacetimeGrid(const float* mvp) const;

  // Render the black hole as a red sphere at the event horizon.
  void renderEventHorizon(const float* mvp) const;

private:
  const bht::physics::BlackHole& bh_;

  // Grid GL resources
  GLuint gridVao_ = 0;
  GLuint gridVbo_ = 0;
  GLuint gridEbo_ = 0;
  GLuint gridShader_ = 0;
  GLint gridMvpLoc_ = -1;
  GLint gridRsLoc_ = -1;
  int gridIndexCount_ = 0;

  // Sphere GL resources
  GLuint sphereVao_ = 0;
  GLuint sphereVbo_ = 0;
  GLuint sphereEbo_ = 0;
  GLuint sphereShader_ = 0;
  GLint sphereMvpLoc_ = -1;
  int sphereIndexCount_ = 0;

  float rs_ = 0.0f;

  bool initGrid(int resolution, float extent);
  bool initSphere();
};

} // namespace bht::render
