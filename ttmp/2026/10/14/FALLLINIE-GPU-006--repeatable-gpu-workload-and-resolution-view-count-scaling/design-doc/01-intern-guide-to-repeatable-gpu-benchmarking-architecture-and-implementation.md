---
Title: Intern guide to repeatable GPU benchmarking architecture and implementation
Ticket: FALLLINIE-GPU-006
Status: active
Topics:
    - performance
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://src/game_renderer.cpp
      Note: Deterministic streaming and camera updates
    - Path: repo://src/main.cpp
      Note: Replay policy and actual measurement loop
    - Path: repo://src/terrain.cpp
      Note: Shared terrain row algorithm
ExternalSources: []
Summary: Build a frame-indexed representative rendering workload and use controlled resolution/view scaling to investigate GPU limits.
LastUpdated: 0001-01-01T00:00:00Z
WhatFor: ""
WhenToUse: ""
---

# Repeatable GPU benchmarking: an implementation guide

## 1. Objective and first principles

Falllinie is a native C++ port of a browser downhill-skiing game, running on an RG DS Plus with Cortex-A55 CPUs and a Mali-G52 GPU. We want to understand why reducing CPU rendering work has not consistently reduced the time required to complete a frame. This ticket builds a repeatable live-rendering workload and a small resolution/view-count experiment. It does not promise a particular optimization or a calibrated measurement of GPU utilization.

A frame is one pass through the main loop. The loop updates the simulation and visual state once, renders the top view, optionally renders the lower first-person view, and presents the windows. A view is not a separate game: both views share simulation, geometry and a GL context. All SDL video and GL calls remain on the main thread.

The key change is to distinguish **simulation time** from **measurement time**. A slow GPU configuration must render the same sequence of simulation states as a fast configuration. The benchmark advances simulation by a fixed amount for each frame, while measuring real elapsed time independently. It therefore becomes slower than real time when rendering is slow. That is deliberate: dropping simulation states would make the workload less comparable.

The first implementation is a deterministic generated-input replay, not a recorded human-input file. It uses the existing seeded autopilot and a fixed update step. It retains the real rider, particles, trails, HUD, terrain streaming and both real camera paths. Audio is disabled. The older `--freeze-scene` fixture omits too much of this work to serve as the main benchmark, although it remains useful for small image checks.

## 2. Understand the existing system before editing it

### The main loop and clocks

`src/main.cpp` owns command-line parsing, SDL/GL lifecycle, the game, renderer, audio engine and measurement objects. At the current checkpoint, the frame update near lines164–173 obtains elapsed wall time and passes it through the autopilot and `NativeFrameClock`. The frame clock uses bounded fixed physics ticks internally, but its input still depends on wall time. Consequently two live autoplay runs at different rendering speeds can follow different trajectories.

`src/runtime.hpp` declares `NativeFrameClock`, `FrameResult`, `TitleAutopilot`, input mapping and feedback publication. `src/runtime.cpp` implements them. The autopilot has its own seed-derived RNG. Keeping that RNG and input-update order identical is necessary for a repeatable run.

`src/game.hpp` and `src/game.cpp` own the authoritative game state: position, velocity, score, modes, particles, events and terrain/world access. `Game::set_best_path` loads persistent scores and `save_best` writes them. Benchmark runs must not load or modify the user's score file. Persistence is an environmental input and an unwanted side effect here.

### Rendering and ownership

`src/renderer.hpp` declares `Renderer`, per-cell GPU buffers and cameras. `src/renderer.cpp` submits terrain, objects, rider groups, trails and snow, then applies the print-style post shader and presents the result. The scene pass writes encoded channels and depth, not simply final display color. The post pass interprets those channels to produce the source appearance.

`src/game_renderer.cpp::Renderer::update_player` updates HUD text, mesh residency, rider geometry, particles and camera motion. Its `render_first_person` method temporarily selects the lower-view camera, hides the rider's own face, invokes the common renderer and restores the saved state. That restoration is essential: one versus two views must not change the next simulation state or top-camera trajectory.

The internal render targets may be256×192,384×288 or512×384 while the compositor output surfaces remain at the display size. Reducing internal resolution therefore reduces some pixel work but does not reduce every output or compositor cost.

```text
fixed frame index + seed
          |
          v
seeded autopilot -> game physics and feedback
          |
          v
renderer visual update (once)
  terrain residency / rider / particles / top camera
          |
          +--> scene -> post -> top output
          |
          +--> optional first-person scene -> post -> lower output
          |
          v
real wall/CPU timing + benchmark state signature
```

