#include "render/cpu_tracer.h"
#include "math/constants.h"

#include <cmath>

namespace bht::render {

// 3D geodesic: compute acceleration for a photon at position `pos` moving with velocity `vel`
// near a Schwarzschild black hole at origin.
// Uses: a = -(Rs * c² / (2 * r³)) * pos  (Newtonian-like for visualization)
// Plus relativistic correction for light bending:
// a = -(Rs * c² / (2 * r³)) * pos + 1.5 * Rs * (|vel|² * pos - (dot(pos,vel)) * vel) / r³
static glm::dvec3 geodesicAccel3D(glm::dvec3 pos, glm::dvec3 vel, double Rs) {
  double r2 = glm::dot(pos, pos);
  double r = std::sqrt(r2);
  double r3 = r2 * r;

  if (r < 1e-10) return glm::dvec3(0.0);

  // Schwarzschild photon equation in isotropic coords (post-Newtonian approx)
  // The key GR correction that produces the Einstein ring:
  // d²x/dλ² = -(Rs/(2r³)) * x + (3*Rs/(2*r⁵)) * (x·v)² * x ... simplified to:
  // Use the effective potential approach:
  // a = -0.5*Rs/r³ * pos * (1 - 1.5*Rs/r) + angular correction
  // Simplified but visually correct form:
  double factor = -0.5 * Rs / r3;
  glm::dvec3 newtonian = factor * pos;

  // GR correction: extra 1.5*Rs/r term enhances deflection for photons
  double grCorrection = 1.5 * Rs / r;
  glm::dvec3 accel = newtonian * (1.0 + grCorrection);

  return accel;
}

void cpuTrace(std::vector<glm::vec3>& pixels, int width, int height,
              const bht::scene::Camera& camera, const bht::physics::BlackHole& bh,
              const TraceParams& params) {
  pixels.resize(width * height);

  double Rs = bh.schwarzschildRadius;
  double dt = params.dt * Rs; // step size in world units
  double escapeR2 = (params.escapeRadius * Rs) * (params.escapeRadius * Rs);
  double captureR2 = Rs * Rs; // event horizon

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      glm::dvec3 dir = camera.generateRayDir(x, y, width, height);

      // Ray state
      glm::dvec3 pos = camera.position;
      glm::dvec3 vel = dir; // photon: unit velocity (we trace in affine parameter, not time)

      glm::vec3 color(0.0f, 0.0f, 0.05f); // default dark blue background

      for (int step = 0; step < params.maxSteps; step++) {
        // RK4 integration
        glm::dvec3 k1v = geodesicAccel3D(pos, vel, Rs);
        glm::dvec3 k1x = vel;

        glm::dvec3 p2 = pos + 0.5 * dt * k1x;
        glm::dvec3 v2 = vel + 0.5 * dt * k1v;
        glm::dvec3 k2v = geodesicAccel3D(p2, v2, Rs);
        glm::dvec3 k2x = v2;

        glm::dvec3 p3 = pos + 0.5 * dt * k2x;
        glm::dvec3 v3 = vel + 0.5 * dt * k2v;
        glm::dvec3 k3v = geodesicAccel3D(p3, v3, Rs);
        glm::dvec3 k3x = v3;

        glm::dvec3 p4 = pos + dt * k3x;
        glm::dvec3 v4 = vel + dt * k3v;
        glm::dvec3 k4v = geodesicAccel3D(p4, v4, Rs);
        glm::dvec3 k4x = v4;

        pos += (dt / 6.0) * (k1x + 2.0 * k2x + 2.0 * k3x + k4x);
        vel += (dt / 6.0) * (k1v + 2.0 * k2v + 2.0 * k3v + k4v);

        double r2 = glm::dot(pos, pos);

        // Hit black hole
        if (r2 < captureR2) {
          color = glm::vec3(0.8f, 0.0f, 0.0f); // red
          break;
        }

        // Escaped
        if (r2 > escapeR2) {
          // Background: simple gradient based on ray direction
          glm::dvec3 normDir = glm::normalize(vel);
          float t = static_cast<float>(0.5 * (normDir.y + 1.0));
          color = glm::mix(glm::vec3(0.1f, 0.1f, 0.2f), glm::vec3(0.05f, 0.05f, 0.15f), t);
          break;
        }
      }

      pixels[y * width + x] = color;
    }
  }
}

} // namespace bht::render
