# Physics Cheat Sheet

## Coordinate System

We work in **spherical coordinates** (r, θ, φ) for the Schwarzschild metric,
but restrict to the **equatorial plane** (θ = π/2) for 2D simulations.

- **r** — radial distance from the black hole center
- **θ** — polar angle (measured from north pole; π/2 = equatorial plane)
- **φ** — azimuthal angle (in-plane angle)

Conversion to Cartesian:
```
x = r * sin(θ) * cos(φ)
y = r * sin(θ) * sin(φ)
z = r * cos(θ)
```

---

## Schwarzschild Metric

The line element for a non-rotating, uncharged black hole:

```
ds² = -(1 - Rs/r)c²dt² + (1 - Rs/r)⁻¹dr² + r²dΩ²
```

Where:
- `Rs = 2GM/c²` is the Schwarzschild radius
- `dΩ² = dθ² + sin²(θ)dφ²`

---

## Key Radii

| Name | Formula | Meaning |
|------|---------|---------|
| Schwarzschild radius | `Rs = 2GM/c²` | Event horizon |
| Photon sphere | `r = 1.5 * Rs` | Unstable circular photon orbits |
| ISCO | `r = 3 * Rs` | Innermost stable circular orbit (massive particles) |

---

## Geodesic Equations (Equatorial Plane, θ = π/2)

For a photon in the equatorial plane, the equations of motion reduce to:

### State Vector

```
State = (r, φ, dr/dt, dφ/dt)
```

### Equations of Motion

**Radial acceleration:**
```
d²r/dt² = (c² * Rs) / (2 * r²) + r * (dφ/dt)²
```

The first term is the gravitational "pull" (note: for photons this is the effective potential gradient).
The second term is the centrifugal contribution.

**Angular acceleration:**
```
d²φ/dt² = -(2 / r) * (dr/dt) * (dφ/dt)
```

This encodes angular momentum conservation in curved spacetime.

---

## Conservation Laws

**Angular momentum** (approximately conserved by good integrators):
```
L = r² * dφ/dt
```

**Impact parameter** (constant of motion for each ray):
```
b = L / E
```

where E is the photon energy at infinity.

**Critical impact parameter** (capture threshold):
```
b_crit = (3√3 / 2) * Rs ≈ 2.598 * Rs
```

- Rays with `b < b_crit` → captured by black hole
- Rays with `b > b_crit` → deflected and escape
- Rays with `b = b_crit` → asymptotically approach photon sphere

---

## Deflection Angle (Weak Field Limit)

For rays passing far from the black hole (b >> Rs):
```
Δφ ≈ 4GM / (c² * b) = 2Rs / b
```

---

## Unit Conventions

We use **SI units** throughout the code:

| Constant | Symbol | Value |
|----------|--------|-------|
| Gravitational constant | G | 6.674×10⁻¹¹ m³/(kg·s²) |
| Speed of light | c | 2.998×10⁸ m/s |
| Solar mass | M☉ | 1.989×10³⁰ kg |

For visualization/testing, we often use **geometric units** where:
- `G = 1`, `c = 1`
- Distances measured in units of `Rs`
- This simplifies equations: `Rs = 2M`

The code supports both via a `UnitSystem` flag or by setting `G=1, c=1` in constants.

---

## Numerical Integration (RK4)

Given state `y = (r, φ, dr, dφ)` and derivatives `f(y)`:

```
k1 = dt * f(y)
k2 = dt * f(y + k1/2)
k3 = dt * f(y + k2/2)
k4 = dt * f(y + k3)

y_next = y + (k1 + 2*k2 + 2*k3 + k4) / 6
```

### Step Size Considerations

- Near horizon (r → Rs): reduce step size to avoid overshooting
- Far from BH (r >> Rs): can increase step size (nearly straight-line)
- Suggested adaptive criterion: `dt ∝ r / |dr/dt|`

---

## Termination Conditions

A ray integration terminates when:
1. **Captured**: `r < Rs` (crossed event horizon)
2. **Escaped**: `r > r_max` (left simulation domain)
3. **Max steps**: exceeded iteration limit (safety)
4. **Disk hit**: ray crosses equatorial plane within disk bounds (Phase 7)
