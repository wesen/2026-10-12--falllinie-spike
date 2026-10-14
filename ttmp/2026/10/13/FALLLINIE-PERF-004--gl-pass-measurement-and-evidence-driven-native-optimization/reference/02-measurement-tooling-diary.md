---
Title: Measurement tooling investigation diary
Ticket: FALLLINIE-PERF-004
Status: active
Topics: [performance]
DocType: reference
Intent: long-term
Summary: Worker implementation, source investigation, host-only evidence and explicit parent target gates.
---

## Step 1: Establish nonnested query attribution and a reproducible static fixture

Read AGENTS, [the measurement contract](../design-doc/01-measurement-contract-and-optimization-investigation.md), parent reference01 Step1, the complete original profiler/main/renderer/game_renderer source, GL dispatch, particle/pose/terrain/world code and actual shader bodies. The old profiler was asynchronous but only measured whole views; wrapping elapsed pass queries inside it would violate GL's single active elapsed target. The source map also showed that a so-called frozen autoplay run would not automatically be reproducible: renderer streaming uses wall-budgeted jobs and trajectories depend on elapsed time.

Implemented explicit off/view/passes granularity, a bounded first-free32-slot query pool, sparse renderer CPU scopes, deterministic static terrain diagnostic and a real omitted post pass. Supervisor approved the static terrain limitation through the live decision channel: it is not frozen representative gameplay, same-camera lower view is intentional, and separate live measurements are required before final ranking. Production paths remain the original renderer/simulation/shaders; the only non-diagnostic behavior correction is respecting explicit single-window `--no-vsync`, which the old setup ignored.

### Prompt Context
**User prompt (verbatim):** "Ok, let's do the GL performance measurement and figuring out ways to make this all tighter / finding the different potential culprits / optimizations / parallelizations / precomputation points."
**Assistant interpretation:** Build narrowly scoped measurement/diagnostic tooling, map concrete source costs and candidate reuse/parallelization seams, and leave actual target operations and optimization acceptance to parent.
**Inferred user intent:** Find evidence-backed ways to recover frame budget without changing source appearance/behavior or guessing that GPU-bound work is cheap physics.
**Commit (code):** `ac7b0b7` — "Add bounded GL pass telemetry and controlled diagnostic fixtures". Work began at `1802822fc688e3c8bbcd983e59bc17535e1feadc` in the isolated one-writer worktree.

### What I did
- Replaced whole-view modulo-indexed query reuse with32 first-free slots carrying original frame/view/pass, active-owner tracking and explicit counters. Kept EXT entry dispatch optional; introduced injectable `QueryApi` for deterministic tests without a context.
- Added `Profiler::Scope` RAII. Only selected view or scene/post/output scopes issue GPU elapsed queries. Dynamic prep/upload are CPU-only nested scene children. Sparse CPU samples use a fixed32-entry queue; timestamps are taken before query end/queueing, logs flush at next poll or destruction rather than within renderer scope.
- CPU frame CSV remains unchanged. New GPU schema uses status plus blank time for disjoint/pending. Stats records mode/cadence, supported, issued/valid/disjoint/unavailable-polls/skipped-pool/unsupported/misuse/disjoint-polls/pending/CPU-dropped. Unsupported does not manufacture zero timing; off means unprobed.
- Poll checks availability first and never waits/spins; observed disjoint invalidates pending/active queries conservatively. Invalid results are retired without numeric read. Removed old arbitrary500ms invalidation. Destructor polls once, emits unresolved terminal rows and deletes IDs with context still alive.
- Instrumented `Renderer::render` scene/post/output and dynamic preparation/upload. No shader changes, new GL threads, render workers, geometry decimation or game/audio modifications. First-person save/restore remains unchanged and scopes finish before main switches window.
- Added CLI `--gpu-profile-mode`, `--profile-cadence`, `--freeze-scene`, `--post-bypass`; retained `--gpu-profile` as view alias. Static fixture rejects explicit play/autoplay/dev-overlay/resize-check, overrides dual's implicit play only, ignores input mutations except quit and never calls frame simulation/update_player. It renders fixed initialized terrain in both windows, with no rider/dynamics/objects/HUD/stream/audio-DSP coverage.
- Post bypass skips post FBO/uniforms/draw, uses scene texture for output/capture, establishes fullscreen VAO and disabled output depth. Existing debug texture selection alone still performs post and is not marketed as a bypass.
- Added renderer/version/extensions/limits and diagnostic metadata, retaining exact main CPU ownership and existing audio diagnostics. Fixed initial `SDL_GL_SetSwapInterval` to honor `--no-vsync` on single-window comparisons; production default remains vsync requested.

