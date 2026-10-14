# PERF-004 measurement tools

**Target measurements PENDING.** Host llvmpipe checks are functional gates, not Mali performance results. No production optimization is selected here. Read the [contract](../design-doc/01-measurement-contract-and-optimization-investigation.md), [cost map](../design-doc/02-render-cost-map-and-candidate-optimizations.md) and [diary](../reference/02-measurement-tooling-diary.md).

## Host validation

From the native repository root:

```sh
T=ttmp/2026/10/13/FALLLINIE-PERF-004--gl-pass-measurement-and-evidence-driven-native-optimization
sh "$T/scripts/build-host.sh"
python3 "$T/scripts/smoke-host.py"
python3 "$T/scripts/plan.py" > "$T/scripts/.work/plan.csv"
```

Uses only installed CMake/compiler/SDL/Python and existing Xvfb/Mesa. No downloads. Build/test/smoke artifacts and transient preference files stay under ignored `scripts/.work/`. `evidence/` contains bounded checked-in host evidence, not binaries or captures. CPU suite is the existing 12 tests plus fake-query lifecycle test; Python analyzer tests run separately. Smoke: 8 frames/case, queries off single/dual, pass and whole-view dual, real post bypass, live dual/dynamic scopes; GL error zero, POST-RUN UNINSTRUMENTED FIXTURE REPEATABILITY of top captures, invalid CLI rejected. Main detaches measurement and redraws before capture: these images do **not** test query-active image equivalence, which is explicitly deferred. Eight frames is deliberately **not a performance sample**. No ARM commands have been run by worker.

## CLI contract

- `--profile CSV`: existing frame CPU CSV plus `.passes.csv`, `.gpu.csv`, `.stats.csv` siblings. No profile path: no clocks, query allocation/polling or file writes inside renderer scopes.
- `--gpu-profile-mode off|view|passes` (default `off`). Existing `--gpu-profile` means `view`. A GPU mode requires `--profile`; last mode option wins. `view` and `passes` never run nested elapsed queries. GPU CSV schema intentionally changed from the old four-column file; do not use the old analyzer.
- `--profile-cadence 1..3600` (default 60): sample frame numbers divisible by cadence, for **both** CPU pass scopes and selected GPU scopes. Frame CPU CSV still records all frames. 1 is useful only for bounded smoke; default target comparisons use 60.
- `--freeze-scene`: **static seeded terrain fixture**, not a frozen full game/replay. Initializes `--seed`, `--cell`, `--cells` and a fixed camera. No frame simulation, `update_player`, camera/RNG advancement, streamed cell jobs, objects, rider, snow, trails, gameplay HUD or audio DSP. Both windows intentionally render the **same initialized camera**. `--dual-screen` normally implies play, but this explicit fixture overrides that implicit implication; explicit `--play`, `--autoplay`, `--dev-overlay`, `--resize-check` conflict and are rejected in either order. Input mutations disabled, quit retained. Explicit seed/palette/debug/internal size are fixed for the run. Receipt/stdout label exclusions and same-camera lower view. Game-related initialization still occurs once; frame workload is frozen.
- `--post-bypass`: non-parity raw encoded scene color instead of printed image. Entire post framebuffer binding, uniforms and draw are skipped. Scene color/depth cleared normally; output copy binds fullscreen VAO and disables depth explicitly. Output/HUD still run. `--debug-view` alone **does not** skip post. Bypass captures use scene FBO, not stale printed texture. Never compare its appearance as a parity improvement.
- `--no-vsync`: now honored on initial single-window setup too (previously only dual loop honored it). Default vsync unchanged. Requested mode can still be rejected by platform; inspect warnings and actual interval.

Receipt adds `measurement` configuration. Existing receipt `source_sha256` identifies historical source asset provenance, **not candidate binary/source revision**; use recorded `source-revision.txt`, metadata and binary SHA256. Existing `timing_target_met` uses the old 33.3ms spike threshold and is **not** a 60FPS acceptance gate. Use frame CSV with `frame >= 60` warmup; old receipt summary has historical indexing conventions. Script metadata/stdout/receipt must travel together.

