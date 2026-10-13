---
Title: Native developer overlay implementation diary
Ticket: FALLLINIE-GAME-002
DocType: reference
Status: active
Topics: [falllinie, gameplay, diagnostics, overlay]
RelatedFiles:
  - File: src/developer_overlay.hpp
    Note: Main-thread numeric telemetry schema and bounded text cache
  - File: src/runtime.cpp
    Note: SELECT edge ownership and chord precedence
  - File: src/renderer.cpp
    Note: Source font bounded batched overlay, top view only
  - File: tests/test_developer_overlay.cpp
    Note: Formatting, timing semantics and cache evidence
  - File: tests/test_developer_controls.cpp
    Note: Edge/chord/rollback/title/pause evidence
---

# Native developer overlay diary

## Step 1: Implement the functional top-view overlay and resolve SELECT chord ownership

Starting from f1402c7 in an isolated implementation worktree, I read AGENTS.md, the diary skill and investigation-entry reference, then the native runtime/main/render seams and existing integration tests. This lane adds main-thread-only telemetry presentation; it does not repair audio, modify physics/world/terrain/math/shaders, consume additional events, or qualify hardware. The parent owns the concurrent DSP repair and physical combined qualification. The supplied failed-device evidence (roughly 50 FPS and crackling, callback max 23.991 ms versus 10.667 ms at 48 kHz/512 frames, 521/1053 callbacks over budget) is prior parent evidence, not a measurement taken by this lane.

The feature now toggles from SELECT/F2 in title, run and pause, uses the original HUD font in one bounded additional triangle batch, and refreshes diagnostic text/geometry at approximately 4 Hz. Existing CPU timestamps supply the previous completed frame; the panel explicitly distinguishes CPU loop timing and cumulative callback budget misses from presentation rates, GPU utilization and driver XRUNs.

### Prompt Context
**User prompt (verbatim):** "also allow for a dev overlay to be shown (when say, pressing select), with lower level data"
**Assistant interpretation:** Add the requested native diagnostic overlay and narrowly specified control/main/render/formatting tests, starting from f1402c7; preserve all parent-owned DSP APIs and source gameplay. Keep this detailed ticket diary, commit coherent milestones promptly, and leave a clean isolated candidate for parent review/cherry-pick.
**Inferred user intent:** Make the failing combined gameplay/audio run inspectable without logging from the callback or disturbing gameplay/event transport.
**Commit (code):** See Step 2 for the functional milestone hash.

### What I did
- Added `developer_overlay.hpp/cpp`: numeric CPU/render/audio/rider snapshot, 18 rows by at most 82 ASCII columns, 250 ms cache and allocation-free 120-frame rolling interval window.
- Added `NativeActions::dev_overlay_toggle/dev_overlay_select/dev_overlay_restore` and a mapper-owned SELECT press token. SELECT+Y (8+3) is the new-run chord; R remains keyboard new-run. SELECT+START (8+9) wins over every other action.
- Wired cached numeric capture on the main thread only, immediately before top rendering; the callback remains untouched. `--dev-overlay` implies `--play`, starts the title/demo with the overlay visible and avoids injected input.
- Added resident terrain/object cells, job-active/cell, visible terrain/object vertex/index counts, snow/trail counts, render/output sizes and exact relative render origin. No job-progress percentage is invented because TerrainJob exposes only cell/activity.
- Added original-font overlay VAO/VBO. Geometry builds/uploads only on text refresh or drawable resize, has a strict maximum of `(18*82+1)*6` vertices, and uses one glDrawArrays call, not fullscreen draws per glyph. Existing print and HUD paths are retained.
- Added dedicated control and formatting/cache tests to the integrated CMake project.
- Ran `cmake -S . -B /tmp/falllinie-dev-overlay-build -DCMAKE_BUILD_TYPE=Release && cmake --build /tmp/falllinie-dev-overlay-build -j4 && ctest --test-dir /tmp/falllinie-dev-overlay-build -R '^(developer_overlay|developer_controls|integration)$' --output-on-failure`.

### Why
- Diagnostics are numeric snapshots and string formatting is main-thread only, so the known callback deadline problem is not aggravated with I/O, strings or new callback clocks.
- CPU player timing includes streaming; it is explicitly labeled as a subset rather than added twice. Bottom-view times are n/a without a second window. GPU elapsed/utilization and CPU utilization remain n/a even when the separate profiler can issue delayed GPU queries; no resolved overlay query API exists.
- Cache and geometry lifetimes avoid new overlay allocation/formatting on ordinary frames; the disabled path does only a few booleans and bounded numeric timing/window updates.