### Terrain work is currently time-budgeted

`src/terrain.cpp::TerrainJob::step` performs sampling and mesh-emission rows until a wall-time budget expires. `src/game_renderer.cpp` currently calls it with3.5ms. Even after fixing simulation dt, a fast CPU can finish more terrain rows in one frame than a slow CPU. That changes when geometry becomes visible and invalidates strict workload comparisons.

The benchmark needs a fixed **work-unit** budget. One work unit is one sampling row or one emission row in the existing job state machine. A fixed count preserves row order and numerical formulas while making job progress depend on frame count, not wall time. Production keeps its3.5ms policy.

### Current measurement tools

`src/profiler.*` owns frame CSV output and render scopes. `src/paired_clocks.*` samples wall and thread-CPU boundaries of the same scope with fixed capacity and explicit final reconciliation. Nested scene/output scopes must not be added to their containing view scope as if all were independent.

The standalone GPU probe lives under the previous PERF004 ticket's `scripts/03-mali-counters/`. It uses pinned Arm libGPUCounters to collect device activity at roughly10Hz. These are whole-device counters, potentially including compositor work. The backend timestamp does not match host monotonic time; existing analyses use explicitly qualified host-poll intervals rather than silently offsetting the clocks.

The A55 PMU tooling under TRACE005 is complementary. It identifies executing CPU events; it cannot tell us the isolated duration of a GPU pass. GL elapsed queries remain unreliable on this device and will stay off in this campaign.

## 3. What previous experiments establish

The CPU range-draw experiment supplied vertex bounds through `glDrawRangeElements`. It removed a min/max helper from observed driver samples and reduced top-scene CPU time, but did not clearly improve complete-frame timing. Some elapsed time appeared later in output scopes. This supports investigating the rendering path beyond local CPU work, without proving a specific GPU wait.

An earlier post-bypass diagnostic improved frame tails but changed the image by displaying encoded scene data. It identifies a promising subsystem, not an acceptable source-faithful optimization. The current matrix keeps post-processing enabled. A future cheap-copy or constant-fragment substitution must be labeled as a changed-workload experiment.

The new benchmark should answer: does changing internal pixel count or rendering a second view materially change throughput, tails and hardware activity under the same frame-indexed simulation trajectory?

## 4. Proposed benchmark contract

Add an explicit `--benchmark-replay` flag. It is opt-in and does not change the production default.

- Use seed42 by default, or the explicitly supplied seed.
- Force play/autoplay and no-audio mode.
- Require a positive frame limit; reject a wall-duration limit.
- Advance the existing game/frame-clock/autopilot by1/60second per rendered frame.
- Give terrain generation32existing row-work units per frame instead of3.5ms.
- Ignore physical gameplay controls, focus-driven pause and presentation mutations; retain quit handling.
- Disable score-file loading/writing and unstable developer overlay text.
- Keep the ordinary shaders and post pass. Reject incompatible static-fixture, resize-check and post-bypass flags in this benchmark mode.
- Keep actual wall/CPU measurement clocks untouched. `--fps 0` means uncapped; `--fps 60` adds real pacing without changing simulation progression.
- Record the benchmark policy and a trajectory signature in the receipt.

The signature is a consistency check, not a universal determinism proof. Cover selected game position/velocity/time/score and top-camera/residency state across frames, excluding resolution-dependent projection matrices, real timing, output dimensions and worker IDs. Same-aspect-ratio cases should match. Captured final images at the same internal size provide a stronger bounded check of rendering behavior. Do not compare image hashes across different resolutions and expect equality.

The normal game already drains feedback events even when audio has not started. Retain that behavior rather than accumulating events or creating a separate simulation implementation. The benchmark still constructs inert audio state; it excludes active synthesis/device/callback work, not all audio-related storage.

## 5. Representative frame flow and pseudocode

At benchmark frame120, both the fast and slow configurations should have consumed exactly120fixed updates. Their simulation time is independent of the elapsed wall seconds. The second view adds rendering work, not another game update.

