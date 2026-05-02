#pragma once

#include <cuda_runtime.h>

#include <cmath>

namespace bht::math {

// Simple 4x4 matrix (column-major, OpenGL convention)
struct Mat4 {
  float m[16] = {};

  static Mat4 perspective(float fovDeg, float aspect, float near, float far) {
    Mat4 r{};
    float f = 1.0f / tanf(fovDeg * 3.14159265f / 360.0f);
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (far + near) / (near - far);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * far * near) / (near - far);
    return r;
  }

  static Mat4 lookAt(float3 pos, float3 fwd, float3 right, float3 up) {
    Mat4 r{};
    r.m[0] = right.x; r.m[4] = right.y; r.m[8]  = right.z;
    r.m[12] = -(right.x * pos.x + right.y * pos.y + right.z * pos.z);
    r.m[1] = up.x;    r.m[5] = up.y;    r.m[9]  = up.z;
    r.m[13] = -(up.x * pos.x + up.y * pos.y + up.z * pos.z);
    r.m[2] = -fwd.x;  r.m[6] = -fwd.y;  r.m[10] = -fwd.z;
    r.m[14] = (fwd.x * pos.x + fwd.y * pos.y + fwd.z * pos.z);
    r.m[3] = 0;       r.m[7] = 0;       r.m[11] = 0;       r.m[15] = 1.0f;
    return r;
  }

  Mat4 operator*(const Mat4& b) const {
    Mat4 r{};
    for (int c = 0; c < 4; c++)
      for (int row = 0; row < 4; row++)
        for (int k = 0; k < 4; k++)
          r.m[c * 4 + row] += m[k * 4 + row] * b.m[c * 4 + k];
    return r;
  }
};

} // namespace bht::math
