#include "core/window.h"
#include "physics/blackhole.h"
#include "ray/integrator_rk4.h"
#include "ray/ray.h"
#include "render/renderer2d.h"

#include <glad/gl.h>

#include <cmath>
#include <cstdio>
#include <vector>

int main() {
  const int WIDTH = 800;
  const int HEIGHT = 800;

  Window window;
  if (!window.init(WIDTH, HEIGHT, "BlackHoleTracer - 2D Rays")) {
    std::fprintf(stderr, "Failed to create window\n");
    return 1;
  }

  bht::render::Renderer2D renderer;
  if (!renderer.init(WIDTH, HEIGHT)) {
    std::fprintf(stderr, "Failed to init renderer\n");
    return 1;
  }

  // Create black hole (geometric units: G=1, c=1, Rs=2*M)
  // We use SI but pick mass so Rs is a nice round number for the view
  double Rs = 50.0; // pixels-ish scale
  double mass = Rs * bht::math::C_SQUARED / (2.0 * bht::math::G);
  bht::physics::BlackHole bh(mass);

  // View scale: how many world units fit in the screen half-width
  double viewScale = Rs * 15.0;

  // Launch rays from the left side at varying angles
  std::vector<bht::ray::Ray> rays;
  int numRays = 20;
  double spacing = viewScale * 1.5 / numRays;

  for (int i = 0; i < numRays; i++) {
    double yOffset = -viewScale * 0.75 + i * spacing;
    double startX = -viewScale * 0.9;

    // Convert to polar
    double r = std::sqrt(startX * startX + yOffset * yOffset);
    double phi = std::atan2(yOffset, startX);
    if (phi < 0.0) phi += bht::math::TWO_PI;

    // Velocity: moving to the right (positive x direction)
    // dr = dx*cos(phi) + dy*sin(phi), dphi = (-dx*sin(phi) + dy*cos(phi))/r
    double speed = bht::math::C * 0.5;
    double dx = speed;
    double dy = 0.0;
    double dr = dx * std::cos(phi) + dy * std::sin(phi);
    double dphi = (-dx * std::sin(phi) + dy * std::cos(phi)) / r;

    rays.emplace_back(r, phi, dr, dphi);
  }

  // Simulate rays (pre-compute trails)
  double dt = Rs / (bht::math::C * 50.0);
  int maxSteps = 5000;

  for (auto& ray : rays) {
    for (int step = 0; step < maxSteps; step++) {
      ray.addToTrail();
      bht::ray::stepRK4(ray, dt, bh);

      // Stop if captured or escaped
      if (ray.r < bh.schwarzschildRadius || ray.r > viewScale * 3.0)
        break;
    }
  }

  std::printf("Simulated %d rays\n", numRays);

  // Render loop — display pre-computed result
  while (!window.shouldClose()) {
    window.pollEvents();

    glClear(GL_COLOR_BUFFER_BIT);
    renderer.render(rays, bh, viewScale);
    renderer.display();

    window.swapBuffers();
  }

  return 0;
}
