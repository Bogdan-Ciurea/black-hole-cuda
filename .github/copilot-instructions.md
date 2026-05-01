# Copilot Instructions — BlackHoleTracer

## Project Overview

BlackHoleTracer is a modular C++20 + CUDA + OpenGL black hole raytracer.
It simulates gravitational lensing around a Schwarzschild black hole.

## Code Conventions

### Namespaces

All engine code lives under the `bht` namespace, with sub-namespaces per module:

```cpp
namespace bht::core { }    // windowing, app loop
namespace bht::math { }    // coordinates, constants
namespace bht::physics { } // black hole, geodesics
namespace bht::ray { }     // ray state, integrators
namespace bht::render { }  // CPU renderer, pixel buffer
namespace bht::scene { }   // camera, accretion disk
namespace bht::gpu { }     // CUDA kernels, GL interop
```

### File Naming

- Headers: `PascalCase.h` (e.g., `BlackHole.h`, `RK4Integrator.h`)
- Sources: `PascalCase.cpp` or `.cu` for CUDA
- Tests: `test_snake_case.cpp` (e.g., `test_blackhole.cpp`)

### Include Order

1. Corresponding header (for .cpp files)
2. Project headers (`"engine/..."`)
3. Third-party headers (`<glm/...>`, `<GLFW/...>`)
4. Standard library headers (`<vector>`, `<cmath>`)

Separated by blank lines between groups.

### CUDA Compatibility

- Physics and math functions that need to run on GPU must be marked `__host__ __device__`
- Use the macro `BHT_HOST_DEVICE` defined as:
  ```cpp
  #ifdef __CUDACC__
  #define BHT_HOST_DEVICE __host__ __device__
  #else
  #define BHT_HOST_DEVICE
  #endif
  ```
- Keep these functions in headers (inline or header-only) for CUDA compilation
- Use `double` for all physics calculations

### Error Handling

- CUDA calls: Use `CUDA_CHECK(call)` macro that checks return code and throws on error
- OpenGL: Check errors in debug builds via `glGetError()` wrapper
- GLFW: Set error callback that logs to stderr
- No exceptions in `__device__` code

### Style

- 2-space indentation, no tabs
- No braces for single-line `if`/`for` (per .clang-format)
- Column limit: 100 characters
- Prefer `constexpr` for compile-time constants
- Use `auto` when the type is obvious from context
- Prefer range-based for loops

### Build System

- CMake 3.28+ with FetchContent for dependencies
- Dependencies: GLFW, GLAD, GLM, Google Test
- Engine is a static library target: `BlackHoleEngine`
- Apps are separate executables linking the engine
- CUDA architecture: sm_120 (RTX 5070, CC 12.0)

### Testing

- Google Test for unit tests
- Use `EXPECT_NEAR` with explicit tolerance for floating-point comparisons
- Test names: `TEST(Module, Function_Condition_ExpectedResult)`
- Run with: `cmake --build build/debug --target RUN_TESTS`

## Module Dependency Order

```
math → physics → ray → render/scene → gpu
         ↑                    ↑
       core (windowing) ------+
```

No circular dependencies. Each module only depends on modules "below" it.
