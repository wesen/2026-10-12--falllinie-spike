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

## Step 3: Remove large streaming stalls and establish a paced 60 FPS mode

Continuing the performance requests quoted in Step 2, I tested small changes against the isolated baseline. Precomputing the same 170 sky-hash values preserved the static print image exactly. Batching the native HUD into three draw calls reduced top submission CPU time substantially, but did not by itself fix overall throughput. Disabling vsync merely moved waits into submission, so the root cause was not simply a swap-interval setting.

A lower-resolution control established substantial pixel-work sensitivity: 384×288 per view produced 68.06 FPS unpaced versus about 49.6 FPS at 512×384. I then replaced synchronous four-cell rebuilds with cooperative row generation and individual GPU cell buffers. A 60 FPS paced 384×288 run sustained 60.00 FPS, with p95 interval 16.80 ms and no previous hundred-millisecond cell hitch. Four intervals still exceeded 18.33 ms; this is not a claim of perfect scanout delivery.

### Prompt Context
- User prompts: see Step 2 verbatim requests.
- Assistant interpretation: attribute bottlenecks through controlled measurements, fix evidenced stalls, and preserve source geometry/print semantics.
- Inferred user intent: stable physical gameplay, not a misleading average-FPS claim.
- Earlier code checkpoint: `a7cee69` — profiling, graceful signal handling and single-instance ownership.

### What I did
- CPU-generated the identical integer hash table and uploaded `uSkySamples[170]` once. Kept source shader assets unchanged.
- Replaced per-character uniforms/draws with bounded glyph quads, one VBO upload and three color batches; retained source font, positions and alpha/paper colors.
- Ran CPU-clock perf sampling: 1K samples, zero lost samples; libmali paths appeared prominently, with native player update only 1.70% of sampled CPU time. Unsymbolized Mali addresses are not attributed to named driver functions.
- Added diagnostic `--no-vsync` and explicit `--fps N` pacing, with pacing wait separately logged.
- Added `TerrainJob` row slices, initial four resident GPU cells and one-cell lookahead; generation receives about 1 ms per frame rather than reconstructing all four cells at a boundary.
- Draw only the four visible cells; evict out-of-range cells while retaining initial cells -1..2 for same-seed restart. At most nine GPU cells are retained during forward progress. Changed static rider indices to a single upload.
- Updated the port's default launch mode to 384×288 per view, no EGL-vsync throttle and 60 FPS absolute-deadline pacing. Native executable still defaults to 512×384 for graphics comparison; launch arguments can override resolution/pacing.

### Why
- Hidden instances accounted for the severe 14 FPS result, but isolated 512×384 rendering still exceeded the 60 Hz production budget.
- CPU draw batching alone did not improve enough because driver/GPU/backbuffer throughput still limited production.
- Cooperative generation and per-cell uploads avoid repeated work and bulk uploads at every cell crossing.

### What worked
- `sky-table-pixel-difference.txt` reports `0`: static print pixels match the preceding highp integer-hash version exactly.
- `profile-second-instance.log`: `fatal: another Falllinie instance is running, or the instance lock is unavailable`.
- New sliced terrain test compares every vertex byte and index against the synchronous result. Existing 268 terrain cases, five hash cases, 12 RNG values and six mesh samples pass; host max height error 0, min normal dot 1.
- 25-second paced run: 1496 frames, 1436 measured after warmup; 59.99995 FPS; interval p50 16.66660 ms, p95 16.79785 ms, p99 17.31642 ms, max 19.51303 ms.
- Streaming-work p99 1.08993 ms, max 2.14224 ms, versus prior 85.92 ms rebuild.
- GL error 0; cells 3, 4 and 5 completed during the run, including an actual cell transition.

### What didn't work
- HUD batching reduced top submit p50 from 6.62 to 2.69 ms but total production remained about 49.4 FPS. Swap waits increased; CPU time saved does not necessarily translate into throughput.
- Unpaced 512×384 remained 49.63 FPS, disproving the simple hypothesis that vsync alone was responsible.
- Two exact-text edits missed their expected location (HUD member and terrain file offset); no partial replacement occurred. Read the current declarations and applied corrected edits.

### What I learned
- Rendering resolution is a meaningful performance control on this device. 384×288 is an explicit quality/performance tradeoff, not an unnoticed source-appearance change.
- GPU elapsed queries describe instrumented scopes, not active-cycle utilization; do not add top/bottom values as if all tiler/compositor work were nonoverlapping.

### What was tricky to build
- Incremental row sampling preserves the source's extra neighbor rows, column order, normals, paint attributes and 110.4 m extent.
- GPU resources remain main-thread owned. The lower view shares resident cells and does not generate its own terrain.
- Absolute-deadline pacing recovers small lateness without an unbounded catch-up loop and resets after a long stall.

### What warrants a second pair of eyes
- New-run seed changes must invalidate resident cells/jobs with generation IDs when full source restart semantics are added; the current same-seed reset can reuse pinned initial cells.
- Four late intervals are not zero dropped frames. Actual scanout feedback and the complete audio/object workload are still unqualified.
- Lower resolution affects fine raster detail even though physical print pitch and source font/HUD resolution are preserved.

### What should be done in the future
- Carry these counters into audio, objects and full pose integration; remeasure only the affected major boundary.
- Keep one final integrated physical playtest, including image quality, comfort and sustained load.

### Code review instructions
- Review `TerrainJob`, `Renderer::upload_cell`, residency/eviction in `update_player`, HUD batches and the pacer.
- Reproduce with the dated analyzer and `profile-cooperative.csv`; compare `profile-isolated-cpu-summary.json`, `profile-batched-summary.json`, `profile-unpaced-summary.json` and `profile-lowres-summary.json`.

### Technical details
- New job budget is checked after a complete row; one row may extend the nominal 1 ms budget. Final GPU upload is included in measured stream work.
- Initial terrain construction still occurs before first presentation; this is startup work, not an in-ride boundary rebuild.
- CSV records `pace_wait_ms` separately from lower swap. Full frame intervals include pacing/bookkeeping; CPU stage metrics do not count sleep as rendering.

