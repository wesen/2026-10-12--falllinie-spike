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
