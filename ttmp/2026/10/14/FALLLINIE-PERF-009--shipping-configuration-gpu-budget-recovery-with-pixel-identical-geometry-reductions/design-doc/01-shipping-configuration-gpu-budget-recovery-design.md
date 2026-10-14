---
Title: Shipping-configuration GPU budget recovery design
Ticket: FALLLINIE-PERF-009
Status: active
Topics:
    - performance
    - graphics
    - handheld
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://src/geometry_bounds.hpp
      Note: Bounds helpers and fog adaptation
    - Path: repo://src/renderer.cpp
      Note: Chunk culling, short indices, acquire scope
    - Path: repo://tests/test_geometry_bounds.cpp
      Note: Conservativeness tests
ExternalSources: []
Summary: Plan and contracts for measuring shadow bounds at the shipping 384x288 dual-view configuration, collecting geometry counters, and adding pixel-identical geometry reductions (fogged-cell constant shading, per-view culling, 16-bit indices) plus a presentation-acquire scope.
LastUpdated: 2026-10-14T18:06:41.235458157-04:00
WhatFor: Executing the next performance step after the vault performance review
WhenToUse: Before implementing or measuring PERF-009 changes; when deciding whether a candidate becomes default
---

# Shipping-configuration GPU budget recovery design

## 1. Purpose

The vault review *PROJECT REPORT - Falllinie Performance Review - Measurement Discipline, Missed Bottlenecks and a Path to 60 FPS* (go-go-parc, 2026-10-14) re-analysed the committed GPU-006/007/008 evidence. Its conclusions, which this ticket acts on, are:

- The dual-view loop is presentation-paced. The mean frame interval is about 16.667 ms plus one refresh per long frame, so the metric that matters is the share of frames that miss a refresh.
- At the shipping configuration (384×288 internal, dual view) the replay needs about 14.0 M GPU-active cycles per loop frame against a 13.33 M budget at 800 MHz (0.043 missed refreshes per frame).
- About 8 M of those cycles do not scale with internal resolution. Geometry is the likely content: seven resident terrain cells, 204,792 terrain triangles per view (≈1.85 per internal pixel), 44-byte vertices and highp varyings, 32-bit indices, terrain drawn to about 660 m ahead although fog is total at 400 m.
- Shadow bounds (GPU-008) were measured only at 512×384. Scaling their measured per-pixel saving predicts ≈12.7 M cycles at 384×288 dual view, i.e. under budget.

This ticket measures that prediction, measures geometry directly, and implements the cheapest pixel-identical geometry reductions. Every change starts opt-in and is promoted to the default only by the rule in Section 8.

## 2. Non-goals

- No change to source shader bodies beyond explicit, documented runtime adaptations (repository rule).
- No pixel-changing optimization (terrain LOD, lower output resolution, replacing fogged terrain with sky). Those are prepared as owner decisions only.
- No GL work off the main thread; the placement worker remains IPC-only.
- No claim of complete game parity or integrated 60 FPS acceptance from replay runs alone.

## 3. Workload and metrics

**Workload.** `--benchmark-replay --frames 600 --seed 42 --render-size 384x288 --dual-screen --no-vsync --fps 0`, ordinary indexed draws, paired clocks at cadence 60, GPU queries off. This is the GPU-006 replay at the shipping size.

**Audio.** Replay currently forces `no_audio`. A new `--replay-audio` flag keeps the replay deterministic but starts the normal buffered audio producer, muted, with the launcher's 1,024-frame buffer. The producer is a consumer of game feedback only; it does not feed back into simulation, so the replay signature must be unchanged. This makes the audio-on/audio-off comparison possible in one session.

**Primary metrics, per case (frames ≥ 60):**

| Metric | Definition |
|---|---|
| Mean interval | mean of `interval_ms` |
| Long-frame share | fraction of intervals ≥ 25 ms |
| Missed refreshes per frame | mean / 16.6667 − 1 |
| p95, p99 | pooled percentiles |
| GPU-active Mcycles per frame | `MaliGPUActiveCy` rate ÷ loop FPS (estimate) |

**Governor.** Each campaign records the CPU governor, sets `performance` for the duration of the campaign and restores the recorded value in a `finally` path. Before/after values are stored in the campaign directory. This removes the 816 MHz–1.99 GHz frequency spread seen in earlier campaigns.

## 4. Phase A — shadow bounds at the shipping configuration

Sequence (one session, one binary): `original, bounds, bounds, original` with audio off, then the same four with `--replay-audio`. Eight timed cases, each with the existing 45 s whole-device counter capture.

Decision: if bounds removes most missed refreshes at 384×288 (long-frame share near zero) without changing the replay signature or the 40-pair visual result already established in GPU-008, it is a promotion candidate. The visual qualification is re-run at 384×288 (Section 6).

## 5. Phase B — geometry counters

A copy of the PERF-004 counter probe takes a counter-set argument. Set `base` reproduces the existing 13 counters. Set `geometry` selects, from the pinned Mali-G52 reference:

`MaliGPUActiveCy, MaliFragActiveCy, MaliNonFragActiveCy, MaliGeomTotalPrim, MaliGeomVisiblePrim, MaliGeomFaceXYPlaneCullPrim, MaliGeomZPlaneCullPrim, MaliGeomSampleCullPrim, MaliTilerPosShadThread, MaliTilerVarShadThread, MaliFragRastQd, MaliFragThread, MaliFragEZSKillQd, MaliFragFPKKillQd, MaliExtBusRdBy, MaliExtBusWrBy`

Per-frame values are normalised by loop FPS, as in the review. The questions: how many primitives per frame are submitted, what fraction is culled and why (frustum vs sample/microtriangle), how many quads are rasterised per internal pixel, and how many varying-shader threads run.