## Step 4: Deliver report and guide; authorize source-complete integration

The user requested delivery before implementation of the remaining source systems. Both documents have now been uploaded successfully under new names, without overwriting earlier reMarkable documents. The receipts establish cloud delivery, not physical synchronization.

The performance runtime checkpoint is `56d7d9c`; final sampled-GPU and load evidence is preserved in this publication checkpoint. The next work separates the audio engine from the cohesive gameplay/world/pose implementation, using isolated worktrees and a small explicit numeric audio-feedback contract. Integration, review, deployment and acceptance remain parent-owned.

### Prompt Context
**User prompt (verbatim):** "upload to remarkable, then implement Still missing: original reactive audio, full grabs/combos, objects/rails/tree collisions and complete source feedback. The intern guide is drafted but not yet uploaded to reMarkable.

btw commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)"
**Assistant interpretation:** Deliver both current documents first; then complete original source systems rather than replace them with generic mechanics/audio, with scoped commits and detailed evidence.
**Inferred user intent:** A source-faithful complete playable handheld port, not another design-only checkpoint.
**Commit (prior runtime):** `56d7d9c` — bounded terrain streaming and measured 60 FPS launch mode.

### What I did
- Uploaded project report and intern guide using `remarquee upload bundle`, `--toc-depth 2`, `--non-interactive`, and no `--force`.
- Checked guide-delivery task `t9zh`; retained full successful receipts in `report-upload.log` and `guide-upload.log`.
- Preserved final 16-second telemetry: 59.99988 FPS, p95 16.76143 ms, p99 16.89238 ms, max 18.02486 ms; sparse GPU queries had 32 returned samples, two invalid startup values.
- Defined `src/feedback.hpp` as the shared event/continuous-audio-state contract. Audio callback transport must exclude text and allocation.

### Why
- Source gameplay/world/pose are interdependent; audio is independently implementable behind numeric snapshots/events. This avoids concurrent writers on the same renderer or Game class.
- The currently deployed prototype remains available until the integrated candidate passes its relevant build/performance boundary.

### What worked
- `OK: uploaded Falllinie Playable Port - Dual Views and Performance Deep Dive.pdf -> /ai/2026/10/13/FALLLINIE-GAME-002`
- `OK: uploaded Falllinie Source Faithful Gameplay Integration - Intern Guide.pdf -> /ai/2026/10/13/FALLLINIE-GAME-002`
- Upload logs contain no rendering/auth failure; no unnecessary PDF inspection or cloud listing was added.

### What didn't work
- No delivery failure observed. The guide reflects the earlier implementation baseline; the separate project report records subsequent cooperative-streaming/performance changes.

### What I learned
- Final sampled GPU scope medians around 9 ms per view are instrumented elapsed values, not a utilization percentage or values to sum as nonoverlapping work.

### What was tricky to build
- New implementation contracts must preserve sample-clock music scheduling while gameplay can pause or slow down; one audio engine serves both displays.

### What warrants a second pair of eyes
- Source-state completeness, real-time callback safety, restart generation invalidation, full pose geometry and control mappings require independent review.

### What should be done in the future
- Complete source game and audio, integrate once, collect relevant performance evidence, then perform the single final human playtest.

### Code review instructions
- Read the integration guide, frozen source, `feedback.hpp`, and subsequent lane handoffs. Existing graphics/terrain qualification must not be substituted for new-system acceptance.

### Technical details
- Shared event kinds distinguish Pop/Land/Bail/Uber/Grotesk/Grind and UI-only combo/recovery events. Audio transfers only numeric payloads through a bounded SPSC queue.
- Native source, scripts and bounded evidence are committed intentionally; build outputs, shared libraries, secrets and unrelated vault state are excluded.

## Step 5: Recover the gameplay timeout without losing the implementation

The first implementation workflow stopped because the gameplay child reached its exact 1,800,000 ms timeout before committing or producing its handoff. Audio completed, but integration and fresh review did not launch. I preserved the staged patch and repository identity, then resumed the same child through the supported recovery protocol. Both implementation worktrees are now clean and committed; none of those changes has been deployed or accepted as an integrated port.

### Prompt Context
- **User prompt:** Continuing the source-complete implementation authorization recorded in Step 4; the original authored text is not reconstructed here from the compacted summary.
- **Assistant interpretation:** Preserve partially completed work, finish its evidence/handoff, and keep the independent integration/review gates intact.
- **Inferred user intent:** Reliable progress rather than silent loss or an execution-mode workaround.
- **Commit (code):** Gameplay `a30784e`, diary `c2f82da`; audio `d03b756`, `6d75082`, `bcd7704`, `c14eba0`. These remain isolated lane commits.

### What I did
- Inspected workflow `5dc015a1-c84c-4616-b9d6-c77b91669238`, the timeout metadata, worktree refs/HEADs and staged inventory.
- Captured the full staged-inclusive `git diff HEAD --binary` and status under `scripts/2026/10/13/full-gameplay-integration/recovery/`.
- Resumed child `3dc4c461-36f4-4457-ad18-03bf42ca5dc7` through `subagent action=resume`, producing recovery run `d98aab64-fe44-40bb-bf57-a08f4cee96c0`.
- Read both durable handoffs and verified both worktrees are clean.

### Why
- A timeout is an infrastructure blocker, not permission to launch an unmanaged replacement or silently skip acceptance.

### What worked
- Recovery produced two gameplay commits and its detailed diary/handoff; the audio lane supplied four scoped commits and CPU/sanitizer/offline evidence.
- Parent main stayed at `c99143e` while the isolated implementations completed.

### What didn't work
- Exact failure: `Subagent timed out after 1800000ms.` The required gameplay handoff was missing at the initial timeout; integration/review were not launched.
- An initial unstaged-only diff was empty because changes were staged. The subsequent `git diff HEAD --binary` is the complete capture, including staged additions.

### What I learned
- The runtime deadline can interrupt finalization even after tests pass. Earlier coherent implementation checkpoints would reduce recovery overhead.

