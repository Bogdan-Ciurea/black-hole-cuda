# BlackHoleTracer — Visual Enhancement Plan

This document describes 10 visual features to be implemented in the BlackHoleTracer project. Each feature includes a description of what it does, where it fits in the codebase, and the expected visual result.

---

## 1. Rotating Accretion Disk

### Description
Animate the accretion disk so it appears to rotate around the black hole over time. The disk material orbits at Keplerian velocities — inner material orbits faster than outer material (differential rotation). This is achieved by passing a `time` uniform to the kernel and applying an angular offset to the disk's radial color gradient based on the azimuthal angle and elapsed time.

### Implementation
- Pass a `float time` parameter to `UnifiedTraceParams` (incremented each frame from `glfwGetTime()`).
- In `diskColor()`, compute the azimuthal angle `atan2(dz, dx)` and add a time-dependent angular offset: `angle + time * angularSpeed(r)` where `angularSpeed(r) = sqrt(GM / r³)` (Keplerian).
- Use the modified angle to create spiral-arm-like brightness variations in the disk color.

### Expected Result
The accretion disk appears to swirl around the black hole. Inner regions rotate faster than outer regions, creating a natural differential rotation pattern. The disk feels alive rather than static.

---

## 2. Doppler Beaming

### Description
In a rotating accretion disk, material on the side approaching the observer appears brighter and blue-shifted, while the receding side appears dimmer and red-shifted. This is the single most recognizable feature of realistic black hole renders (as seen in Interstellar's Gargantua). It breaks the disk's visual symmetry and immediately signals "this is a real physics simulation."

### Implementation
- Compute the orbital velocity direction of disk material at the hit point: `v_orbital = cross(diskNormal, radialDir) * orbitalSpeed(r)`.
- Project the orbital velocity onto the ray direction to get the radial velocity component `v_r = dot(v_orbital, rayDir)`.
- Compute the Doppler factor: `D = 1 / (gamma * (1 - v_r/c))` where `gamma = 1/sqrt(1 - v²/c²)`.
- Scale brightness by `D³` (relativistic beaming) and shift color temperature by `D` (blue-shift for approaching, red-shift for receding).

### Expected Result
The accretion disk is visibly asymmetric: one side (approaching the camera) is noticeably brighter and bluer, while the opposite side is dimmer and redder. The asymmetry changes as the camera orbits around the black hole, providing a dynamic and physically accurate visual.

---

## 3. Bloom / Glow Post-Process

### Description
Apply a bloom post-processing effect so that bright pixels (disk inner edge, stars, photon ring) bleed light into surrounding darker pixels. This simulates how real cameras and eyes perceive very bright light sources — they appear to glow and have a soft halo.

### Implementation
- After the unified kernel writes to the framebuffer, run a two-pass Gaussian blur on a separate CUDA buffer:
  1. **Threshold pass**: extract pixels above a brightness threshold into a "bright buffer."
  2. **Horizontal blur**: apply a 1D Gaussian kernel horizontally.
  3. **Vertical blur**: apply a 1D Gaussian kernel vertically.
  4. **Composite**: add the blurred bright buffer back onto the original framebuffer.
- All passes run as separate CUDA kernels on the GPU for performance.
- Bloom intensity and threshold should be configurable via `UnifiedTraceParams`.

### Expected Result
The hot inner edge of the accretion disk radiates a soft golden-white glow. Stars have visible halos proportional to their brightness. The photon ring (if implemented) glows intensely. Dark regions remain dark. The overall image has a cinematic, HDR-like quality.

---

## 4. Photon Ring

### Description
At exactly 1.5× the Schwarzschild radius (the "photon sphere"), light can theoretically orbit the black hole. In practice, rays that pass very close to this radius get bent by nearly 180° or more before escaping, creating a thin, extremely bright ring around the black hole's silhouette. This is distinct from the accretion disk — it's light from the entire universe compressed into a narrow ring.

### Implementation
- In the kernel's escape path, track the **closest approach distance** (`r_min`) of the ray to each black hole during integration.
- When a ray escapes, check if `r_min` was within a narrow band around `1.5 * Rs` (e.g., `1.5*Rs ± 0.2*Rs`).
- If so, apply a brightness boost proportional to how close `r_min` was to the photon sphere: `boost = exp(-k * (r_min - 1.5*Rs)²)`.
- Add the boosted light as a white/blue-white contribution to the final pixel color.
- This stacks with lensing amplification for even brighter photon ring pixels.

### Expected Result
A thin, bright ring hugs the black hole's silhouette, just outside the event horizon shadow. It's brighter than the surrounding lensed background and has a slightly blue-white tint. When the accretion disk is present, the photon ring appears as an additional bright arc superimposed on the disk's inner edge. It's one of the most visually striking features of a black hole render.

---

## 5. Background Skybox

### Description
Replace the procedural hash-based starfield with a high-resolution background texture (equirectangular star map or cubemap). This provides a much more realistic and detailed stellar background, with nebulae, the Milky Way band, and varying star densities. Gravitational lensing distortions become far more visually impressive when applied to a detailed background.

### Implementation
- Load an equirectangular HDR star map (e.g., a free NASA/ESA deep sky image) as a CUDA texture.
- In the kernel's escape path, instead of calling `starField()`, convert the ray's final direction to spherical UV coordinates and sample the texture.
- Use `cudaTextureObject_t` with bilinear filtering for smooth sampling.
- Apply the existing lensing amplification to the sampled color.
- The texture is loaded once at startup and bound for the lifetime of the application.
- Provide a fallback to the procedural starfield if no texture is found.

### Expected Result
The background is a rich, detailed star field with visible nebulae and the Milky Way. Gravitational lensing creates dramatic warping of background stars — the Einstein ring effect becomes spectacularly visible as background stars are stretched and amplified around the BH silhouette. The overall image looks like a photograph rather than a procedural render.

---

## 6. Disk Thickness / Turbulence

### Description
Real accretion disks are not perfectly flat — they have vertical structure due to thermal pressure and turbulence. Add a slight vertical displacement to the disk intersection test using procedural noise, making the disk appear to have volume, ripples, and turbulent structure.

### Implementation
- In `segmentDiskIntersect()`, instead of testing against a flat `y = center.y` plane, compute a displaced Y based on the XZ position: `y_displaced = center.y + amplitude * noise(x, z, time)`.
- Use a simple 2D hash-based noise function (similar to the starfield hash) for the displacement.
- The noise amplitude should be small relative to the disk extent (e.g., 0.3–0.5 units) and scale with radius (thicker at the outer edge).
- Optionally animate the noise with time for a turbulent, roiling appearance.

### Expected Result
The accretion disk has visible texture and irregularity. Instead of a perfectly smooth gradient ring, it has subtle bumps, ripples, and turbulent structure. When animated, the disk appears to churn and flow. This adds significant visual realism without heavy computational cost.

---

## 7. Gravitational Redshift on Disk

### Description
Light emitted from deep within a gravitational well loses energy as it climbs out, shifting to longer (redder) wavelengths. For the accretion disk, material near the inner edge (close to the BH) is significantly red-shifted compared to the outer edge. This is separate from the radial color gradient — it's a physical dimming and reddening caused by gravity.

### Implementation
- Compute the gravitational redshift factor at the disk hit point: `z = 1 / sqrt(1 - Rs/r) - 1` where `r` is the distance from the BH center.
- Apply the redshift to the disk color: reduce blue/green channels more than red, and dim overall brightness by `1 / (1 + z)`.
- This compounds with the existing radial gradient (inner=hot, outer=cool) to create a physically motivated color profile.

### Expected Result
The innermost ring of the disk, while intrinsically the hottest, appears dimmer and redder than expected due to gravitational redshift. There's a visible transition zone where the disk is brightest — not at the very inner edge, but slightly outward where the balance between intrinsic luminosity and gravitational dimming peaks. This is physically accurate and adds subtle realism.

---

## 8. Particle Jets

### Description
Many accreting black holes produce relativistic jets — narrow beams of plasma ejected along the spin axis (perpendicular to the accretion disk). These jets are visually dramatic: long, faintly glowing columns extending far above and below the black hole.

### Implementation
- Define jets as two conical volumes along the Y axis (up and down from the BH center).
- Parameters: half-angle (~5–10°), length (e.g., 50 units), color (blue-white), and opacity falloff.
- In the kernel, test if a ray segment passes through either jet cone.
- If so, accumulate a faint additive glow based on path length through the cone and distance from the jet axis.
- The jet should be semi-transparent (additive blending, not solid) so the background shows through.
- Optionally add brightness variation along the jet length for a knotty/pulsed appearance.

### Expected Result
Two faint, narrow, blue-white beams extend vertically from the black hole, perpendicular to the accretion disk. They taper and fade with distance. The jets are semi-transparent — stars and background are visible through them. They frame the black hole dramatically and immediately communicate "this is an active galactic nucleus."

---

## 9. Tone Mapping / HDR

### Description
Currently, pixel colors are clamped to [0, 1] which means bright areas (inner disk, stars with lensing amplification) all clip to white. Tone mapping compresses the high dynamic range of the scene into displayable [0, 1] range while preserving detail in both bright and dark regions. This prevents the washed-out look of clipped highlights.

### Implementation
- Accumulate pixel colors in `float3` (not `uchar4`) during ray tracing — allow values > 1.0.
- After the main kernel, apply a tone mapping operator in a separate CUDA kernel pass:
  - **Reinhard**: `color_mapped = color / (1 + color)` — simple, preserves ratios.
  - Or **ACES filmic**: more cinematic with a slight S-curve and warm highlights.
- Add an exposure parameter: `color *= exposure` before tone mapping.
- Convert to `uchar4` only after tone mapping.
- Exposure can be adjustable at runtime (e.g., +/- keys).

### Expected Result
The accretion disk's inner edge retains color detail instead of blowing out to flat white. Stars near the BH that get lensing amplification still appear bright but have visible color gradients. Dark regions like the BH shadow and deep space retain contrast. The overall image has a cinematic, film-like quality with smooth gradients from dark to bright.

---

## 10. Anti-Aliasing (Supersampling)

### Description
Ray-traced edges (BH silhouette, Einstein ring, disk edges, star boundaries) currently show visible jagged aliasing because each pixel is traced from a single ray through its center. Supersampling shoots multiple rays per pixel with slight sub-pixel offsets and averages the results, producing smooth edges.

### Implementation
- Add a `samplesPerPixel` parameter to `UnifiedTraceParams` (e.g., 1, 4, 9, 16).
- For each pixel, generate `N` rays with jittered sub-pixel offsets (stratified sampling within the pixel area).
- Accumulate colors from all samples and divide by N for the final pixel color.
- This multiplies kernel workload by N, so provide a runtime toggle (e.g., press `A` to cycle 1×/4×/9×).
- For 4× SSAA, use a 2×2 grid with random jitter within each quadrant.

### Expected Result
The black hole silhouette edge is smooth instead of stair-stepped. The Einstein ring is a clean, continuous curve. Disk edges and star boundaries are crisp without jaggies. At 4× SSAA, the image quality is dramatically improved at the cost of 4× render time. At 1× (default), performance is unchanged. The toggle allows trading quality for framerate interactively.

---

## Implementation Order (Suggested)

The features are ordered by visual impact and dependency:

| Priority | Feature | Depends On | Complexity |
|----------|---------|------------|------------|
| 1 | Rotating Accretion Disk | — | Low |
| 2 | Doppler Beaming | Rotating Disk | Medium |
| 3 | Gravitational Redshift | — | Low |
| 4 | Photon Ring | — | Low |
| 5 | Tone Mapping / HDR | — | Medium |
| 6 | Bloom / Glow | Tone Mapping | Medium |
| 7 | Disk Thickness / Turbulence | — | Low |
| 8 | Anti-Aliasing | — | Low |
| 9 | Particle Jets | — | Medium |
| 10 | Background Skybox | — | High |

Features 1–4 are kernel-only changes (modify `unified_kernel.cu`). Features 5–6 require a post-processing pipeline. Feature 10 requires texture loading infrastructure.
