#pragma once

#include "gpu/device_types.h"

#include <cuda_runtime.h>

#include <cmath>

namespace bht::scene {

// CPU-side scene object description used by both the renderer and the kernel.
struct SceneObject {
  uint32_t type;               // OBJ_BLACK_HOLE or OBJ_SPHERE
  float3 position;
  float schwarzschildRadius;   // BH only
  float visualRadius;          // render size
  float3 color;                // RGB [0,1]
  float emissionStrength;      // 0 = non-emissive
  float3 emissionColor;
  bool drawGeodesicGrid;       // BH only, CPU-side flag for GL grid overlay

  // Accretion disk (BH only) — flat XZ plane centered on this object
  bool hasAccretionDisk = false;
  float diskInnerRadius = 0.0f;
  float diskOuterRadius = 0.0f;
  float3 diskInnerColor = {1.0f, 1.0f, 1.0f};
  float3 diskOuterColor = {1.0f, 0.0f, 0.0f};
  float diskEmissionStrength = 1.0f;
  float diskHalfThickness = 0.0f;   // vertical half-extent (0 = infinitely thin)

  // Relativistic jets (BH only)
  bool hasJets = false;
  float jetLength = 50.0f;          // half-length along Y axis
  float jetHalfAngle = 8.0f;        // degrees

  // Convert to device-side object for the CUDA kernel
  bht::gpu::DeviceSceneObject toDevice() const {
    return {type, position, schwarzschildRadius, visualRadius,
            color, emissionStrength, emissionColor,
            hasAccretionDisk ? 1 : 0,
            diskInnerRadius, diskOuterRadius,
            diskInnerColor, diskOuterColor, diskEmissionStrength,
            diskHalfThickness,
            hasJets ? 1 : 0, jetLength,
            cosf(jetHalfAngle * 3.14159265f / 180.0f)};
  }
};

} // namespace bht::scene