### What was tricky to build
- Dependent stages must not mistake the recovered child for completion of the already-terminal original workflow.

### What warrants a second pair of eyes
- Gameplay bail/GROTESK snapshot behavior, restart/recovery transient differences, omitted browser UI branches and audio saturation oversampling remain explicitly reported integration/review topics.

### What should be done in the future
- Integrate exact lane commits, review the candidate independently, then qualify the combined target workload. User-reported rendering instability now takes priority before that integration.

### Code review instructions
- Inspect lane diaries `02-source-gameplay-integration-diary.md` and `03-source-audio-integration-diary.md` at their recorded refs; neither host evidence nor a clean worktree proves physical acceptance.

### Technical details
- Gameplay ref `pi-subagents/source-gameplay-3dc4c46-aa40-s0-t0`, HEAD `c2f82da466f53780628fef4e451e54d1f1e48221`.
- Audio ref `pi-subagents/source-audio-29dd4e5-e6b1-s0-t0`, HEAD `c14eba08453700f86f5d9de76c636968e533d548`.

## Step 6: Reproduce long-run rendering precision loss

The user reported unstable texture/blue lines and distant mountains moving vertically. I paused feature integration and captured the running prototype before changing it. Its rider had reached approximately 58.6 km; it was still the pre-integration process, producing about 60 frames/sec. A controlled CPU emulation of shader float arithmetic isolates a serious translation-dependent sky reconstruction error, rather than simply blaming deliberate animated grain or the low render resolution.

### Prompt Context
- **User prompt (verbatim):** "all of a sudden there's a lot of \"jitteriness\" in the texture and lbluelines that appear and disappear. something is off in the rendering..."
- **User prompt (verbatim):** "and the mountains in the back \"jitter\" up and down"
- **User prompt (verbatim):** "btw commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)"
- **Assistant interpretation:** Diagnose and correct rendering instability, preserve evidence, and record substantive debugging in the full investigation format.
- **Inferred user intent:** Stable visuals during extended play, not a cosmetic workaround that masks a numerical defect.
- **Commit (code):** Fix not yet implemented at this checkpoint.

### What I did
- Captured DSI-2/DSI-1 screenshots, process identity, binary SHA256 and a bounded log tail into `scripts/2026/10/13/render-jitter-investigation/`.
- Inspected `math.hpp`, camera matrix creation in both renderer files and the unmodified original vertex/post shaders.
- Added `precision-probe.cpp`: constant camera orientation, 512 offsets spaced 0.001 m around `(0,-60000,60000)`, explicit float multiply/add emulating GLSL without FMA. Compared absolute coordinates with a 128 m snapped render origin.
- Ran `c++ -std=c++17 -O2 -ffp-contract=off -Isrc scripts/2026/10/13/render-jitter-investigation/precision-probe.cpp -o build/jitter/precision-probe` and saved its output.

### Why
- The current matrices are stored/uploaded as float. Large world translations damage both clip-space depth and inverse-VP sky reconstruction; inverting the already-rounded matrix in double does not recover lost information.
- A camera-relative render origin preserves absolute simulation/terrain/source texture coordinates and makes projection arithmetic well-conditioned. Smoothing or disabling grain would not repair depth reconstruction.

### What worked
- Live process was PID 210855; binary SHA256 `908c64d2c0db9662ad5dcd0ee5ceccf6118937dfc56405ded89b38e748928b9a`.
- Log showed rider z=58537.4 through 58645.7 and roughly 60 FPS, distinguishing this report from the earlier contention/stall incident.
- Absolute probe: max ray error `0.0322687073`, adjacent ray jump `0.0617543549`, angular displacement equivalent about `16.4358` pixels at 288 px/62 degrees.
- Relative probe: max ray error `2.96038603e-05`, adjacent jump `3.40007826e-05`, about `0.00904924` pixels.

### What didn't work
- No tool failure occurred in this investigation checkpoint. Existing absolute-coordinate float projection demonstrably fails translation invariance in the CPU reproduction.
- This is CPU arithmetic evidence, not yet a Mali capture or proof that every visible texture artifact shares this cause.

### What I learned
- Short qualification runs near the origin did not exercise the distance-dependent precision failure. The original highp depth-sampler correction cannot fix inaccuracies introduced before rasterization.

### What was tricky to build
- Scene position, post-process camera position, inverse VP and sun projection must use the same render coordinate system. Original world coordinates still feed shading, object models, procedural generation and physics.
- Both displays must independently choose a valid origin and restore the chase camera after the first-person pass.

### What warrants a second pair of eyes
- Origin-boundary continuity, first-person up vector, source shader preservation, depth-space consistency and integration with the unmerged object/pose renderer.

### What should be done in the future
- Implement render rebasing, add targeted depth/sky/boundary tests, capture a far-cell comparison on Mali, then resume source-system integration. Physical confirmation remains pending.

### Code review instructions
- Start with the probe, `math.hpp`, `Renderer::render`, `make_targets`, `update_player` and `render_first_person`. Inspect source shader expressions before modifying runtime source strings.

### Technical details
- Both original shaders remain unchanged at this evidence checkpoint.
- Approximate pixel displacement is ray-vector displacement divided by vertical angular pixel size, not a measured image registration result; float-emulation arithmetic may differ from Mali FMA behavior.

## Step 7: Implement camera-relative projection without changing the world

I implemented a snapped 128 m render origin while preserving absolute coordinates for simulation, geometry storage and source shading. Each view reconstructs its projection/inverse projection with small translations; the scene vertex shader subtracts the same origin only when projecting, and the post shader and sun projection use that relative frame. The original shader files remain byte-identical. Host compilation, the original terrain checks and narrowly targeted sky/boundary probes pass; this checkpoint is not yet a device qualification.

### Prompt Context
- **User prompt:** See Step 6 for the exact rendering report and diary/commit instruction.
- **Assistant interpretation:** Fix the numerical cause rather than suppressing effects or altering simulation.
- **Inferred user intent:** Stable extended runs and traceable fixes.
- **Commit (code):** Evidence checkpoint `44c841d`; implementation hash will be recorded at the next checkpoint.