```cpp
for (frame = 0; frame < frame_limit; ++frame) {
    wall_start = monotonic_now();
    poll_events();                     // quit remains allowed
    dt = replay ? 1.0 / 60.0 : bounded_wall_delta();
    controls = autopilot.read(game, dt);
    frame_clock.advance(game, controls, dt);
    renderer.update_player(game, dt);   // fixed row budget in replay
    publish_feedback(audio, game, audio_started);

    render_top();
    if (dual_view) render_lower_and_restore_camera();
    apply_real_time_pacing_if_requested();
    record_actual_frame_timings();
    update_selected_state_signature(); // exclude clocks/resolution
}
```

Terrain job flow should share the existing implementation rather than copying its numerical algorithm:

```cpp
bool TerrainJob::step(double budget_ms, unsigned max_work_units = 0) {
    do {
        execute_one_existing_sampling_or_emission_row();
        ++work_units;
    } while (!complete &&
             (max_work_units ? work_units < max_work_units
                             : elapsed_ms() < budget_ms));
    return complete;
}
```

The actual signature computation can use an explicitly ordered byte hash of selected scalar fields, not object memory containing padding or pointers. It is a same-binary, same-platform diagnostic fingerprint. Do not advertise cross-architecture bit identity for floating-point simulation based on it.

## 6. Experiment runner and evidence layout

Use a separate candidate deployment and the existing offline ARM toolchain. Inspect for active game/profiler workloads before launching. Do not replace the production executable, change governors/affinity or install a new driver.

Start with three4:3 internal sizes and one/two views. Hold ordinary indexed draws constant in this first matrix; do not simultaneously add a range-draw factor and make interpretation harder. Run the same fixed frame count, for example600frames, for every case. Discard a declared warmup such as the first60frames from frame summaries. Bracket the matrix with a repeated384×288dual case, and add a paced reference to separate throughput from target-cadence behavior.

```text
counter probe (bounded device capture)
        |
        +--- start game, fixed frame budget
        |       record actual frame CSV / paired scopes / receipt
        |       capture final image outside measured loop
        |
        +--- wait for bounded completion
        +--- collect statuses and frequency/thermal snapshots
        v
analyzer
  validate replay policy and equal signatures
  select qualified counter interval window
  summarize frame distributions and actual elapsed throughput
  retain unsupported/nonfinite/clock limitations
```

Keep raw logs, CSV, receipts and compact captures under this ticket's scripts/evidence tree. Keep binaries/toolchains/downloaded libraries ignored. Store executable/source hashes, exact arguments and counter-probe identity. A successful program exit is necessary but not sufficient: also reconcile paired final counts, GL status and benchmark receipts.

Counter rates should follow the already declared host-poll convention. For selected adjacent host completions entirely inside the usable game window, sum event deltas and divide by summed host intervals. If approximate events per frame are reported later, identify exactly which frame/window denominator is used and disclose compositor contamination. The first implementation need not pretend a10Hzsample belongs to one exact frame.

## 7. Decision records

### Decision: generated-input replay before recorded-input files
- **Context:** The existing autopilot is seeded and exercises real rendering, but wall dt changes trajectories.
- **Options:** Static terrain, recorded human input, or fixed-step seeded autopilot.
- **Decision:** Fixed-step seeded autopilot for the first slice.
- **Rationale:** It reuses current simulation and visual work without a new input-file format.
- **Consequences:** It is one reproducible trajectory, not every representative game state; longer/scenario-specific fixtures remain future work.
- **Status:** accepted for implementation.

### Decision: fixed terrain work units only in benchmark mode
- **Context:** Wall-budgeted streaming makes geometry residency speed-dependent.
- **Options:** Prebuild every possible cell, synchronously finish each cell, or execute a fixed number of existing rows per frame.
- **Decision:** Fixed32row-work units.
- **Rationale:** Preserve bounded work and existing row order without enormous precomputed worlds.
- **Consequences:** Benchmark streaming policy differs from production and must appear in receipts; production time budgeting remains unchanged.
- **Status:** accepted for implementation.

### Decision: measure scaling before adding new profilers
- **Context:** Counters already work, elapsed-query semantics do not, and driverCPU reductions did not clearly improve frames.
- **Decision:** Use current counters and paired clocks for a small scaling matrix.
- **Rationale:** A reproducible workload is more valuable now than an unqualified Streamline/RenderDoc installation.
- **Consequences:** Pass-exact attribution and blocked/runnable distinctions remain unresolved.
- **Status:** accepted.

## 8. Implementation phases and review points

