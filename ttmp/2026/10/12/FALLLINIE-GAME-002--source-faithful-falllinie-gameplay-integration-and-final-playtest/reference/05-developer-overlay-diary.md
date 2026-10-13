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
