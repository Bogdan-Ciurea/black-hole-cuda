#pragma once

#include "physics/blackhole.h"
#include "scene/camera.h"

#include <glm/glm.hpp>

#include <vector>

namespace bht::render {

struct TraceParams {
  int maxSteps = 10000;
  double dt = 0.5;       // step size in units of Rs
  double escapeRadius = 50.0; // in units of Rs
};

// Trace all pixels on CPU, output RGB per pixel
void cpuTrace(std::vector<glm::vec3>& pixels, int width, int height,
              const bht::scene::Camera& camera, const bht::physics::BlackHole& bh,
              const TraceParams& params = {});

} // namespace bht::render