### What I did
- Added `RenderFrame`/`render_frame` in `src/math.hpp` with rounded 128 m origins and matching relative eye/VP/inverse VP.
- Added a guarded runtime vertex-source substitution for projection only; original `vW` remains absolute for lighting, fog, paint and shadow calculations.
- Updated `Renderer::render` to preserve the active view's rotational up vector, publish the render origin and use relative post/sun inputs.
- Added `--absolute-render-coordinates` for bounded A/B diagnostics and expanded diagnostic cell bounds from 64 to 4096 so cell 536 can reproduce the observed location.
- Extended the CPU probe to exercise production helpers and a snapped-origin boundary with both world-up and rolled first-person up.

### Why
- Shader depth reconstruction and sky reconstruction need well-conditioned projection matrices. Merely using a highp sampler or double-precision inversion after float truncation does not fix the inputs.
- Keeping rebasing inside the render pass avoids changing source generation, physics, caches or the shared simulation, and minimizes conflicts with the unmerged gameplay renderer.

### What worked
- `cmake --build build -j4` passed; `build/test-terrain` passed all 268 terrain cases and existing hash/RNG/mesh/matrix/incremental identity checks.
- Production-helper probe retained the absolute/relative results in Step 6. Origin-boundary max ray jumps were `3.2737144e-05` (world up) and `3.84849886e-05` (rolled up), both below the `1e-4` regression threshold.
- ARM64 Docker cross-link passed against the previously captured target SDL library; no dependency installation or direct GLES linkage was required.
- `node scripts/generate-gl-loader.cjs` reproduced the current 59-entry loader without a change.

### What didn't work
- Earlier evidence commit check initially reported `reference/01-investigation-diary.md:359: new blank line at EOF.` Removed the redundant final blank line, reran the check and committed `44c841d` successfully.
- No implementation compile/probe failure occurred. Mali A/B captures and moving dual-view observation are still pending at this checkpoint.

### What I learned
- Camera-relative rendering can retain source world-space shader inputs; a renderer-only fix need not reset the procedural course or modify deterministic physics.

### What was tricky to build
- The existing VP rotational row provides the currently active view's up basis, including first-person roll, without relying on a separate mode flag. Its translation must not be reused for inversion.
- Sun position must be expressed relative to the same origin as VP; otherwise it would drift independently of the background.

### What warrants a second pair of eyes
- Snapped-origin continuity, GPU float vertex precision at much larger distances, shading/shadow calculations still in absolute space, and preservation of this fix during integration conflict resolution.

### What should be done in the future
- Run one bounded far-cell absolute/relative comparison on Mali, restore the interactive dual-view build, and measure moving render continuity. Do not call the final gameplay playtest complete on this basis.

### Code review instructions
- Review `render_frame`, the guarded vertex-source substitution and `Renderer::render`; run the precision probe and `build/test-terrain`. Confirm no archived shader was edited.

### Technical details
- Physics/world positions remain double where originally defined. GPU mesh attributes retain their original float storage; rebasing fixes projection cancellation but does not restore information already rounded in extremely distant vertices.
- `--absolute-render-coordinates` is diagnostic only; the default is camera-relative. Both view passes independently build frames and continue sharing one context/simulation.

## Step 8: Verify the far-cell fix on Mali and restore the interactive prototype

The selected on-device A/B comparison reproduces dense false blue texture/edge speckle at almost 59 km using absolute coordinates and removes that pattern with relative coordinates. Both runs share the same binary, geometry, source shaders and render resolution. I then restored one interactive dual-view instance with the fix and captured a bounded production-interval sample. This verifies the static far-distance rendering defect and confirms that the repair did not upset the measured frame cadence; dynamic physical horizon acceptance and the integrated game's final human playtest are still distinct, pending gates.

### Prompt Context
- **User prompt:** See Step 6 for the verbatim rendering report and diary/commit instruction.
- **Assistant interpretation:** Check the concrete defect on the actual GPU before resuming feature integration.
- **Inferred user intent:** A real fix with reproducible evidence, not a restart that merely hides the distance-dependent fault.
- **Commit (code):** `7eac73f` — Keep depth and sky projection stable with camera-relative coordinates.

### What I did
- Gracefully terminated only the owned old PID 210855, atomically replaced the executable, then ran exactly two selected static diagnostics at seed 42/cell 536/four cells/384×288/90 frames: absolute and relative coordinates.
- Copied each print/scene/depth BMP, log and receipt into the dated investigation folder.
- Restored the interactive dual-view 384×288/no-vsync/60 FPS prototype; capture-time PID 264668, binary SHA256 `4c997f2f150e80fc3239d58835450e50620693aedfa2f4c580e5f16f575cb4ce`.
- Copied the first 960 frame rows (60 warmup + 900 measured) and ran the existing topic-local profile analyzer; added reproduction instructions in the evidence README.

### Why
- A far-cell comparison isolates coordinate precision from camera movement, procedural differences, resolution or an unrelated feature deployment.
- This is a feature-boundary rendering check, not a repeat of the earlier broad graphics matrix.

### What worked
- Both Mali-G52 receipts report GL_error=0; camera eye `[81.55062225,-18288.21271,58960]` and look `[83.06030725,-18303.44767,58998]` match between cases.
- Absolute `print.bmp` visibly contains dense blue speckle on the terrain banks; relative `print.bmp` retains clean banks and legitimate contour/paint/halftone features.
- Restored live sample: 900 measured frames, 59.999998472 FPS, interval p95=16.736092 ms, zero intervals >18.333 ms and none >33.333 ms. These are production intervals, not physical scanout evidence.

### What didn't work
- Implementation commit initially reported `precision-probe.cpp:13: new blank line at EOF.` Removed the redundant blank line and committed successfully after repeating the whitespace check.
- An attempted read of the historical root alias `scripts/analyze-profile.cjs` returned `ENOENT: no such file or directory`; used the actual dated `scripts/2026/10/13/falllinie-performance/analyze-profile.cjs` instead.
- No GPU/build/runtime failure occurred. Static image evidence cannot independently prove every dynamic mountain movement is gone, and legitimate low-resolution halftone aliasing/animated paper grain remain unchanged.

