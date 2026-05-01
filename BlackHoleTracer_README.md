# BlackHoleTracer

## Overview

BlackHoleTracer is a modular C++20 + CUDA + OpenGL project that
simulates how light behaves around a black hole using general relativity
principles.

The goal is to progressively build a physically-based raytracing engine
capable of rendering gravitational lensing effects around a
Schwarzschild black hole.

This project is designed to be developed iteratively with GitHub Copilot
CLI, with clearly defined steps and modular architecture.

------------------------------------------------------------------------

## Core Objectives

-   Implement geodesic-based ray tracing (not classical ray tracing)
-   Simulate light bending using Schwarzschild metric
-   Build a modular rendering engine (CPU → GPU)
-   Use CUDA for high-performance parallel ray integration
-   Visualize results in real-time using OpenGL

------------------------------------------------------------------------

## Tech Stack

-   C++20
-   CUDA (NVIDIA GPU acceleration)
-   OpenGL (GLFW + GLAD)
-   CMake (project management)

------------------------------------------------------------------------

## Architecture

    /engine
      /core        → windowing, app loop
      /math        → vectors, coordinate systems
      /physics     → black hole + geodesics
      /ray         → ray definitions + integrators
      /render      → CPU + GPU rendering
      /scene       → camera, objects
      /gpu         → CUDA kernels

------------------------------------------------------------------------

## Development Roadmap

### Phase 0 --- Setup

-   CMake project with CUDA + OpenGL
-   Window creation and render loop

### Phase 1 --- Math

-   Vector math (Vec3)
-   Coordinate conversions

### Phase 2 --- Physics

-   Black hole model (Schwarzschild radius)
-   Geodesic equations

### Phase 3 --- Rays (2D)

-   Ray structure
-   Euler integrator (baseline)
-   RK4 integrator (final)

### Phase 4 --- 2D Visualization

-   Render rays and trails
-   Validate physics visually

### Phase 5 --- 3D CPU Raytracer

-   Camera system
-   Pixel buffer rendering
-   Per-pixel ray simulation

### Phase 6 --- CUDA Acceleration

-   Parallel ray computation
-   CUDA kernel per pixel
-   OpenGL interop

### Phase 7 --- Visual Effects

-   Accretion disk
-   Gravitational lensing
-   Background stars

------------------------------------------------------------------------

## Physics Model

We simulate light paths using the Schwarzschild metric approximation.

Key equations:

-   Schwarzschild radius: Rs = 2GM / c\^2

-   Angular acceleration: d²φ = (2 / r) \* dr \* dφ

-   Radial acceleration: d²r = (c² \* Rs) / (2r²) + r \* dφ²

Integration method: - Runge-Kutta 4 (RK4)

------------------------------------------------------------------------

## Key Design Principles

-   Modular architecture from the start
-   Identical CPU and GPU logic where possible
-   Accuracy first, optimization later
-   Small incremental steps (Copilot-friendly)

------------------------------------------------------------------------

## Expected Outcomes

-   2D simulation of light bending
-   3D raytraced black hole
-   Real-time GPU rendering
-   Visual effects like photon sphere and accretion disk

------------------------------------------------------------------------

## Notes for Copilot

-   Follow modular file structure strictly
-   Keep functions small and testable
-   Prefer clarity over premature optimization
-   Maintain consistent math conventions
-   Ensure CPU and CUDA implementations match logically

------------------------------------------------------------------------

## End Goal

A real-time physically-based renderer that visually demonstrates how
black holes distort spacetime and bend light.
