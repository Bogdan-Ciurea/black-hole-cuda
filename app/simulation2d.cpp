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
  if (!window.init(WIDTH, HEIGHT, "BlackHoleTracer - 2D Simulation")) {
    std::fprintf(stderr, "Failed to create window\n");
    return 1;
  }

  bht::render::Renderer2D renderer;
  if (!renderer.init(WIDTH, HEIGHT)) {
    std::fprintf(stderr, "Failed to init renderer\n");
    return 1;
  }

  // Black hole sized for nice visualization
  double Rs = 50.0;
  double mass = Rs * bht::math::C_SQUARED / (2.0 * bht::math::G);
  bht::physics::BlackHole bh(mass);

  double viewScale = Rs * 15.0;

  // Spawn rays in a vertical line on the left, all moving right
  std::vector<bht::ray::Ray> rays;
  std::vector<bool> active;
  int numRays = 20;
  double spacing = viewScale * 1.5 / numRays;

  for (int i = 0; i < numRays; i++) {
    double yOffset = -viewScale * 0.75 + i * spacing;
    double startX = -viewScale * 0.9;

    double r = std::sqrt(startX * startX + yOffset * yOffset);
    double phi = std::atan2(yOffset, startX);
    if (phi < 0.0) phi += bht::math::TWO_PI;

    double speed = bht::math::C * 0.5;
    double dr = speed * std::cos(phi);
    double dphi = (-speed * std::sin(phi)) / r;

    rays.emplace_back(r, phi, dr, dphi);
    active.push_back(true);
  }

  double dt = Rs / (bht::math::C * 50.0);
  int stepsPerFrame = 10;

  std::printf("Simulation running: %d rays, %d steps/frame\n", numRays, stepsPerFrame);
  std::printf("Press ESC to exit\n");

  while (!window.shouldClose()) {
    window.pollEvents();

    // Integrate active rays
    for (int i = 0; i < numRays; i++) {
      if (!active[i]) continue;

      for (int s = 0; s < stepsPerFrame; s++) {
        rays[i].addToTrail();
        bht::ray::stepRK4(rays[i], dt, bh);

        // Stop if captured or escaped
        if (rays[i].r < bh.schwarzschildRadius || rays[i].r > viewScale * 3.0) {
          active[i] = false;
          break;
        }
      }
    }

    glClear(GL_COLOR_BUFFER_BIT);
    renderer.render(rays, bh, viewScale);
    renderer.display();

    window.swapBuffers();
  }

  return 0;
}