### What I learned
- The false texture was a long-distance projection/depth defect, not a reason to alter source palettes or remove source print effects. The selected GPU reproduction validates the direction indicated by the CPU arithmetic probe.

### What was tricky to build
- Keep one instance during stop/deploy/diagnostic/restore sequencing, preserve the exact far-location evidence before resetting, and leave unmerged source gameplay/audio branches out of the bug comparison.

### What warrants a second pair of eyes
- Preserve relative scene/post/sun projection in the upcoming source-renderer integration; inspect the actual A/B images rather than infer correctness from GL_error=0 alone.

### What should be done in the future
- Resume integration/fresh review, qualify the combined gameplay/audio/render workload, and retain the single final physical human playtest after integration.

### Code review instructions
- Compare `falllinie-far-absolute/print.bmp` with `falllinie-far-relative/print.bmp` and their matching camera receipts; reproduce with the README commands. Review `fixed-live-summary.json` with its production-versus-presentation caveat.

### Technical details
- Archived shader files are unchanged; the vertex projection substitution is guarded at runtime. Float world-space mesh/shading precision remains a limitation at much more extreme distances.
- The two diagnostic cases deliberately use normal vsync and are not a 60 FPS performance qualification. The subsequent paced dual-view CSV is the relevant regression sample.

## Step 9: Accept the reviewed source integration, correct two notes and build ARM64

The integration workflow and independent read-only review finished successfully on candidate `6338bf5`. The reviewer accepted that exact candidate with two non-blocking source-correctness notes. I verified its full patch hash/applicability and clean worktree, fast-forwarded parent main to the candidate, corrected both notes with targeted regression checks, and independently reran the affected host gates. The complete ARM64 executable now builds, but a fresh SSH check found the handheld unreachable, so no integrated deployment or physical qualification is claimed.

### Prompt Context
- **User prompt:** Continuing the implementation authorization in Step 4 and the verbatim diary/commit instruction in Step 6. The workflow completion and review are runtime evidence, not additional authored user requests.
- **Assistant interpretation:** Parent must inspect/accept the candidate, retain the rendering repair, address small genuine review findings and own final target qualification.
- **Inferred user intent:** Finish the source systems without calling host success physical acceptance.
- **Commit (code):** Reviewed candidate `6338bf566ef96e095729a43609e8e7a0e65b3b26`; parent correction hash is supplied by the next commit/response.

### What I did
- Read the full integration handoff and independent review from workflow `8aa2753a-f96d-4c91-9364-3db00bc07873`.
- Verified candidate patch SHA256 `e6613823e11c564873946e05ee2ad7e7dc06e97538dc07a7e539aef0c155cea3`, `git apply --check` at parent base d911ee4, and clean parent/candidate status; integrated through `git merge --ff-only 6338bf5`, not by also applying the patch.
- Added `audio_detail::acid_shape`: clamp the half-gain acid input to [-1,1] before tanh, matching the source WaveShaper endpoint behavior. Added positive/negative over-range scalar checks.
- Required best-score parsing to end after trailing whitespace; added rejection of `123garbage` and acceptance of trailing whitespace.
- Copied the independent report into `scripts/2026/10/13/full-gameplay-integration/parent-evidence/`; recorded fresh CTest/audio CPU/audio sanitizer/precision output there.
- Built ARM64 with the existing Docker toolchain and captured target SDL library, without installing dependencies or directly linking GLES.

### Why
- The source curves clamp inputs outside their table domain; an unbounded analytic tanh was a real transfer-function mismatch, not merely lookup interpolation.
- Parsing an integer prefix silently accepts malformed persistence data; source numeric conversion rejects malformed suffixes.
- A reviewed worker result still requires parent verification. The old prototype's GPU/audio/performance evidence cannot qualify the substantially larger integrated workload.

### What worked
- Fast-forward integration succeeded after hash/applicability and clean-state checks; original shader/math/jitter evidence remains preserved.
- Fresh parent `cmake -S . -B build/integrated-source`, build and CTest: 6/6 passed (terrain/gameplay/world/pose/integration/dummy audio lifecycle).
- Fresh strict audio CPU and ASan/UBSan runs passed, including source arrangement/IR/reactivity/2x saturation, endpoint check, long-pause transport and watched callback allocations=0. Queue drops/steals printed by the deliberate stress case are not normal-workload failures.
- Fresh production relative-coordinate probe passed for world and rolled up/boundary cases.
- ARM64 cross-link passed; executable SHA256 `a6b940e628ea514f42bb668c6a121b61c630107115cc06dfc0f5eb0362d682e5`.

### What didn't work
- Device read-only preflight failed exactly: `ssh: connect to host 192.168.0.202 port 22: No route to host` (exit 255). No new executable was transferred or launched after this failure.
- A precautionary exact-text edit against an assumed literal newline in the new test returned `Could not find the exact text`; inspection showed the test already contained correctly escaped C++ newline/tab characters, so no repair was needed. Subsequent compile/tests passed.

### What I learned
- The independent review establishes a useful merge boundary, not physical release acceptance. The restored 2x audio graph, full objects/rig and source feedback need a new combined target budget check.

### What was tricky to build
- Keeping world-space source shading while preserving relative scene/post/sun projection during source-renderer conflict resolution; the reviewer explicitly inspected that seam.
- Long pauses suspend callback consumption; effective-snapshot suppression must avoid filling the queue without losing failed-command retry or source event latching. The candidate includes targeted tests for this.

### What warrants a second pair of eyes
- Parent correction tests are independently rerun, but actual SDL backend negotiation, callback budget/underruns, dynamic GPU behavior and controller comfort remain unqualified.
- The read-only reviewer did not execute tests; parent fresh outputs supply that missing executable evidence, not a claim of exhaustive parity.

### What should be done in the future
- Restore console SSH reachability, check whether an emulator is active before any target run, then deploy/qualify the combined source build and perform the single final human playtest. Do not interrupt a user's DS game without agreement.
- Update ticket completion metadata only against concrete acceptance evidence; physical tasks remain open.