### Why
- Elapsed queries cannot nest. Separate modes are easier to audit than interleaved, implicit schedules.
- Sparse fixed pools bound observer work and RAM; pending queries do not stall the game. Explicit unsupported/disjoint/pending records prevent false GPU-zero conclusions.
- Static terrain gives identical initialized geometry/camera/time across off/on, view-count and resolution tests, unlike wall-time autoplay. It cannot answer snow/trail/stream/audio questions, so that limitation is explicit in stdout/receipt/docs.
- Real bypass is causal diagnostic tooling. Showing a debug texture **after** post would not test post cost.

### What worked
- Initial Release build completed with installed host toolchain; existing12 CPU tests and new profiler test passed13/13.
- Fake query test proved pool saturation/skipping, no result read before availability, invalid result retirement, frame/view mapping, cadence, nonnesting, unsupported/off behavior, misuse suppression, pending-at-shutdown and exception scope unwind. Later extension also covers disjoint while active.
- Supervisor approved the exact static fixture scope and reiterated target/live acceptance gates. Progress update recorded the single-window no-vsync finding rather than silently treating it as a performance gain.

### What didn't work
- No unexpected build/test failure observed in this step. No target execution, SSH, installs, target-library acquisition, Docker execution or deployment was attempted; those gates are **not run**, not passing results.
- The proposed generic "freeze a live scene" interpretation was insufficiently deterministic because wall-budgeted streaming can complete different work. It was replaced **before implementation**, with supervisor approval, by the explicitly smaller static-terrain fixture rather than silently widening simulation control.

### What I learned
- Snow geometry depends on each view's eye/basis; trail geometry does not. Reusing both indiscriminately would break fidelity.
- Rider mesh templates are already precomputed; indexes/groups still regenerate but zero-length segment omissions mean topology is not trivially invariant.
- Terrain/World have mutable caches, cumulative state and references; current `TerrainJob(Terrain&)` is not a safe worker job despite deterministic local seeding.
- Context/swap boundary costs and driver backpressure remain distinct from GPU timer elapsed. Bottom submit contains context/head-camera overhead, while player includes stream/rider uploads outside new renderer probes.

### What was tricky to build
- A selected elapsed owner must survive suppressed nested misuse: inner scope with no token must not end outer query. Scope token and single active slot make this explicit and fake tests assert backend cannot nest.
- Exception unwinding must end elapsed query before caller restores first-person state/context. Profiler is destroyed inside the context-owning try before SDL teardown; scopes never cross make-current calls.
- Bypassing post removed the normal path that disabled depth and selected fullscreen VAO. Output now establishes those states explicitly **when bypassing**, avoiding stale depth/VAO output without adding redundant normal-path GL calls.
- Sampled dynamic CPU separation briefly retains both snow/trail meshes before uploads. Unprofiled and unsampled paths preserve original expression/upload order. This observer effect is documented, not sold as a free optimization; query off/on comparisons hold it fixed.

### What warrants a second pair of eyes
- Actual target EXT implementation, disjoint behavior and pool availability; no llvmpipe result establishes Mali support/overhead.
- Final context validity on platform errors and shutdown; normal and render-exception scope closure are covered, SDL driver failure recovery remains parent hardware gate.
- Source fidelity of bypass as **non-parity** diagnostic, and static fixture metadata being preserved with CSVs rather than interpreted as gameplay performance.
- Changed GPU CSV schema; old AUDIO-era readers must not consume it as the historical four-column format.

### What should be done in the future
- Parent/reviewer inspect exact candidate, cross-build offline using explicitly approved target libs, smoke device, then balanced off/on before attribution. Do not select production changes from these host tests.
- Expand terrain/objects/HUD/context/draw-byte probes only if scene/post/output and live dynamic evidence warrants it; no broad benchmark framework was added.