## 6. Phase C — pixel-identical geometry reductions

All three are runtime choices recorded in the receipt and the startup log. Each has an argument for pixel identity; each is nevertheless qualified by image comparison.

### 6.1 Constant shading for cells entirely in full fog (`--fog-cells constant`)

For a scene fragment the original shader ends with

```glsl
float f=smoothstep(uFog.x,uFog.y,dist);
lum=mix(lum,1.,f); blk*=1.-f; gry*=1.-f;
o=vec4(clamp(lum,0.,1.),blk,uMat/255.,gry);
```

with `dist=length(vW-uCam)` and `uFog=(150,400)`. When `dist >= 400`, `f` is exactly 1 (smoothstep clamps its parameter to 1, and 1·1·(3−2) = 1). Then `blk` and `gry` become exactly 0 for finite inputs, `lum` becomes 1 or within one ulp of 1, and the RGBA8 target stores 255. The output is `(1, 0, uMat/255, 0)` independent of lighting, shadows, contours and glyphs.

Adaptation: insert `uniform int uFogConstant;` and, as the first statement of `main`, `if(uFogConstant!=0){o=vec4(1.,0.,uMat/255.,0.);return;}`. The CPU sets `uFogConstant=1` for a terrain or object mesh only when the minimum distance from the shading camera position (`uCam`, the unshaken-plus-shake eye actually uploaded) to the mesh's world-space bounding box exceeds `400 + 2` m. The 2 m margin covers float interpolation of `vW` and float evaluation of `length` with a very large factor of safety at these magnitudes. Particles, trail and rider are never flagged.

The flag is a uniform, so the branch is uniform control flow and the derivative functions in the original body are unaffected. The same program and vertex shader are used, so depth is bit-identical.

### 6.2 Per-view frustum culling (`--view-culling on`)

Each static terrain and object mesh stores a world-space AABB computed from its uploaded float positions. Before drawing in a view, the eight corners, translated by the frame's render origin, are transformed by the frame's `vp` in double precision. The mesh is skipped if all eight corners lie strictly outside the same clip half-space (`x<-w`, `x>w`, `y<-w`, `y>w`, `z<-w`, `z>w`) by a relative margin. Clip coordinates are linear in position, so if all corners of the box are outside one half-space, every point of the box is; such a mesh produces no fragments and skipping it cannot change the image. The margin covers the difference between double evaluation and the GPU's float transform.

### 6.3 16-bit indices for static meshes (`--short-indices on`)

Static terrain cells (14,873 vertices) and object meshes with at most 65,536 vertices upload `GL_UNSIGNED_SHORT` indices. Index values and order are unchanged; only storage width changes. Draw offsets for those meshes are always 0. Dynamic meshes and the rider keep 32-bit indices. `glDrawRangeElements` (when `--draw-ranges` is on) passes the matching type.

## 7. Phase D — presentation-acquire scope and window depth

**Acquire scope.** A new paired-clock/profiler child scope, `acquire`, inside `output`, wraps an explicit `glClear(GL_COLOR_BUFFER_BIT)` of the default framebuffer immediately after binding it (`--output-clear on`). The full-screen copy overwrites every pixel, so the clear does not change the image. If the hypothesis is right, the wait for a free window buffer moves into `acquire`, and its wall-minus-CPU time per window measures presentation throttling directly.

**Window depth.** `--window-depth 0` requests `SDL_GL_DEPTH_SIZE=0` for the window surfaces. The output pass never enables depth testing, and the offscreen scene depth is a separate texture. Kept separate from the clear so each change is measured alone.

## 8. Promotion rule

A candidate becomes the default when all of these hold:

1. **Pixel identity:** every compared image is byte-identical to the original mode: full-window top and lower backbuffers plus internal scene/print/depth for both cameras, at the five GPU-008 checkpoints (frames 120/600/1200 seed 42 palette 0; frame 600 seed 7; frame 600 palette 5), at 384×288.
2. **Signature:** replay signature unchanged.
3. **Timing:** in an ABBA campaign, the long-frame share and mean are not worse than the original's bracket.
4. **Health:** `GL_error=0`, complete paired records, zero counter collection errors.

Candidates that fail (1) are kept opt-in and documented. Defaults change in a dedicated commit that names the evidence directory.

## 9. Validation gates

- Host build and the existing CPU tests (`ctest`) at each code boundary.
- New CPU tests: frustum rejection is conservative on randomized boxes against reference float clip transforms; fog-distance bound is conservative against densely sampled box points; 16-bit conversion preserves index values.
- ARM build with the existing toolchain image.
- Device visual qualification and timing campaign per phase.

## 10. Files

| File | Change |
|---|---|
| `src/main.cpp` | flags, replay audio, window depth, receipt fields |
| `src/renderer.hpp/.cpp` | mesh bounds, index type, culling, fog-constant uniform, acquire scope |
| `src/geometry_bounds.hpp` | new: AABB, clip-space rejection, box distance |
| `src/profiler.hpp/.cpp`, `src/paired_clocks.*` | `acquire` pass label |
| `tests/` | new bound tests |
| `ttmp/.../FALLLINIE-PERF-009.../scripts/` | build, probe, target case, capture, campaign, analysis |

## 11. Risks

- The fog-constant adaptation adds a uniform and a branch to the scene program; Mali may schedule the original body differently. Pixel identity is checked, and timing decides.
- Culling changes the set of draw calls; driver state caching or index-range scans could shift CPU cost between scopes. Measured, not assumed.
- `--replay-audio` might perturb timing beyond the real game's audio load if the replay runs faster than real time; with presentation pacing near 60 Hz the producer runs at its normal rate.
- Changing the governor is a device-wide setting; the campaign must restore it even on failure.