### What worked
- Full host Release CMake executable build succeeded against installed SDL2 2.30.0, with no installs.
- Narrow CTest passed 3/3: integration, developer_overlay, developer_controls (0.02 s total). The integration audio fixture ran once; no full audio suite or graphics/device matrix ran.
- Tests check both visibility states, all chord press orders, no hold-repeat, title and paused gameplay, independent F2 ownership, original jump/boost/grabs/START/F3/R controls, measured callback mean/max/budget, unavailable values and cache cadence.

### What didn't work
- No build or test failed. Initial successful build emitted `src/developer_overlay.cpp:8:100: warning: format not a string literal and no format arguments [-Wformat-security]` for the no-argument generic formatting helper. Fixed that overload branch with `if constexpr(sizeof...(args)==0)` and a literal `%s`; follow-up warning-free build is recorded in Step 2.
- Physical/graphics validation is deliberately deferred: the task explicitly prohibits device access and any graphics matrix. Host compilation cannot prove shader linkage, actual pixel readability or two-window display placement.

### What I learned
- Immediate SELECT press feedback and zero transient visibility for a chord joined on a later snapshot are mutually exclusive. I escalated this instead of silently changing press-edge behavior to release-edge behavior.
- Parent approved immediate standalone SELECT edge toggle, restoration when a later held-SELECT chord joins, no same-snapshot side effect, and independent F2 toggles that cancel ownership. This is a deliberate, documented transient native behavior, not a debounce timeout.

### What was tricky to build
- Main saves pre-SELECT visibility when `dev_overlay_select` is set. A later chord emits restore before any new toggle, preserving the exact original visibility rather than blindly inverting it. Release clears mapper ownership. F2 cancels pending ownership so an unrelated keyboard toggle is not undone later. Tests distinguish simultaneous no-side-effect from sequential temporary-toggle/restore behavior.
- First-person rendering temporarily sets `first_person_view_` in addition to the existing camera/rider/following state, restores it both normally and in the catch path, and cannot draw the overlay. Top diagnostics are collected before the lower view changes camera state.

### What warrants a second pair of eyes
- Review main's restore-then-toggle ordering and mapper press-token cancellation, especially SELECT-first then Y/START and F2 while SELECT is held.
- Review row lengths/geometry bounds and GL state restoration. Physical acceptance must inspect legibility at 1024x768 and absence on first-person output.
- The reported audio max/miss counters are cumulative since device setup, not interval deltas and not driver XRUN counts; wording must remain explicit.

### What should be done in the future
- Parent reviewer must inspect/cherry-pick after the DSP repair, then qualify on device with `--dual-screen --dev-overlay` and the existing launcher. Compare parent CSV stages to overlay values and evaluate actual callback misses/presentation independently.
- No source-parity or performance acceptance claim follows from this host checkpoint.

### Code review instructions
- Start at `NativeInputMapper::read`, `main.cpp`'s action handling/cached snapshot/completed_frame, `format_developer_overlay`, `Renderer::draw_developer_overlay`, and `render_first_person` restoration.
- Reproduce the exact CMake/CTest command above. Dedicated tests do not open video/audio devices; integration uses offline render only.

### Technical details
- Stable consumed parent API: AudioDiagnostics sample_rate, buffer_frames, callback_count/total_ns/max_ns/over_budget, queue_depth/high_water, dropped_commands, stolen_voices, nonfinite_samples, active_voices/peak_voices and muted. AudioEngine::diagnostics is called only on the main thread and only when overlay refresh is due and the device is open.
- Callback budget = `1000 * actual buffer_frames / sample_rate` ms; mean = cumulative `callback_total_ns / callback_count / 1e6`, max = cumulative max ns / 1e6. No callbacks => mean/max n/a; closed device => callback/rate/buffer n/a.
- Source terrain/world/particles are only observed through renderer-owned existing state. No added second simulation, no extra frame advance, no second publish_feedback.
- Build outputs are outside the worktree at `/tmp/falllinie-dev-overlay-build`. No scripts/experiments were needed for Step 1.

## Step 2: Tighten unavailable-data semantics, publish current controls and finish host evidence

The functional checkpoint was committed promptly as ecbd3de72eea25f33b59250e3bdb7d8370e0bef3, before extended documentation/polish. I then reviewed the complete scoped diff, tightened missing-measurement text, added extreme-value/independent-toggle cases, and documented the approved transient SELECT chord behavior. No device access, deployment, installs, subagents, alternate execution mode or pushes occurred.

