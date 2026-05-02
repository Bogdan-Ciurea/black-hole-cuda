#pragma once

#include <cuda_runtime.h>

namespace bht::gpu {

// Maximum scene objects in constant memory
constexpr int MAX_SCENE_OBJECTS = 16;

// Object type identifiers (uint32_t for GPU ABI stability)
constexpr uint32_t OBJ_BLACK_HOLE = 0;
constexpr uint32_t OBJ_SPHERE = 1;

// Device-side camera for ray generation
struct DeviceCamera {
  float3 position;
  float3 forward;
  float3 right;
  float3 up;
  float fov; // degrees
};

// Device-side scene object (BH or sphere)
// Approximate multi-BH lensing: superposed Schwarzschild fields (visual only).
struct DeviceSceneObject {
  uint32_t type;
  float3 position;
  float schwarzschildRadius; // capture + lensing radius (BH only)
  float visualRadius;        // render collision radius
  float3 color;              // RGB [0,1]
  float emissionStrength;    // 0 = non-emissive
  float3 emissionColor;      // for future use
};

// Device-side accretion disk (flat XZ plane)
struct DeviceAccretionDisk {
  float3 center;
  float innerRadius;
  float outerRadius;
  float3 innerColor;   // hot inner edge
  float3 outerColor;   // cool outer edge
  float emissionStrength;
};

constexpr int MAX_ACCRETION_DISKS = 4;

} // namespace bht::gpu
