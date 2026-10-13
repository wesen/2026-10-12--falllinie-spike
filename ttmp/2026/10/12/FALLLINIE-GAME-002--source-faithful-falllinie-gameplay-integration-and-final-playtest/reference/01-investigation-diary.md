---
Title: Investigation diary
Ticket: FALLLINIE-GAME-002
Status: active
Topics:
    - falllinie
    - graphics
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://scripts/2026/10/13/falllinie-performance/profile-isolated-cpu-summary.json
      Note: Isolated frame timing and cell-build hitch evidence
    - Path: repo://src/main.cpp
      Note: Frame timing, signal handling and single-instance ownership
    - Path: repo://src/profiler.cpp
      Note: Asynchronous sampled GPU queries and CPU CSV logging
ExternalSources: []
Summary: Source-faithful gameplay guide, dual views, sky continuity and measured frame-performance investigation.
LastUpdated: 2026-10-13T01:30:00Z
WhatFor: Preserve implementation decisions, diagnostic evidence and unfinished work.
WhenToUse: Resume native gameplay integration or review the profiling changes.
---

# Investigation diary

## Step 1: Dual views and continuous background contours

The prior requests added a lower-screen first-person view and identified disconnected background mountains. Their exact authored text is unavailable in this compacted context; this entry records the surviving request summary rather than reconstructing a quotation. Both views now use one simulation and one GLES context, made current sequentially to the two windows.

The sky correction exposed a second precision issue: the first unsigned integer hash used the fragment shader's default integer precision and produced almost-flat contours. Explicit `highp uint` restored varied, continuous silhouettes. This deliberately changes the original sine-hash realization; it is not proof of the exact original compiler transformation causing the reported jumps.

### Prompt Context
- User request: compacted summary of lower first-person view and background mountain discontinuities.
- Assistant interpretation: retain chase view above, render the same rider/world below, and address sky continuity without rewriting the terrain.
- Inferred user intent: usable dual-screen gameplay with source-shaped graphics.
- Commit (code): `cec4fcd` — Add lower-screen first-person camera and continuous integer-hashed sky contours.

### What I did
- Added `--dual-screen`, head-relative camera and sequential context binding; launcher enables both views.
- Added wrapped angular sky lattice and `--source-sky-hash` comparison mode.
- Built/deployed, recorded a short automated run and actual DSI-1/DSI-2 captures.
- Drafted a source-complete intern guide, including audio, state routing, pose, objects and cooperative streaming; it is not implementation of those remaining features.

### Why
- One simulation and shared GL resources avoid divergent gameplay and duplicate terrain construction.
- Angular wrapping avoids an atan discontinuity when the rider looks backward.

### What worked
- Two views rendered; placement commands succeeded; short receipt reported GL error 0.
- `evidence/stable-sky.png` shows varied, continuous corrected silhouettes.

### What didn't work
- Initial default-precision hash flattened the mountain noise: see `evidence/sky-first-attempt-low-precision.png`; fixed with explicit high precision.
- The initial eight-second performance receipt was later found contaminated by older background app instances. It must not be used as an isolated benchmark.

### What I learned
- An image and a zero GL error do not establish numerical precision or isolated performance.

### What was tricky to build
- The lower pass restores camera state; capture redraws the primary view because offscreen targets are shared.

### What warrants a second pair of eyes
- Head-versus-travel orientation during fakie/flips and nausea risk need human feedback.

### What should be done in the future
- Replace the baked head anchor with the runtime rig's head transform; retain one final integrated physical playtest.

### Code review instructions
- Review `src/main.cpp`, `Renderer::render_first_person` and compile-input sky adaptation in `src/renderer.cpp`.

### Technical details
- Top DSI-2, lower DSI-1; lower FOV 78 degrees; rider hidden in first-person pass.

## Step 2: CPU/GPU profiling and hidden-instance contention

The user explicitly expanded validation to investigate frame loss and authorized performance counters and tracing. I added CPU stage CSVs, sampled asynchronous GPU elapsed queries, rolling FPS, a read-only system sampler and a reusable summary command. Profiling immediately exposed both a real cell-generation hitch and major interference from old app processes.

Three older processes were still rendering. Background shell launches inherited ignored SIGINT; `pkill -INT -x falllinie-spike` did not stop them. Sending SIGTERM removed them. The native app now installs explicit graceful handlers and holds a nonblocking single-instance lock, preventing future hidden rendering copies.

### Prompt Context
**User prompts (verbatim):**
- "make sure we don't drop frames / figure out why we drop them, as well. collect FPS etc..."
- "you can use performance register / whatever it takes to measure CPU and GPU and other load issues"
- "perf ebpf etc... whatever you want and gets you there"
- "btw commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)"

