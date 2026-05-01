cmake_minimum_required(VERSION 3.28)
project(BlackHoleTracer LANGUAGES CXX CUDA)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# CUDA standard
set(CMAKE_CUDA_STANDARD 20)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)

# Default CUDA architecture (RTX 5070 = Compute Capability 12.0)
if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
  set(CMAKE_CUDA_ARCHITECTURES native)
endif()

# Dependencies
include(cmake/Dependencies.cmake)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Engine static library (auto-collects sources)
add_subdirectory(src)

# Application executables (one per .cpp file in app/)
add_subdirectory(app)

# Tests
enable_testing()
add_subdirectory(tests)