### Code review instructions
- Start [profiler.hpp](../../../../../../src/profiler.hpp), [profiler.cpp](../../../../../../src/profiler.cpp), [test_profiler.cpp](../../../../../../tests/test_profiler.cpp).
- Inspect [renderer.cpp](../../../../../../src/renderer.cpp) scope and bypass braces, [main.cpp](../../../../../../src/main.cpp) CLI/freeze/context ownership, and unchanged [game_renderer.cpp](../../../../../../src/game_renderer.cpp) restoration/update path.
- Commands: `sh "$T/scripts/build-host.sh"`; `python3 "$T/scripts/smoke-host.py"`; all work under `$T/scripts/.work`. See [CLI/counter contracts](../scripts/README.md).

### Technical details
- Default mode off, cadence60,32 GPU slots,32 queued CPU records. Two-view live sampled frame normally uses12 CPU records and6 pass queries (or2 view queries), so pool exhaustion skips instead of growing or waiting. Null profile path performs no timer clocks/query I/O in scopes.
- View CPU contains scene/post/output; dynamic prep/upload contain only snow/trail construction or upload wall, not rider/stream. No nested durations are added by analyzer. Main player already includes stream.
- Baseline query file had no samples; prior top9.5481/bottom8.1037ms are CPU wall only. Expected60Hz16.6667ms versus historical19.7839ms mean remains motivation, not a measured improvement.

## Step 2: Validate host paths and build a bounded parent investigation workflow

Built reusable ticket-local host validation, parent-only offline ARM build recipe, local bounded target case runner, balanced order printer and strict analysis tools. Ran the host functional gate with existing Xvfb/Mesa, preserving all useful build/test/run files beneath this ticket and copying only bounded evidence out of ignored work directories. No device commands were executed and no third-party binaries were downloaded/copied/committed.

The [source cost map](../design-doc/02-render-cost-map-and-candidate-optimizations.md) now separates source facts, historical CPU evidence, new host correctness checks and preliminary hypotheses. It names invalidation keys and correctness risks for reuse/precompute/immutable CPU jobs. Final target hypothesis acceptance remains explicitly pending.

### Prompt Context
**User prompt (verbatim):** See Step1.
**Assistant interpretation:** Make results independently reviewable/reproducible and provide bounded parent commands without crossing the device/production optimization boundary.
**Inferred user intent:** Turn instrumentation into a useful investigation, not merely add timers or claim GPU utilization.
**Commit (code):** `ac7b0b7` — "Add bounded GL pass telemetry and controlled diagnostic fixtures" (host-validated source/tooling milestone).

### What I did
- Added `scripts/analyze.py`, nine unit tests, `build-host.sh`, `smoke-host.py`, `plan.py`, `run-one.sh`, `build-parent-arm.sh` and their documented contracts. New useful scripts/logs live under this ticket, not root scratch or /tmp scripts.
- Analyzer reports individual CPU/GPU distributions and counts. Rejects invalid/nonnumeric/negative/nonfinite valid timings, numeric invalid samples, nested or duplicate GPU scopes, issued-record mismatches, instrumentation misuse/drop and absent final stats. Does not synthesize missing timings, sum children or combine query modes.
- Fake lifecycle test is CTest13; `-UNDEBUG` preserves its assertions in Release. Existing12 tests remain unchanged.
- Six8-frame host GL cases: off single, off dual, pass dual, whole-view dual, bypass dual, live dual. Captured top static prints for byte comparison; tested CLI lower/upper/cadence-text/mode and fixture-conflict rejection. Added receipt JSON/config assertions.
- Exercised `run-one.sh` locally under Xvfb with an explicitly labeled10s static-off run, then analyzed its CSV. Metadata, caps/frequency snapshots, binary hash, bounded exit and output records worked; timing is **host-only**, not a target benchmark.
- Documented14-run forward/reverse static then ABBA live plan, warmup60, per-run order/argv/hash/caps/memory/frequency/thermal/audio logging, and explicit parent checks for running games/save safety/BusyBox timeout behavior. Helpers never kill services or change policy/affinity.
- Added offline ARM helper requiring an explicit preapproved absolute library directory and existing image, no network/pull/SSH/copy. Syntax only checked, not executed by worker.

### Why
- An analyzer must distinguish missing/disjoint from zero and must not add nested CPU measurements. Query observer effects require matched CPU telemetry off/on, then potentially an additional no-profile comparison for sub-ms changes.
- Five static cases plus representative live ABBA avoid a sprawling matrix while checking query overhead, second view, actual omitted post work and one internal-resolution change. Repetition order is recorded rather than silently pooled.
- Target library acquisition and production deployment are parent authority. The existing general build-device script performs SSH/docker build, so it is inappropriate for this worker and is not called.