## Timing and counters

- `passes.csv`: `frame,view,pass,parent,cpu_ms`. `view` contains `scene`, `post`, `output`; dynamic preparation/upload are **CPU-only children of scene**. They separate snow+trail mesh construction from `glBufferData` wall time on sampled live frames. Player rig updates, rider upload and streamed-cell uploads remain in main `player_ms` (including `stream_ms`), **outside render scopes**. Terrain/objects/rider GPU cost is combined in scene. Output includes full drawable copy, top HUD and developer overlay. No draw/byte counters were added.
- Scope stop timestamp is cached **before** ending query/queuing CPU sample; no CSV writes occur inside renderer scopes. Fixed 32-entry CPU buffer flushes at next frame poll or destruction. CPU view contains nested scope/query overhead and excludes first-person camera wrapper setup; main bottom submit includes that wrapper and `MakeCurrent`. Top events includes context restoration, query polling and prior pass logging. Frame interval includes previous frame logging/bookkeeping. These remain wall intervals, not GPU utilization. Do not add children to parents or stream to player.
- At sampled frames dynamic measurement holds both snow/trail meshes briefly before their uploads; unprofiled and unsampled paths keep the original evaluation/upload order. This observer allocation/scheduling difference is an additional limit; compare unprofiled versus profiled-off if pursuing sub-ms CPU changes. Off/on query comparison holds CPU telemetry fixed.
- 32 GPU slots, first-free allocation (not modulo reuse); each owns original frame/view/pass until available. Poll once per frame, check availability before `GetQueryObjectui64vEXT`, no wait/spin/finish. A disjoint observation invalidates all pending/active samples conservatively. Invalid samples retire without fetching numeric result. No arbitrary duration clamp (old >500ms invalidation removed).
- `gpu.csv`: `frame,view,pass,status,gpu_ms`; statuses `valid`, `disjoint`, `pending`, `pending_disjoint`. Invalid and pending rows have **blank** time. Unresolved queries get terminal rows on orderly shutdown and are deleted without waiting. Unsupported/off emits header only, not numeric zeros.
- `stats.csv`: final one-row mode/cadence/support, issued, valid, disjoint, unavailable **poll attempts** (not distinct missed samples), skipped pool opportunities, unsupported selected scopes, misuse, disjoint polls, pending at shutdown, CPU buffer drops, `schema_version=2` and `cpu_samples` (scope rows emitted/attempted at flush, including the final destructor flush). The analyzer reconciles this count against unique CPU scope records and rejects any drops, orphan parent or issued GPU scope without a matching CPU scope, even for pending/disjoint GPU status. Pending-disjoint contributes to pending, not retired-disjoint. Skipped/unsupported are counters, not fabricated measurements. Mode `off` support=0 means **not probed**, not a hardware assertion. Incomplete shutdown lacks stats and analyzer rejects it. Sparse sampling may miss bursts; report counts and cadence.
- RAII ends active scopes during exceptions. One context shared between windows; scopes close before any switch; first-person state restoration remains existing try/catch. Profiler lives inside the context-owning `try`, destructs before renderer and before outer SDL context deletion. No GL calls on workers.

## Parent-only offline ARM build

Do not run the old `scripts/build-device.sh`: it acquires target libraries over SSH and builds an image. The new explicit-input helper uses an **already existing** image without network/pull or copying libraries:

```sh
# Parent approves exact existing library directory first; worker has not copied it.
sh "$T/scripts/build-parent-arm.sh" /absolute/approved/native-repo/target-libs
# Outputs: $T/scripts/.work/arm/{falllinie-spike,build.log,elf.txt,compiler.txt,...}
```

Parent reviews/cross-compiles/integrates main-loop edits, deploys a separately named candidate directory (not `/storage/apps/falllinie-spike` production), records actual source/binary hashes and validates target SDL/context/query entry points. This helper is syntax checked only here; toolchain/target-library compatibility remains a parent gate.

## Parent-only bounded target comparisons