The final host executable builds warning-free and the three narrow tests pass. This is a checked host candidate, not a graphics/audio-device qualification; external reviewer and parent physical acceptance remain required. README historical first-playable limitations are explicitly historical rather than a new parity claim. Only this new diary is changed in the ticket; parent diary/index/tasks remain untouched.

### Prompt Context
**User prompt (verbatim):** See Step 1.
**Assistant interpretation:** Finish the scoped overlay candidate with independent review evidence, current controls/help, and a clean committed worktree.
**Inferred user intent:** The parent can cherry-pick without guessing APIs, rebuilding a full audio matrix or mistaking callback misses for driver faults.
**Commit (code):** ecbd3de72eea25f33b59250e3bdb7d8370e0bef3 — "Add cached native developer overlay and SELECT chord controls"; this step accompanies the subsequent bounded-polish commit, discoverable immediately after it on the ref below.

### What I did
- Fixed the initial successful-build format-security warning and verified a warning-free rebuild before the functional commit. Exact follow-up command: `cmake --build /tmp/falllinie-dev-overlay-build -j4 && ctest --test-dir /tmp/falllinie-dev-overlay-build -R '^(developer_overlay|developer_controls)$' --output-on-failure && git diff --check`; result 2/2 passed, 0 failed (0.01 s), no diff whitespace errors.
- Made rolling FPS/interval n/a before any completed frames, and queue/voice counters n/a when audio is closed instead of presenting fabricated zero measurements. Sanitized formatting bytes to the original ASCII atlas range, retaining bounded 82-column clipping for extreme counters/backend names.
- Added simultaneous F2+SELECT and F2+joined-new-run checks, maximum uint64 counters/long backend checks, and the existing integration test's no-overlay-side-effect quit assertion.
- Moved all first-person camera mutations into the existing exception restoration region; camera, shake, FOV, following state, rider count and first-person overlay suppression are restored even if view setup/render throws.
- Updated run/title/pause HUD legends, native `--help`, and README current controls/overlay semantics. The launcher already forwards arguments unchanged; no launcher defaults or scripts changed.
- Final changed source/test paths: CMakeLists.txt, README.md, src/developer_overlay.hpp/cpp, src/runtime.hpp/cpp, src/renderer.hpp/cpp, src/game_renderer.cpp, src/main.cpp, tests/test_developer_controls.cpp, tests/test_developer_overlay.cpp, tests/test_integration.cpp. Ticket changes consist solely of this reference/05 file.

### Why
- Explicit n/a is a stronger diagnostic contract than zero before a measurement exists, especially on title where audio intentionally remains closed.
- F2 must remain independent even when it coincides with the snapshot that joins a SELECT chord. Main's restore-then-toggle action order is now explicitly tested.
- First-person suppression is per-view state, not globally switching off the overlay; that prevents visibility leaks and preserves top redraw/capture behavior.

### What worked
- Exact final narrow command: `cmake --build /tmp/falllinie-dev-overlay-build -j4 && ctest --test-dir /tmp/falllinie-dev-overlay-build -R '^(developer_overlay|developer_controls|integration)$' --output-on-failure && /tmp/falllinie-dev-overlay-build/falllinie-spike --help && git diff --check`. Build succeeded without warnings; CTest reported `100% tests passed, 0 tests failed out of 3`, total 0.02 s. Help reports SELECT/F2 overlay, SELECT+Y/R new run, F3 debug views and SELECT+START/F10 quit. Help exits before SDL initialization/instance lock, so this command did not open devices.
- After the final title-only HUD legend adjustment, `cmake --build /tmp/falllinie-dev-overlay-build -j4 && git diff --check` succeeded warning-free. No test logic changed afterward.
- Scope audit: `git diff f1402c7 -- src/audio.cpp src/audio.hpp src/audio_dsp.hpp src/game.cpp src/world.cpp src/terrain.cpp src/terrain.hpp src/math.hpp assets/shaders` produced no output. Shader assets, audio, physics/world/terrain/math are unchanged.
- Static review found no known blockers in the scoped diff: source-font UV convention matches existing HUD; upload bound is 8,862 vertices; 12x20 glyphs and 22px row spacing fit the 1008x422 panel at 1024x768; normal frames only draw cached geometry.

### What didn't work
- No tests/builds failed in this step. No full audio suite or graphics matrix was run. The earlier warning and its fix are preserved in Step 1 rather than hidden.
- There is no actual device/pixel evidence for this candidate. GL shader linking, visible panel legibility, top-only placement, first-person exception restoration under actual GL errors and performance delta must still be reviewed/qualified physically.