1. **Document and deliver:** create ticket/tasks/diary, write this guide and upload it to reMarkable. The upload is a design delivery, not evidence that implementation is complete.
2. **Replay feature:** add configuration, fixed update dt, fixed terrain work units, disabled environmental inputs and receipt fingerprint. Keep all video work main-thread.
3. **Feature-boundary validation:** build host/ARM, run relevant existingCPUtests and compare same-seed replay receipts across view counts/pacing. Check fixed-work-unit terrain generation matches existing geometry; do not build a large unrelated regression matrix.
4. **Actual device campaign:** execute bounded scaling cases, retain failures and complete evidence, analyze counters and frame distributions. A mismatch in replay signatures blocks treating those cases as the same trajectory.
5. **Checkpoint:** document observations and limitations, commit coherent changes and evidence, and identify the next measured question. Do not automatically promote a rendering shortcut.

## 9. How to interpret the results

A strong response to internal pixel count supports investigating pixel-dependent work, but is not by itself proof of arithmetic versus texture versus bandwidth limitation. A weak response can indicate fixed submission/output costs or another limiting dependency. View count changes both GPU work and main-thread submission, so interpret it with threadCPU measurements.

Do not equate active cycles with productive GPU utilization, a frame interval with physical display latency, or counter-poll frequency with hardware event observation frequency. The compositor can do work even when the game has one window. No-vsync disables the requested swap interval but is not a guarantee that the entire presentation stack is unthrottled.

Paced measurements answer whether frame tails meet the target. Uncapped measurements expose achievable loop throughput under the presentation path. Both are useful; neither substitutes for actual scanout or input-to-photon measurement.

## 10. File and API reference map

- `src/main.cpp`: configuration, loop, dual-window sequencing, actual timing and receipt output.
- `src/runtime.hpp/.cpp`: seeded autopilot, fixed physics accumulator and audio feedback drain.
- `src/game.hpp/.cpp`: authoritative state, RNG-dependent gameplay, particles and persistence.
- `src/game_renderer.cpp`: visual update, cooperative streaming and camera state restoration.
- `src/renderer.hpp/.cpp`: mesh buffers, indexed drawing, scene/post/output and image capture.
- `src/terrain.hpp/.cpp`: row job and deterministic mesh construction; `build_terrain` is the existing synchronous reference.
- `src/profiler.hpp/.cpp` and `src/paired_clocks.hpp/.cpp`: application scopes, sampling cadence and completeness.
- TRACE005 `scripts/10-cpu-pmu`, `11-cpu-attribution`, `13-index-ranges`: previous bounded campaign patterns and source/binary provenance.
- PERF004 `scripts/03-mali-counters`: pinned counter probe, host timestamps and derived data conventions.
- [Arm libGPUCounters](https://github.com/ARM-software/libGPUCounters) and its pinned G52 counter definitions: event domains and derived ratios.
- [SDL_GL_SetSwapInterval](https://wiki.libsdl.org/SDL2/SDL_GL_SetSwapInterval): requested swap pacing, including failure handling.
- [SDL_GL_MakeCurrent](https://wiki.libsdl.org/SDL2/SDL_GL_MakeCurrent): shared-context window switching.
- [glDrawRangeElements](https://registry.khronos.org/OpenGL-Refpages/es3.0/html/glDrawRangeElements.xhtml): the existing optional CPU-side candidate; not another variable in the first matrix.

API links explain contracts, not measured device behavior. Local source and retained target evidence determine what this implementation actually does. Read the current implementation diary alongside this guide when resuming: proposed design becomes implemented behavior only when a checkpoint supplies concrete evidence.

## 11. Implemented and measured checkpoint — 2026-10-14

The replay contract is implemented in `32b8bfd`. Host/ARM builds and 15 CPU tests passed. The eight-case device campaign `target-20261014T180649Z` completed with matching selected-state signatures and same-resolution endpoint images; see [measured results](../analysis/01-measured-replay-resolution-and-view-count-scaling-on-mali-g52.md).

Dual-view mean intervals were 16.62 ms at 256×192, about 17.40 ms at 384×288, and 23.57 ms at 512×384. Single-view cases stayed near 60 completed frames/s despite increasing hardware activity, so application pacing off is not proof of unrestricted presentation throughput. These results qualify the benchmark and motivate further GPU investigation; they do not promote a shader or resolution change into production.

The runner now uses the user's installed Ed25519 key with noninteractive SSH. The previously delivered reMarkable guide is the pre-implementation design edition; this local checkpoint records the later measurement outcome.