### Code review instructions
- Review `parent-evidence/independent-review.md` against candidate 6338bf5, then the small parent acid_shape/persistence diffs and corresponding scalar/malformed-input tests. Reproduce CTest and check-audio.sh with AUDIO_SANITIZE=1.

### Technical details
- The first full candidate includes source gameplay/world/pose/particles/audio/runtime/main/CMake, native title/autopilot, app-owned persistence, bounded SPSC transport, full 2.6-second partitioned IR and preallocated final 2x saturation.
- Native/WebAudio numeric and presentation adaptations remain documented; no waveform identity, browser DOM/touch equivalence or final physical acceptance is claimed. The currently installed binary is still the old prototype with the coordinate fix, not this new executable.

## Step 10: First combined target run exposes an audio deadline failure

The user reopened the handheld and authorized testing. SSH was reachable and no emulator/game process was running, so I atomically deployed the integrated build and ran a bounded 20-second dual-view source-autopilot test. Graphics completed without a GL error, but production cadence was around 50 FPS and the user heard crackling. This is a failed combined qualification, not a successful release or a claim that crackles are intended source glitch sounds.

A second, muted 15-second diagnostic kept the full synthesis workload while measuring actual SDL callback wall time. About half the callbacks exceeded their 512-frame/48 kHz budget, strongly implicating the alternating 1024-frame partitioned convolution burst. I added advisory lock-free callback counters and a muted diagnostic switch; the next step is an affected-path DSP optimization and requested developer overlay.

### Prompt Context
- **User prompt (verbatim):** "I reopened the DS so you can test things"
- **User prompt (verbatim):** "audio is all crackly though"
- **User prompt (verbatim):** "also allow for a dev overlay to be shown (when say, pressing select), with lower level data"
- **Assistant interpretation:** Diagnose real target crackling, expose lower-level telemetry, and add an input-accessible developer overlay without changing source sound design.
- **Inferred user intent:** Smooth audio/rendering and inspectable runtime behavior during handheld testing.
- **Commit (code):** Baseline 24b50d8; instrumentation checkpoint hash follows this entry.

### What I did
- Verified connectivity, ARM64 platform, available storage and absence of emulator/Falllinie processes before deployment.
- Ran integrated --autoplay/dual-screen/384x288/no-vsync/fps60 for 20 seconds with CPU CSV, capture and receipt; copied evidence into target-evidence/initial-run.
- Added AudioDiagnostics callback count/total/max wall time/over-budget count, actual buffer frames and live queue depth through lock-free numeric publication. Clock frequency is cached before callback startup; no callback strings/logging/allocation was added.
- Added --muted for silent full-DSP workload checks and one final main-thread audio diagnostics log line.
- Ran a 15-second muted combined test and copied its evidence into target-evidence/audio-budget-before.

### Why
- x86 callback costs did not qualify this ARM target. Real callback wall time against negotiated sample/buffer sizes directly tests whether DSP can finish within its available interval.
- A muted test avoids repeatedly exposing the user to crackling while preserving instrument/convolver computation.
- Developer telemetry must distinguish measured callback budget misses from actual backend-reported XRUNs; they are not the same counter.

### What worked
- Initial combined run: 926 frames, 866 measured; frame p50=19.8396 ms/p95=22.1455 ms/p99=24.3633 ms, peak RSS=113928 KiB, GL_error=0. Source autopilot produced actual scores/landings and advanced beyond 400 m.
- Instrumentation builds and all six host CTest gates passed; ARM64 cross-link passed.
- Muted target: PulseAudio on PipeWire1.2.6, float32 stereo48kHz, SDL buffer512 (10.6667 ms interval); 1053 callbacks, mean8.84164 ms/max23.9911 ms, 521 over-budget. Queue high2, command drops0, voice steals0, nonfinite samples0.
- Second GPU run also ended GL_error=0; frame p50=19.2869 ms/p95=21.3023 ms. No continuing audible test was left running.

### What didn't work
- User report: "audio is all crackly though". Measured callback maxima exceed the available buffer interval by more than2x; audio acceptance fails.
- Target rendering no longer meets the old prototype's60FPS qualification; source objects/rig/full audio add a material combined workload.
- An initial atomic main.cpp exact-edit call failed because `bool muted=false` was not a standalone declaration; the file actually has a combined declaration. No edits were applied by that failed call; corrected exact matches compiled successfully.

### What I learned
- Bounded, allocation-free audio is necessary but not sufficient: an infrequent large convolution burst can miss deadlines even when average work is under budget.
- Nearly every second callback is over budget, consistent with reverb's1024-frame partition and512-frame callback; further isolation/optimization must verify this mechanism rather than assume it solely from averages.

### What was tricky to build
- Preserve actual source2.6-second stereo IR and music/effect graph while removing computational burst cost; increasing a buffer alone would trade latency without proving adequate headroom.
- SELECT currently means new-run. The requested overlay must rebind it explicitly, retain quit chord precedence and provide a separate new-run action.

### What warrants a second pair of eyes
- Callback timing overhead, advisory snapshot consistency/reset on reopen, source impulse-response preservation and numerical regression if FFT/convolution arithmetic is optimized.

### What should be done in the future
- Optimize and isolate partitioned DSP, rerun one narrow muted target comparison, add developer overlay, then repeat combined qualification before the final human playtest.

### Code review instructions
- Read initial-run/audio-budget-before logs/CSV/receipt, AudioDiagnostics and Impl::callback. Over-budget counters measure callback wall duration, not driver XRUN reports. Check source shapes/oracle and finite/allocation tests after optimization.

### Technical details
- Actual budget is buffer_frames/sample_rate; callback clocks use SDL_GetPerformanceCounter with cached frequency and publish relaxed atomic counters. The sole audio consumer owns accumulation; all formatting/I/O is main-thread-only.
- JS research stays isolated and has no device access. Target tests are bounded, not a restart of the earlier broad graphics matrix.

## Step 11: Reduce DSP burst cost, preserve the source response and isolate remaining headroom risk

