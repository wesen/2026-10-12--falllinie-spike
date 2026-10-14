---
Title: Implementation and measurement diary
Ticket: FALLLINIE-PERF-009
Status: active
Topics:
    - performance
    - graphics
    - handheld
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Detailed diary for PERF-009: shipping-configuration shadow-bounds measurement, geometry counters, pixel-identical geometry reductions and presentation-acquire scope."
LastUpdated: 2026-10-14T18:06:41.484562526-04:00
WhatFor: ""
WhenToUse: ""
---

## Step 1: Design the shipping-configuration budget recovery

This step turns the vault performance review's plan into a ticket and a design document before any code changes. The design fixes the workload (GPU-006 replay at the shipping 384×288 dual-view size), the metric (long-frame share and missed refreshes per frame, not only the mean), the promotion rule (byte-identical images at five checkpoints, unchanged replay signature, no timing regression), and four phases: shadow bounds at the shipping size with and without audio, a geometry counter set, three pixel-identical geometry reductions, and a presentation-acquire scope.

Reading the source to write the design corrected one assumption from the review: the receipt's `vertices`/`indices` count only terrain cell meshes (`Renderer::update_player` sums `cell_meshes_` over the seven-cell patch), so object geometry is additional. It also confirmed the projection path: `vW` is absolute world position while `gl_Position` uses `w.xyz-uRenderOrigin`, which matters for both fog distance and frustum tests.

### Prompt Context
**User prompt (verbatim):** "Ok, create a new design doc for this and then implement, commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)"
**Assistant interpretation:** Create a docmgr ticket with a design document covering the plan I proposed (shadow bounds at the shipping size, geometry counters, pixel-identical geometry reductions, presentation instrumentation), then implement and measure it on the handheld, committing in focused steps and keeping an investigation-format diary.
**Inferred user intent:** Turn the review's recommendations into measured, default-ready improvements toward 60 FPS without fidelity loss, with the same evidence standard as earlier tickets but less overhead.

### What I did
- Created ticket `FALLLINIE-PERF-009` with `docmgr ticket create-ticket`, a design doc and this diary with `docmgr doc add`, and nine tasks.
- Read `src/renderer.cpp` (`initialize`, `render`, `upload_cell`, `draw_indexed`, `capture`), `src/game_renderer.cpp` (`update_player` streaming window, `render_first_person`), `src/shadow_bounds.hpp`, `src/math.hpp` (`render_frame`), the replay/audio handling in `src/main.cpp`, the PERF-004 counter probe and the GPU-008 campaign scripts.
- Listed the pinned Mali-G52 counter names from `.work/libGPUCounters/docs/mali-g52_counter_reference.html` to choose the geometry set.
- Checked device reachability: `ssh root@192.168.0.202 uname -a` → `Linux RK3566 7.2.7 #1 SMP PREEMPT ... aarch64`; governor `ondemand`; no Falllinie process running.

### Why
- The review identified that the best existing candidate was never measured at the shipping size and that geometry was never examined; measuring first prevents implementing changes that are not needed.
- Opt-in flags with a written promotion rule avoid the earlier pattern where successful candidates stayed opt-in indefinitely.

### What worked
- The GPU-008 scripts (`01-build.sh`, `03-target-case.sh`, `04-capture-case.sh`, `05-campaign.py`, `capture_tools.py`) are directly reusable with a render-size parameter.
- The pinned libGPUCounters checkout exposes all geometry counters needed (`MaliGeomTotalPrim`, `MaliGeomVisiblePrim`, `MaliGeomSampleCullPrim`, `MaliFragRastQd`, `MaliTilerVarShadThread`, ...).

### What didn't work
- No failures in this step.

### What I learned
- The kernel identifies the SoC as RK3566 (`uname -a`), while earlier reports say RK3568. Both have a single-core Mali-G52 and Cortex-A55 cores; the GPU identity used in the analysis (G52 r1p0, one core) comes from the counter library, so no conclusion changes.
- Replay forces `no_audio=true` (`src/main.cpp`), so an audio-on comparison needs a new flag rather than an existing option.
- The receipt's vertex/index counts cover terrain cells only.

### What was tricky to build
- Defining the fog-constant condition required following which eye is uploaded as `uCam`: `render()` uploads `eye=camera_.eye+camera_shake_` in absolute world coordinates, and the fragment shader computes `dist=length(vW-uCam)` in absolute coordinates. The CPU test must use exactly that point, not the render-origin-relative `frame.eye`.

### What warrants a second pair of eyes
- The argument that `mix(lum,1.,1.)` stores 255 in an RGBA8 target regardless of implementation rounding; the visual checkpoints are the practical check.
- Whether a uniform early return in the scene shader changes Mali code generation for the non-flagged path in a way that costs time.

### What should be done in the future
- N/A for this step.

### Code review instructions
- Read `design-doc/01-shipping-configuration-gpu-budget-recovery-design.md`, Sections 3–8.

### Technical details
- Budget: 800 MHz × 16.667 ms = 13.33 M GPU cycles per loop frame.
- Terrain cell: 139 rows × 107 columns = 14,873 vertices; 29,256 triangles; resident window `cell-1 .. cell+5`.
