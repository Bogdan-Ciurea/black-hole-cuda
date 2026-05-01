#pragma once

#ifdef __CUDACC__
#define BHT_HOST_DEVICE __host__ __device__
#else
#define BHT_HOST_DEVICE
#endif

namespace bht::math {

// Physical constants (SI units)
inline constexpr double G = 6.67430e-11;     // m³ kg⁻¹ s⁻²
inline constexpr double C = 2.99792458e8;    // m/s
inline constexpr double C_SQUARED = C * C;   // m²/s²

// Mathematical constants
inline constexpr double PI = 3.14159265358979323846;
inline constexpr double TWO_PI = 2.0 * PI;
inline constexpr double HALF_PI = PI / 2.0;

// Geometric units (G=1, c=1) for simplified simulations
namespace geometric {
inline constexpr double G = 1.0;
inline constexpr double C = 1.0;
} // namespace geometric

} // namespace bht::math
