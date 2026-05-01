# Visual Reference — Expected Outputs Per Phase

## Phase 0 — Window

**Expected**: A black (or dark gray) window, 1280×720, with a title bar reading "BlackHoleTracer". Closes on ESC or window X button. Nothing rendered — just proof the OpenGL context works.

---

## Phase 4 — 2D Ray Visualization

**Expected**:
- Black background
- White or colored filled circle at center (the black hole, radius proportional to Rs)
- Multiple rays drawn as colored trails (thin lines of pixels)
- Rays launched from one side of the screen at various angles
- Rays that pass close to the BH should visibly **curve** toward it
- Rays aimed directly at the BH should terminate (trail ends at the circle)
- Rays far from the BH should be nearly straight
- A few rays should orbit partially before escaping (near-critical impact parameter)

**Reference description**: Imagine dropping marbles at varying angles past a bowling ball — they curve, some get captured, some deflect. This is that, but with light.

---

## Phase 5 — 3D CPU Raytrace

**Expected**:
- Full-screen image rendered once (not real-time, may take seconds)
- Camera looking at the black hole from a distance
- Black circle in center = the black hole shadow (slightly larger than Rs due to lensing)
- **Einstein ring**: a bright ring around the shadow where background light is focused
- Background (if a star field): should appear distorted/warped near the BH
  - Stars behind the BH are lensed into arcs
  - Stars far from the BH appear normal
- The shadow should have radius ≈ `b_crit ≈ 2.6 * Rs` as seen from infinity

**What WRONG looks like**:
- No curvature (rays go straight) → physics not applied
- Shadow too small (= Rs exactly) → missing lensing contribution
- Uniform black/white → color mapping broken

---

## Phase 6 — GPU Accelerated

**Expected**: Visually identical to Phase 5, but rendering in real-time (>30 FPS at 1280×720). The image should update interactively if camera controls are added.

---

## Phase 7 — Full Effects

**Expected**:
- Everything from Phase 6, plus:
- **Accretion disk**: A thin, bright ring/disk around the BH in the equatorial plane
  - Should appear warped — the far side of the disk is visible *above and below* the BH due to lensing
  - Inner edge at ISCO (3Rs), outer edge configurable
  - Optional: color gradient (hotter near inner edge = bluer/whiter, cooler at outer = redder)
- **Photon sphere glow**: Faint brightening at the edge of the shadow
- **Background star field**: Stars that are lensed into arcs near the shadow edge
- **Double image**: The top and bottom of the disk create "mirror" images from backside light wrapping around

**Iconic reference**: The black hole in the movie *Interstellar* (Gargantua) — though ours is non-rotating (Schwarzschild), so the disk appears symmetric (no Doppler blueshift/redshift asymmetry).