I optimized the full convolution rather than substituting a smaller reverb: real-signal half spectra, explicit finite complex arithmetic, planar SIMD-friendly storage, and delayed-partition work distributed through the current input block. The newest partition still waits for its completed input, so the original native1024-sample wet latency stays unchanged. Profiling also exposed time in voice_sample; a compact sorted occupied-slot list now avoids calling that large function for every empty slot on every sample while retaining the original summation/stealing order.

The target trend improved substantially, but the512-frame audio gate is still not passed: the latest30-second run recorded19 over-budget callbacks out of2786, with max15.0859 ms against10.6667 ms. These results are intentionally not described as a complete crackle fix. A more conservative buffer option and external review are next; the requested overlay finished separately and still needs review/integration/device legibility checks.

### Prompt Context
- **User prompt:** See Step10 for the exact crackling and developer-overlay requests, and Step6 for the detailed diary/commit instruction.
- **Assistant interpretation:** Preserve actual source sound design while reducing computation and make remaining target failures explicit.
- **Inferred user intent:** Smooth audio with reproducible lower-level evidence rather than unmeasured changes.
- **Commit (code):** Instrumentation/evidence f1402c7; DSP checkpoint hash follows this entry.

### What I did
- Stored only1025 independent bins of each2048-point real-signal spectrum; rebuilt the Hermitian half before inverse FFT.
- Replaced generic complex multiplication with explicit real arithmetic; separated real/imaginary planes for the multiply-accumulate. GCC optimizes only that bounded kernel atO3, without fast-math/reassociation flags; ARM compiler reports16-byte vectorization.
- Precomputed delayed partitions as their historical inputs are already available, in64-sample work groups. Only current-block FFT/partition0/inverse FFT run at its boundary.
- Added a sorted occupied-voice-slot list, preserving ascending original mixing order and earliest-ending stealing semantics; expired slots retire without allocating.
- Added a non-impulse stereo test against direct source-IR convolution, beyond the previous impulse/FFT tests.
- Ran affected strict CPU/ASan/UBSan gates and selected muted target comparisons; captured a diagnostic perf report and scheduler state.

### Why
- Average time under budget cannot compensate for an alternating oversized block. Delayed partitions can be computed earlier without changing their input history or adding wet latency.
- Interleaved generic complex operations inhibit efficient SIMD; independent planar bins reduce memory traffic/shuffling.
- Calling voice_sample for128 mostly empty slots adds avoidable work and cache pressure; a bounded active list retains the same finite resource limits.

### What worked
- All affected strict CPU and ASan/UBSan runs pass, watched render allocations=0, source arrangement/reactions/rate/block identity intact.
- Full20800-tap/channel8kHz IR reference error remains9.28995e-09; non-impulse stereo convolution max error3.17755e-08 versus direct source response. Tail beyond2 seconds and24-sample2x saturation latency remain tested.
- Recorded target stages at48kHz/512: first half-spectrum run mean6.67424 ms/max19.0377/178of1320 over budget; spread run mean7.61094/max17.6165/112of2754; planar run mean7.73134/max15.5368/99of2752; active/locality run mean6.34155/max15.0859/19of2786. Durations/game states differ, so these are observed diagnostic trends, not controlled equal-work benchmark percentages.
- Latest combined run GL_error=0, frame p50=19.1291 ms/p95=21.0531 ms; production FPS remains around52, not the earlier prototype's60.
- Diagnostic perf recorded1673 samples/no lost samples; voice_sample and planar accumulation were substantial audio-thread hotspots. SDLAudioP2 runs normal time-sharing at nice-20, not realtime; no scheduler/governor policy was changed.

### What didn't work
- The latest512-frame callback test still has19 budget misses; max15.0859 ms exceeds10.6667 ms. No audible success claim or final acceptance is made.
- First spreading reduced deadline misses but did not reduce average cost consistently; smaller work groups disturb voice/cache locality.64-sample groups and occupied slots improve that tradeoff.
- A convenience SCP loop used inferred paths that did not exist; explicit copies from audio-planar/perf-report.txt and audio-active/scheduler.txt succeeded. Only successful evidence copies are retained.

### What I learned
- Both numerical kernels and data-access scheduling matter; spreading work too finely can exchange peak reduction for cache overhead.
- SDL's current audio thread already has nice-20. Avoid blaming low nice priority or changing global scheduling/CPU governors without evidence.

### What was tricky to build
- Tail history excludes the ring slot that will hold the unfinished current block; partitions1..P-1 are valid to precompute, partition0 is not. Restart/init clear all tail accumulators and phase state.
- Active slots remain sorted so different caller block sizes do not change summation order; voice stealing reuses an occupied slot without duplicating its index.

### What warrants a second pair of eyes
- Ring indices, delayed-partition phase, restrict non-alias guarantees, active-list insertion/retirement/stealing and finite-range assumptions need external review despite passing sanitizers/oracles.

### What should be done in the future
- Test a conservative explicit audio-buffer option, review DSP/overlay, integrate the overlay and qualify readability plus audio before asking for the final human playtest.
- Separately address the combined graphics52FPS load without hiding it as an audio fix.

### Code review instructions
- Start with Reverb::process/planar_accumulate, Impl::add/render active-slot loop and test_fft_and_source_ir. Inspect all target-evidence audio-budget-* logs; perf instrumentation run is diagnostic only and not acceptance evidence.

### Technical details
- Full2.6-second stereo response, sample rate, musical arrangement, native wet latency and bounded128voice/256command/64pump limits are unchanged.
- Callback over-budget is wall time versus buffer duration, not actual PipeWire/ALSA XRUN count; prepared-render allocations/nonfinite/drop/steal metrics remain separately reported.

## Step 12: Independent review, explicit buffering and developer panel on Mali

The parent integrated the worker's developer overlay and obtained an independent exact-diff review of both the DSP optimization and overlay at eb6c1a5. No code blockers were found; the reviewer explicitly accepted further qualification, not release. The overlay is now installed on the handheld and captured on the top screen, with the lower first-person view free of developer text.