### What I learned
- Existing port launcher argument forwarding already supports `--dev-overlay`; modifying launcher defaults would unnecessarily widen scope.
- The telemetry schema intentionally leaves CPU/GPU utilization absent and formats them n/a. A future parent extension must provide a real measurement and scope rather than overload CPU loop FPS.

### What was tricky to build
- Each text refresh builds the bounded lines on the main thread and copies them into Renderer; Renderer rebuilds GPU glyph geometry only when dirty or drawable dimensions change. Separate overlay VAO/VBO prevents the unchanged original HUD upload from overwriting the cache.
- The panel intentionally overlays some central gameplay feedback when enabled; the top primary HUD and bottom control legend remain in their original rendering path. Disabling restores the original view rather than adding a second render/simulation path.

### What warrants a second pair of eyes
- Required external reviewer gate is pending; this lane performed a self-review only and did not launch subagents.
- Inspect actual glyph size/contrast at 1024x768, potential overlap of long trick feedback, SELECT-first transient then restoration, and no panel on lower output.
- Verify callback snapshot consistency against parent instrumentation. As supplied in the supervisor update, parent DSP optimization improved prior target mean 8.84 to 6.67 ms but still had 178/1320 misses and max 19 ms; these are parent observations, not this lane's qualification results or a reason to relax measurement wording.

### What should be done in the future
- Parent should review and cherry-pick this ref's two commits onto the DSP-fixed main worktree, resolve any narrowly overlapping main/input/HUD documentation edits, then deploy/run its physical gate.
- Suggested physical invocation appended to the existing launcher: `--dev-overlay`; use `--autoplay --dev-overlay` only for automatic run/audio qualification if desired. Title mode alone intentionally has audio n/a until drop-in.
- For acceptance, compare with and without overlay, inspect original HUD on disable, title/run/pause/F2/SELECT/SELECT+Y/both SELECT+START orders, first-person absence, and audio misses versus actual buffer budget. Do not equate loop FPS with physical scanout rate.

### Code review instructions
- Managed ref: `pi-subagents/worker-Implement-a-lightweight-native-developer-overlay-for-Falllinie-in-your-isolated-6fb1cf25-d979bc7-d17d-s0-t0`.
- Worktree: `/home/manuel/code/wesen/worktrees/2026-10-12--falllinie-spike/pi-worktree-d979bc73-d1bb-4d8c-9e55-bfe1271f758b-s0-0`.
- Base: f1402c7. Review `git diff f1402c7..HEAD` and the narrow host command above, not a full audio/graphics matrix. Worktree/index should be empty after the polish commit; final handoff reports exact hashes and that check.
- Formatting/control tests are named after their source features. Integration retains one frame clock/one feedback publisher and only adds the quit assertion.

### Technical details
- Extension seam: `DeveloperFrameTimings` contains valid/dual, interval and stage wall times, ticks and physics_scale. Main uses precisely those same values for the existing Profiler::cpu call, then stores them for the next overlay snapshot. Snapshot state/cell counters are current-at-refresh; only frame timing/ticks are labeled previous-completed.
- `DeveloperRenderDiagnostics` exposes resident mesh-cell counts, visible terrain/object V/I, active terrain job/cell, particle counts, render/output dimensions and the origin obtained from the existing `render_frame` function. It does not mutate jobs, meshes, terrain or world.
- `DeveloperOverlaySnapshot` owns numeric rider/audio/frame/render data plus a main-thread backend name. `format_developer_overlay` and `DeveloperOverlayCache` are host-testable without GL. Cache invalidation is explicit on visibility actions; normal refresh is every 250 ms. `DeveloperFrameWindow` is fixed-size/no allocation.
- `Renderer::set_developer_overlay(bool)` changes visibility; `set_developer_overlay_lines` publishes the cache and dirties geometry; `developer_diagnostics` is main-thread only. Private per-view `first_person_view_` suppresses the panel, and restoration covers the setup/render catch path.
- NativeActions visibility integration contract: apply `dev_overlay_restore` to the stored pre-SELECT visibility first; for `dev_overlay_toggle`, save current visibility only when `dev_overlay_select`, then invert. A restore+independent-F2 toggle can coexist on one snapshot. No hold-repeat/new-run repeat occurs.
- Residual risks: added cached batch still has a real GPU/blend cost, unmeasured here; extreme numeric text is deliberately clipped rather than dynamically resized; FPS uses up to 120 intervals and can include warmup/new-seed stalls. GPU elapsed/utilization, CPU utilization and driver XRUN counts remain unavailable. No binaries, shared libraries, WAVs, secrets or experiment scripts were added.
