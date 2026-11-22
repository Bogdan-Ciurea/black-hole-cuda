# BlackHoleTracer

A C++20, CUDA, and OpenGL black hole visualization project. It traces rays
around a Schwarzschild black hole to show gravitational lensing: light bending,
capture at the event horizon, and distorted background objects.

The main demo combines GPU ray tracing with an accretion disk, animated
turbulence, particle jets, a starfield, bloom, and exposure controls. Smaller
demos show the steps behind it, from drawing a circle to tracing rays on the CPU.

This is an educational, experimental renderer, not a precision astrophysics
solver. The rendering paths use simplified gravity models, and effects such as
jets are visual additions rather than a full simulation of black hole matter.

## Requirements

The supplied presets are for **Windows 10/11 with Visual Studio 2022**.

| Requirement | Purpose |
| --- | --- |
| Visual Studio 2022, with **Desktop development with C++** and a Windows SDK | C++ compiler and MSBuild |
| NVIDIA CUDA Toolkit **13.0+**, including Visual Studio integration | CUDA compiler and runtime |
| Compatible NVIDIA GPU and current NVIDIA driver | GPU rendering and CUDA/OpenGL interoperability |
| OpenGL **4.6 Core** support | Window rendering and shaders |
| CMake **3.28+** | Configuration, dependency fetching, and builds |
| Git | Cloning this project and its dependencies |
| Python 3 with pip and **Jinja2** | Generating the GLAD OpenGL loader |
| Internet access for the first configuration/build | Fetching dependencies and the optional skybox |