**Assistant interpretation:** Measure frame intervals and CPU/GPU/load causes rather than infer smoothness from average FPS; keep scoped commits and a detailed investigation record.
**Inferred user intent:** Smooth physical gameplay and an evidence-backed explanation of stutters.

### What I did
- Added `src/profiler.{hpp,cpp}`: buffered CPU CSV, eight query slots, query availability checks, no query-result waits or `glFinish`.
- GPU queries are opt-in, sampled every 60 frames, and invalidated for disjoint or implausible results over 500 ms.
- Added events, simulation, player update, cell rebuild/upload, top/bottom submit and swap timing.
- Added rolling HUD/console FPS; `scripts/sample-load.sh` reads CPU/GPU clocks, temperatures and process/system CPU counters.
- Collected `perf stat` hardware cycles, instructions/cache misses and scheduling/page-fault counters.
- Collected matched CPU-only and sampled-GPU runs, first with accidental interference, then after eliminating old processes.

### Why
- CPU draw submission includes possible driver backpressure, not just application arithmetic.
- GPU queries can perturb tile rendering; a CPU-only control is required.
- Nonblocking queries avoid making the profiler itself synchronously wait for the GPU.

### What worked
- Isolated CPU-only run: 47.33 FPS after 60 warmup frames; interval p50 20.91 ms, p95 24.66 ms, p99 26.92 ms.
- Hidden-instance run: 14.13 FPS, p95 90.29 ms. Removing those instances is a concrete large improvement, but not proof of 60 Hz production.
- Isolated simulation p95 0.165 ms; player update ordinarily under 1 ms.
- Cell transition frame 679: 125.99 ms interval; synchronous rebuild/upload 85.92 ms.
- Both isolated run receipts report GL error 0.
- Hardware PMU events work; CPU clock 1.992 GHz and GPU clock 800 MHz were observed. Clock readings alone are not utilization.

### What didn't work
- Exact command `pkill -INT -x falllinie-spike` left PIDs 162003, 172380 and 173481 alive; fixed through SIGTERM cleanup and explicit native signal handling.
- GPU timer startup returned approximately `1.84467441e+13` ms with disjoint flag 0. These are invalid results, not GPU measurements; added range rejection.
- `perf stat ... -e ...cycles...` also reported `<not supported> rockchip_ddr/cycles/`; do not claim DDR counter coverage.
- Initial build emitted `warning: this 'if' clause does not guard... [-Wmisleading-indentation]` at `src/profiler.cpp:19`; split the statements.
- An exact-text edit failed with `Could not find edits[2] ... oldText must match exactly`; no partial edit was applied, and a corrected batch succeeded.
- No bpftrace executable or readable kernel BTF was discovered. Existing perf and GLES timers were sufficient for the initial attribution; no arbitrary registers or driver policies were changed.

### What I learned
- The initial benchmark setup was not isolated. Old graphics processes can dominate driver submission times without expensive physics.
- The remaining ordinary frame cost is rendering/swap pacing; the largest isolated hitch is synchronous cell construction.

### What was tricky to build
- Queries and their destruction must occur while the GL context is current. Sampling is opt-in, and unavailable results are skipped rather than waited on.
- The lock uses close-on-exec and no-follow flags; the signal handler only writes `sig_atomic_t`, with orderly cleanup on the main loop.

### What warrants a second pair of eyes
- Vendor GPU elapsed queries have observable startup anomalies; sample values are instrumented elapsed observations, not trustworthy GPU active-cycle utilization.
- App-produced intervals are not compositor presentation feedback. Do not label the budget-miss count as physically confirmed dropped scanouts.

### What should be done in the future
- Reduce ordinary rendering cost and replace full synchronous cell rebuilds with bounded cooperative generation.
- Investigate actual presentation feedback if physical dropped-frame counts, rather than app production deadlines, are required.

### Code review instructions
- Review `Profiler`, signal/lock lifecycle and stage boundaries in `src/main.cpp`.
- `node scripts/2026/10/13/falllinie-performance/analyze-profile.cjs scripts/2026/10/13/falllinie-performance/profile-isolated-cpu.csv` reproduces the baseline summary; compare `profile-isolated-gpu-summary.json`.

### Technical details
- `--profile /tmp/frames.csv` enables CPU logs; add `--gpu-profile` for sampled EXT elapsed queries.
- Warmup exclusion is 60 frames. Stream time is a subset of player time; do not sum them twice.
- Raw performance evidence and reusable analysis/sampling scripts are under `scripts/2026/10/13/falllinie-performance/`; initial graphics captures remain under `evidence/`.