A new explicit buffer request allows a conservative latency/headroom experiment without changing the source DSP graph. A 55-second muted dual-view run with the overlay and actual1024-frame/48kHz buffer still missed three callback budgets. This is substantially better operational headroom, not proof that audible crackling or the60FPS requirement is resolved.

### Prompt Context
**User prompt (verbatim):** See Step 1 and the overlay worker's diary05. The immediate events were automated worker/reviewer notifications, not newly authored feature requests.
**Assistant interpretation:** Integrate and review the requested SELECT diagnostics, continue measured audio repair, preserve source fidelity and publish honest device evidence.
**Inferred user intent:** A usable native game with inspectable low-level behavior and clean sound, not silently reduced effects or unsupported acceptance claims.

### What I did
- Cherry-picked overlay commits as0a153c7/eb6c1a5; the integrated host suite passed8/8.
- Supplied exact text patches after the read-only reviewer could not execute Git. Archived the patches, change stat and full review under `review-evidence/`.
- Added `AudioEngine::open(int buffer_frames=512)` with power-of-two256..4096 validation before opening; obtained SDL buffer remains diagnostic truth. Added `--audio-buffer`, retaining executable/API512 default. The port launcher requests1024; callers can override it explicitly.
- Extended dummy lifecycle checks for rejecting123 and obtaining1024 after close/reopen. Existing idempotent-open behavior remains intact.
- Rebuilt host and ARM64, deployed to the parent-owned device and ran55seconds muted autoplay/dual384x288/no-vsync/fps60/overlay/1024. Saved logs, CSV and scene/print captures (no --receipt was requested) under `target-evidence/overlay-buffer1024/`.
- Left a separate audible-capable interactive title instance with the overlay enabled, then captured DSI-2 and DSI-1 using grim. Audio remains unopened until A/START drops in; no claim of human listening is made.
- Reviewed the JS research package separately. Its underlying measurements/design were supported, but historical deployment wording and ng capture provenance required correction. Parent corrected those in research-only commit720afc7; no production interpreter was introduced.

### Why
The source reverb's full2.6-second stereo response,1024-sample wet delay, arrangement and2x saturation stay unchanged. A larger callback buffer is a documented latency trade-off rather than a concealed DSP reduction. The developer pane explicitly distinguishes CPU wall intervals and callback budget misses from scanout, utilization and driver XRUNs.

### What worked
- Exact DSP/overlay review found no qualifying code issues: delayed history indexing, Hermitian reconstruction, occupied-voice ordering, callback fixed storage, control rollback/F2 ownership, cached geometry bounds and exception-safe first-person restoration were supported.
- All8 host tests passed after the buffer change; ARM64 compilation/link had no warnings.
- Device: pulseaudio/48kHz/1024frames,2574callbacks,mean12.2459ms,max39.8702ms,3budget misses,queue high3,drops/steals/nonfinite0;2756frames/2696measured,p5019.7951ms,p9521.6306ms,p9923.0993ms,RSS115092KiB,GL_error0.
- Top grim capture shows the bounded source-font panel; lower capture has no developer panel. The GPU scene captures visibly contain tree geometry and persistent trail/particle forms. Screenshot readability is not physical human legibility acceptance.

### What didn't work
- Audio still exceeded its21.3333ms wall budget three times; rendering remained about50FPS. No driver-XRUN or audible-success evidence was produced. The55-second1024-buffer run covers different song phases than earlier30-second512 comparisons, so it is not a controlled same-workload buffer-only benchmark.
- Initial edits to main/lifecycle tests matched two occurrences: `Found 2 occurrences ... Each oldText must be unique.` Those failed calls changed neither file; bounded contextual replacements resolved the ambiguity. Audio header/implementation edits in parallel had succeeded and were verified before compilation.
- The reviewer lacked Git execution by design. Parent-generated immutable patch artifacts resolved this without granting commands or changing agent execution mode.
- Staging initially failed with `fatal: pathspec 'scripts/2026/10/13/full-gameplay-integration/target-evidence/overlay-buffer1024/receipt.json' did not match any files`: no --receipt had been requested. That combined shell lacked failure guards, so the attempted native commit had no staged changes while four unrelated research cherry-picks still succeeded. Verified all eight native modifications remained intact and unstaged, removed the nonexistent staging path and repeated with explicit failure guards. No research/production file overlap occurred. Raw BMPs remain local/ignored; the conversion helper asserts identical decoded PNG pixels before committing bounded captures.

### What I learned
The developer panel makes the remaining cost observable: title captures show top/bottom CPU submit wall time, but that cannot establish additive GPU time. Source terrain residency is seven cells, unlike the earlier four-cell graphics prototype; prior60FPS results are not equivalent integrated-workload acceptance.

### What was tricky to build
Buffer configuration belongs to main-thread device setup, not the callback. Changing an already-open engine still requires close/reopen; the sample clock and DSP graph are not restarted by an idempotent open. SELECT late-chord rollback remains worker-owned logic already reviewed; the new buffer path does not touch it.

### What warrants a second pair of eyes
Physical panel legibility/SELECT chord behavior, sustained audible playback, late callback timing, and combined render/audio performance. The separate reviewer reports preserve exact scope and unresolved qualification gates.

### What should be done in the future
Obtain focused listening feedback from the installed build, identify remaining frame-stage work without removing source features, and qualify combined60FPS/audio only after sufficient evidence. Research corrections do not authorize JS in production or imply JS target execution.

### Code review instructions
Inspect audio.hpp/open setup, main argument/start paths, port launcher and dummy lifecycle test. Run8/8 host suite and bounded target comparisons. Review top/lower PNGs and run.log; do not infer scanout rate or driver XRUNs from the CSV/callback counters.

### Technical details
At48kHz,512 and1024 frames have10.6667ms and21.3333ms nominal buffer durations respectively; actual end-to-end audio latency is not measured. Interactive controls are SELECT overlay, F2 keyboard toggle, SELECT+Y new run and SELECT+START quit. The source-font pane updates at4Hz and is top-only; original mathematical X-ray/DOM/Unicode omissions are not repaired merely by adding developer telemetry.