The presets compile for CUDA compute capability **12.0** (`120`), targeting
hardware such as the RTX 5070. See [Other NVIDIA GPUs](#other-nvidia-gpus) if your
card has a different compute capability.

**CUDA is required even when building the CPU demos or unit tests.** The current
build always enables CUDA and links everything against the shared engine; there
is no CPU-only configuration switch.

## Install and build

Install the tools above, then open **Developer PowerShell for VS 2022**.
Ensure `cmake`, `git`, `python`, and `nvcc` are available in that terminal.
All commands below use PowerShell and run from the repository root.

### 1. Clone the repository

```powershell
git clone https://github.com/Bogdan-Ciurea/black-hole-cuda.git
Set-Location .\black-hole-cuda
```

### 2. Prepare the GLAD generator

Install Jinja2 into the Python environment that CMake will use:

```powershell
python -m pip install "Jinja2>=2.7,<4.0"
```

You do not need to install GLAD itself as a Python package; CMake fetches its
source and invokes the generator from there.

### 3. Configure and build

Use Release for interactive rendering:

```powershell
cmake --preset release
cmake --build --preset release
```

For development and debugging:

```powershell
cmake --preset debug
cmake --build --preset debug
```

An additional `relwithdebinfo` configure/build preset provides an optimized
build with debug information.

Configuration fetches **GLM, GLFW, GLAD, Google Test, and stb** automatically.
They do not need separate manual installation. Downloaded dependency sources
and generated build files live under the selected build directory.

CMake also downloads a star map from
[Solar System Scope](https://www.solarsystemscope.com/textures/) to
`assets\skybox.jpg` if it is absent. This directory is excluded from Git.
If the image download fails, configuration reports a warning and the main demo
can use a procedural starfield instead. Dependency download failures, unlike a
missing skybox, must be resolved before building.

### Other NVIDIA GPUs

Override the preset's architecture with the compute capability supported by
your GPU and CUDA Toolkit. For a local build on a GPU detected by CMake:

```powershell
cmake --preset release -DCMAKE_CUDA_ARCHITECTURES=native
cmake --build --preset release
```

Alternatively, pass an explicit architecture number appropriate for your GPU.
Use the same override when configuring Debug if you plan to run its tests.
The override affects that build directory; it does not edit the shared presets.

## Run

Start the full scene after building Release:

```powershell
.\build\release\app\Release\main.exe
```

You should see an interactive black hole scene with an accretion disk, background
stars, and a grid overlay. The mouse is captured for camera movement.
Press **Esc** or close the window to exit.

For a Debug build, the equivalent command is:

```powershell
.\build\debug\app\Debug\main.exe
```

The path contains both the preset directory and the Visual Studio configuration:
`build\<preset>\app\<configuration>\<demo>.exe`.

### Main scene controls

| Input | Action |
| --- | --- |
| Mouse | Look around |
| W / S | Move forward / backward on the horizontal plane |
| A / D | Move sideways |
| Q / E | Move up / down |
| G | Toggle the geodesic grid |
| H | Toggle the accretion disk |
| B | Toggle bloom |
| T | Toggle disk turbulence |
| J | Toggle jets |
| + / - | Increase / decrease exposure; hold to adjust continuously |
| Esc | Exit |

### Other demos

Each source file in `app` produces an executable with the same base name.
Replace `main.exe` in the run command with one of these:

| Executable | What it demonstrates |
| --- | --- |
| `circle.exe` | Minimal OpenGL window, shader, and fullscreen-quad setup |
| `rays2d.exe` | Precomputed 2D ray trails around a black hole |
| `simulation2d.exe` | 2D rays integrated incrementally while the window runs |
| `geodesic_grid.exe` | A 3D grid visualization around the black hole |
| `raytrace3d.exe` | CPU ray tracing at 200 x 200, displayed in an 800 x 800 window |
| `raytrace_cuda.exe` | A simpler interactive CUDA ray tracer at 800 x 800 |

The CPU ray tracer is intentionally low resolution and can be much slower than
the CUDA version. Captured rays appear red in that diagnostic demo.
The 3D CPU and CUDA ray tracers share the mouse, WASD, and Q/E camera controls.
In the grid demo, the mouse orbits, W/S adjusts distance, and Q/E adjusts height.
Esc exits all demos.

To build just one application target:

```powershell
cmake --build --preset release --target raytrace_cuda
.\build\release\app\Release\raytrace_cuda.exe
```

This still builds the engine and any dependencies required by that application.

## Project architecture

The project separates reusable engine code from runnable demonstrations and
tests:

```text
black-hole-cuda\
  CMakeLists.txt          Top-level project and target setup
  CMakePresets.json       Windows configure, build, and test presets
  cmake\                 Dependency setup and skybox download
  src\                   BlackHoleEngine static library
    core\                GLFW window, OpenGL context, event polling
    math\                Constants, coordinate conversions, matrix utilities
    physics\             Black hole properties and geodesic acceleration
    ray\                 Ray state, Euler and RK4 integration
    scene\               Cameras and scene-object descriptions
    render\              CPU tracing and OpenGL display helpers
    gpu\                 CUDA tracing, bloom, skybox, and CUDA/GL interop
  app\                   One demonstration executable per .cpp file
  tests\                 Google Test cases, combined into bht_tests
  docs\                  Development plan, physics notes, visual references
  assets\                Downloaded/local images, not tracked in Git
  build\                 Generated builds and dependencies, not tracked in Git
```

**Build structure:** `BlackHoleEngine` is a static library containing the
engine's `.cpp` and `.cu` sources. Applications and `bht_tests` link against it.
CMake discovers engine sources recursively, application sources in `app`, and
`test_*.cpp` files in `tests`. Reconfigure after adding source files.

**Conceptual layers:** math and physics describe the model; ray integrators
advance it; cameras and scene objects provide inputs; render and GPU modules
produce and display images. Core supplies the window and event loop. The
interactive cameras and scene objects also bridge to CUDA device types.

**Main scene frame flow:**

```text
Keyboard/mouse input -> camera and scene state
                    -> CUDA ray tracing into an HDR buffer
                    -> exposure / tone mapping into a display buffer
                    -> bloom
                    -> OpenGL image display and grid overlay
```

CUDA/OpenGL interoperability lets CUDA write the image buffer that OpenGL
displays. This requires a compatible GPU for both the CUDA computation and the
OpenGL context.

The CPU/2D physics code uses double-precision calculations. The GPU rendering
path also has its own device structures and float-based calculations; do not
assume every demo implements identical numerical equations or precision.

For deeper background, see the [development plan](docs/PLAN.md),
[physics notes](docs/physics_cheatsheet.md), and
[visual references](docs/visual_reference.md). These are supporting design
documents; the commands above describe the current build layout.

## Tests

Tests use **Google Test**, with individual cases registered in **CTest**.
Build before running them:

```powershell
cmake --preset debug
cmake --build --preset debug --target bht_tests
ctest --preset debug
```

For another GPU, add the architecture override to the configure command as
described above. The Debug test preset selects the Debug configuration and
prints failure output.

Useful variations:

```powershell
# List discovered tests without running them
ctest --preset debug -N

# Run only black hole and integrator cases
ctest --preset debug -R "^(BlackHole|Integrator)\."

# Invoke Google Test directly with a filter
.\build\debug\tests\Debug\bht_tests.exe --gtest_filter="CpuTracer.*"

# Run tests in an already-built Release configuration
ctest --preset release

# Alternative Visual Studio test target, after building the tests
cmake --build .\build\debug --config Debug --target RUN_TESTS
```

The suite covers:

- Schwarzschild radius, event-horizon checks, and photon-sphere properties.
- Coordinate conversion, round trips, and angle normalization.
- Geodesic acceleration and circular-orbit conditions.
- Euler and RK4 integration, accuracy comparisons, and ray deflection.
- Camera ray generation and aspect-ratio behavior.
- CPU tracer capture/escape behavior and output size.

These are numerical and CPU-side tests, not end-to-end GPU or visual regression
tests. They do not open the interactive demo windows. Run the demos separately
to check CUDA/OpenGL interoperability, rendering, effects, and camera controls.

## Troubleshooting

- **CUDA compiler not found:** install the toolkit and its Visual Studio
  integration, then reopen Developer PowerShell. Check `nvcc --version`.
- **Unsupported GPU architecture or "no kernel image":** configure with an
  architecture supported by your card and toolkit instead of the preset's
  `120`, then rebuild.
- **`No module named jinja2`:** install Jinja2 into the interpreter CMake
  selected. To select the same Python used by your terminal explicitly:

  ```powershell
  $pythonPath = python -c "import sys; print(sys.executable)"
  cmake --preset release "-DPython_EXECUTABLE=$pythonPath"
  cmake --build --preset release
  ```

- **Window creation or OpenGL initialization fails:** verify OpenGL 4.6 support
  and update the graphics driver.
- **CUDA/GL interoperability fails:** make sure the application uses the NVIDIA
  GPU, not an integrated GPU. On multi-GPU machines, check Windows graphics
  preferences and the display's GPU.
- **Executable not found:** build first and check the configuration-specific
  path. The directory is `app`, not `apps`.
- **Skybox missing:** rerun configuration to retry the download. The main demo
  also tries `assets\skybox.png` before `assets\skybox.jpg`, and uses a procedural
  starfield if neither loads.

## License

The project code is released under the [MIT License](LICENSE). Downloaded
third-party libraries and the skybox image retain their respective licenses;
the project's MIT license does not relicense those resources.