### What worked
- Host build/CTest13/13 and analyzer9/9 passed. GL cases ran on `llvmpipe (LLVM 20.1.2, 256 bits)` with `GL_error=0`; no query misuse or CPU buffer drops in smoke stats.
- Pass dual issued48 queries for8 frames×2views×3passes; view dual16; bypass dual32 and no post CPU/GPU scope. Live dual provided both-view dynamic prep/upload CPU scopes. Off modes issued0 (unprobed, not unsupported hardware).
- Static top captures were identical across off single/off dual/view dual/pass dual in the host gate. This is fixture repeatability/query-noninterference evidence, not complete-game parity.
- Runner exercised10s bound, order/source/hash/argv/receipt/snapshots and analyzer produced finite independent distributions. Actual target results remain PENDING.

### What didn't work
- No unexpected failure observed. Expected CLI failures were retained verbatim in smoke summary, including `fatal: profile cadence must be 1..3600`, `fatal: GPU profile mode must be off, view or passes`, `fatal: GPU profiling requires --profile CSV`, and `fatal: static terrain fixture conflicts with play/autoplay/dev-overlay/resize-check`.
- ARM build and device gate not run: isolated worktree does not own target libraries; parent must approve explicit existing inputs. No execution-mode fallback or system installation was attempted.

### What I learned
- The old receipt's `source_sha256` is historical asset provenance, not candidate build revision. Scripts record exact revision/binary hash separately. Its legacy33.3ms `timing_target_met` must not be read as60FPS acceptance.
- Frame CSV's explicit `frame >= 60` analyzer warmup is the comparison authority; old receipt summary has historical frame-index conventions. All pass timings remain nonadditive.
- CPU frequency/thermal snapshots only at run boundaries cannot prove continuous frequency or GPU utilization. Static fixture misses dynamic and stream costs by design, not by accidental silence.

### What was tricky to build
- Both query mode off and unsupported hardware have empty GPU CSVs; final stats differentiates disabled-unprobed from requested-but-unavailable. Graceful shutdown may retain pending rows and must never wait to improve sample count.
- Parent target may lack Python or have BusyBox timeout variants. `run-one.sh` is POSIX shell, app bounds itself, and analysis happens on host. External timeout syntax remains an explicit parent preflight rather than an assumed forced kill.
- Keeping all helper outputs bounded/reviewable required ignored `.work` for full builds/captures/transient tests and a small checked-in evidence subset. No build outputs, target libraries or screenshots need to be committed.

### What warrants a second pair of eyes
- Parser status/count coverage, fake dispatch assumptions, and candidate normal-path default fidelity.
- Native target query boundaries can affect tile render scheduling; off/on intervals must gate use of pass attribution. Disjoint rate and pending counts need actual driver results.
- Proposed shared-target changes, culling/LOD and shader precompute have correctness/fidelity risks; the cost map ranks investigations, not accepted measured gains.

### What should be done in the future
- Parent obtains independent review, cross-compiles, deploys separately, runs the small balanced static set then representative live dual with audio diagnostics and reports actual distributions. Revisit priorities only then.
- If dynamic probes dominate, isolate trail versus snow and upload bytes before a cache/worker design; if output dominates, separate copy from HUD. No need to add those controls speculatively now.

### Code review instructions
- Read [scripts/README.md](../scripts/README.md), source cost map and bounded [evidence directory](../scripts/evidence/).
- Rerun host build and smoke from exact HEAD; `git diff --check`; ensure shader/physics/audio/geometry source files unchanged.
- Parent-only build helper is syntax checked but unexecuted. Review mounts/no-network/library hashes before using it with explicit approved source libraries.

### Technical details
- Run-one cases: static-off/static-passes/static-single/static-bypass/static-res/live-off/live-passes. Default30s, seed42, internal384x288 except static-res512x384, fps60/no-vsync, cadence60. Live buffered1024/muted full DSP/vinyl/dev overlay; static lacks audio entirely. No auto-stop or governor/affinity modifications.
- Bounded evidence includes validation log, smoke summary and GL status excerpts. Full candidate patch/revision listing will be written to ignored `scripts/.work/review/` after final commits; path and exact hashes also go in the durable implementation handoff.