`run-one.sh` executes **locally on the selected machine**, never SSHes, installs, stops services, changes governor/FIFO/affinity or chooses a fallback. Parent must first inspect live game/services and obtain a safe window; existing native instance lock prevents concurrent games. Do not force removal/termination. Preserve saves; for live runs the parent may explicitly provide a separate `XDG_DATA_HOME` after checking device SDL behavior. Do not change audio mode on failure.

The script takes six arguments: absolute candidate binary, absolute shader assets directory, **new** absolute output directory (parent exists), case, exact source revision, order/repetition ID. It refuses overwrites. `SECONDS_PER_RUN` allows 10/20/30/60/120 (default 30), with app-internal bounded runtime. Parent should additionally use a **verified device-supported** external timeout wrapper; BusyBox `timeout`/`flock` variants are not assumed. `PLACE=1` explicitly enables existing placement IPC; default leaves layout untouched. No service cleanup is automatic.

Example **on target, by parent only** after separate deployment:

```sh
SECONDS_PER_RUN=30 PLACE=1 sh /candidate/perf-scripts/run-one.sh \
 /candidate/falllinie-spike /candidate/assets/shaders \
 /candidate/evidence/01-static-off static-off EXACT_SOURCE_REV 01-rep1
# Use plan.csv order; each output directory is distinct.
```

The 14-run printed plan is small and balanced: 5 static cases forward then reverse (10 runs), followed by live off/on/on/off (4 runs). Static cases: dual384 queries-off baseline, matched queries-passes, single384 passes, dual384 post bypass, dual512x384 passes. Each maintains seed/cell/cells/fps60/no-vsync, 1024x768 requested output, cadence60. Live phase is autoplay/dual384, buffered48k requested via existing audio engine, buffer1024, muted **full DSP**, vinyl on, developer overlay on. Same seed is **not** identical trajectory under variable render intervals; compare distributions and ride/stream context rather than subtracting individual live frames. Target rate/output sizes and audio-open success must be verified, not assumed from arguments.

Every run records ordered argv (one argument per line), source label, binary SHA256, UTC/uname, before/after memory, CPU policy min/max/current/governor and thermal snapshots where readable, receipt, renderer/version/extensions/caps, profile files and final audio counters. Snapshots are boundaries, **not continuous frequency traces or utilization**. Audio diagnostics absence or `audio unavailable` invalidates live full-DSP comparison. Missing stats/abrupt timeout invalidates query accounting. Cap/snapshot logs do not establish headroom.

Copy bounded results back under this ticket's `scripts/target/...` by parent; analyze on host (target need not have Python):

```sh
python3 "$T/scripts/analyze.py" "$T/scripts/target/01-static-off/frames.csv" --warmup 60 \
 > "$T/scripts/target/01-static-off/analysis.json"
```

Stats schema2 is required for complete CPU-scope evidence. Historical unversioned files are explicitly schema1, which lacks a CPU-sample count; analysis fails closed with a named legacy-version diagnostic and asks for regeneration with schema2. **Do not rewrite raw old receipts/stats to add a guessed count.** Their raw timing rows remain historical evidence, but cannot be recertified complete by this analyzer. Existing receipt JSON format itself is unchanged. See [review correction annotation](evidence/review-corrections.md).

Analyzer reports each independent/nested scope distribution with sample counts/mean/p50/p95/p99, invalid/pending status counts and entire final query stats. It rejects nonfinite/negative/blank valid times, invalid samples with numeric times, duplicate/nested GPU scopes, mismatched issued/CPU-sample counts, missing CPU parents or GPU counterparts (all terminal statuses), sampled frames missing the top CPU view, misuse/drop and incomplete files. Genuine unsupported GPU modes remain valid when CPU records are complete; absence of GPU rows alone is not an error. It **never sums nested costs or combines modes**, never substitutes zero for empty GPU data. Compare off/on frame intervals first; if query overhead is material, lower cadence and rerun balanced pairs before interpreting attribution. Repeat representative live runs before parent accepts a culprit/optimization.
