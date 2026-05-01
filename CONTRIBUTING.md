# Contributing to BlackHoleTracer

## Prerequisites

- **Windows 10/11**
- **Visual Studio 2022** (Community or higher) with C++ Desktop workload
- **CUDA Toolkit 13.0+** (with compatible NVIDIA driver)
- **CMake 3.28+** (4.x recommended)
- **Git**

## Getting Started

### Clone

```bash
git clone https://github.com/<your-user>/black-hole-cuda.git
cd black-hole-cuda
```

### Configure

Using CMake presets (recommended):

```bash
cmake --preset debug
```

Or manually:

```bash
cmake -B build/debug -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CUDA_ARCHITECTURES=120
```

First configure will download dependencies via FetchContent (GLFW, GLM, GLAD, Google Test). This may take a minute.

### Build

```bash
cmake --build build/debug --config Debug
```

Or use a preset:

```bash
cmake --build --preset debug
```

### Run Tests

```bash
ctest --preset debug
```

Or:

```bash
cmake --build build/debug --target RUN_TESTS --config Debug
```

### Run Demos

Executables are in `build/debug/apps/Debug/`:

```bash
./build/debug/apps/Debug/demo_window.exe
```

## Project Structure

```
black-hole-cuda/
├── cmake/              → CMake helper modules (FetchContent)
├── engine/             → Static library (BlackHoleEngine)
│   ├── core/           → Windowing, app loop
│   ├── math/           → Coordinates, constants
│   ├── physics/        → Black hole model, geodesics
│   ├── ray/            → Ray state, integrators
│   ├── render/         → CPU renderer, pixel buffer
│   ├── scene/          → Camera, accretion disk
│   └── gpu/            → CUDA kernels, GL interop
├── apps/               → Demo executables
├── tests/              → Google Test unit tests
└── docs/               → Physics reference, visual guides
```

## Development Workflow

1. **Check which phase you're implementing** — see the plan or README
2. **Write tests first** (TDD encouraged for physics/math modules)
3. **Implement the feature** in the appropriate engine module
4. **Run tests**: `ctest --preset debug`
5. **Run the phase's demo app** to visually validate
6. **Format code**: `clang-format -i <files>` (uses `.clang-format` in repo root)

## Code Style

- See `.clang-format` for formatting rules
- See `.github/copilot-instructions.md` for naming and architecture conventions
- Key points: 2-space indent, `bht::` namespace, `__host__ __device__` on physics functions

## Adding a New Module File

1. Create `engine/<module>/NewFile.h` and `.cpp`
2. Add the source to `engine/CMakeLists.txt`
3. Add a test in `tests/test_newfile.cpp`
4. Add the test to `tests/CMakeLists.txt`
