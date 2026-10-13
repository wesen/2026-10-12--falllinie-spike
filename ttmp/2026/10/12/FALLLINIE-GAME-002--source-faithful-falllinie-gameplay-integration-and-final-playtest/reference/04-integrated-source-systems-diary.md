---
Title: Integrated native source systems investigation diary
Ticket: FALLLINIE-GAME-002
DocType: reference
Status: active
Topics: [falllinie, gameplay, audio, integration]
RelatedFiles:
  - File: src/main.cpp
    Note: Main-thread lifecycle and one frame pipeline
  - File: src/renderer.cpp
    Note: Source rig/objects/effects with parent relative projection
  - File: src/audio.cpp
    Note: Source event latch and final nonlinear processing
---

# Integrated native source systems diary

## Step 1: Recover exact lane commits and preserve the relative-rendering repair

I read the repository instructions, diary investigation format, both full recovery handoffs, parent Steps 5–8 and the render-jitter README before integrating the exact six requested commits into the managed worktree based on d911ee4. The gameplay cherry-pick conflicted only in renderer.cpp/hpp; resolution retains source objects/rig/shadows/particles/six palettes/feedback and restores the parent's relative scene projection, inverse VP, camera eye, sun and absolute diagnostic switch. No device interaction occurred; the current handheld still runs the fixed old prototype.

### Prompt Context
**User prompt (verbatim):**

> Task: Continue the same native managed-worktree implementation protocol after recovered gameplay timeout. Work ONLY in your managed worktree, based on parent d911ee4. Do not touch device, deploy, install dependencies, push, launch subagents or use another execution mode. Parent owns physical acceptance. First read repository AGENTS, diary skill /home/manuel/.pi/agent/skills/diary/SKILL.md and its investigation-entry reference, and these handoffs completely: /home/manuel/.pi/agent/sessions/--home-manuel-code-wesen-claw-stuff--/subagent-artifacts/outputs/5dc015a1-c84c-4616-b9d6-c77b91669238/implementation/source-gameplay.md and source-audio.md in the same directory. Frozen authority fixtures/falllinie.html.
> Cherry-pick source gameplay a30784e7eae813fc46063440efea87693066ac29 then c2f82da466f53780628fef4e451e54d1f1e48221; audio d03b756ee353123d6baf822fd7003910df9fb462,6d75082abc0f9cce3b39dd26f2bfe367da26714c,bcd77043223b9ea181f521f0b6cf08a0f74b588a,c14eba08453700f86f5d9de76c636968e533d548. Resolve conflicts by preserving BOTH source objects/rig/feedback and parent camera-relative rendering fix 7eac73f. Read parent diary Steps 5–8 and scripts/2026/10/13/render-jitter-investigation/README.md: absolute float projection caused severe blue-edge speckle and mountain jitter after 58.6 km. Relative scene origin, relative post IVP/eye, sun projection, and diagnostic flag MUST survive; world shading/physics remain absolute. Current handheld still runs the fixed OLD prototype, not these source systems.
> Own cohesive integration main/CMake/input/audio lifecycle and any genuinely necessary source correctness fixes. Build new game/world/pose/particles/audio cpp files; retain profiler, lock, signal handlers, main-thread SDL/GL, sequential two-window context, first swaps before IPC placement, upper capture redraw, single simulation/audio engine and graceful resource destruction. Frame order: clamp raw dt<=.05, begin_frame once before fixed ticks; accumulate dt*returned scale, <=12 ticks at120Hz, discard overflow; real_time_feedback once afterward; renderer update once; publish RideSound, consume/clear events once per frame, never per view. No extra combo_tick. Pause freezes audio clock; slowmo/restarts do not alter/restart music; mute does not stop clock. Resolve quit chord before pause/restart edges; remove X/Y debug collision with source boost/grabs. Measured raw map A0/B1/X2/Y3/L1=4/R1=5/L2=6/R2=7/SELECT8/START9/dpad13–16. Source boost1/2, jump0, grab bits1/2/4/8 on4/5/6/7; keyboard source mapping from frozen fixture. Keep debug views/palette6 on nonconflicting keys. restart(seed) invalidates generation/caches; choose and log reproducible new seed when native new-run action requests it; no legacy reset path. Provide SDL_GetPrefPath app-owned best file to Game; no repo or arbitrary cwd saves.
> Reconcile the gameplay handoff's bail/GROTESK snapshot issue with source event-latched Music.grotesk, avoiding duplicate GroteskOn landing reactions when steady snapshots precede events. Inspect source reset/place transient differences and correct meaningful mechanical divergence rather than hide it; document deliberate native adaptations. Implement remaining native title/start/pause/controller feedback and source title autopilot if missing; DOM/touch markup need not be cloned, but do not silently claim those browser branches are native parity. Keep original arrangement, instruments, effects and gameplay. Audio handoff explicitly omitted source final 2x saturation oversampling: restore an efficient preallocated 2x nonlinear processing/anti-alias path with targeted checks, or report a concrete blocker, not 'complete' with known omitted graph behavior. Intrinsic native-versus-WebAudio oscillator/filter/random/compressor differences may remain clearly documented; no waveform-identity claim.
> Commit at coherent intermediate milestones PROMPTLY (previous gameplay lane waited until timeout). Keep detailed chronological reference/04-integrated-source-systems-diary.md in the existing FALLLINIE-GAME-002 ticket, full investigation-entry format, exact failures and targeted test evidence. Do not edit parent diary/index/tasks. Narrow checks at feature boundaries: gameplay host script, audio CPU/sanitizer script, integrated CMake build, terrain and relative-coordinate probe; no repeated broad graphics matrix or physical playtest. Consider a dummy SDL audio open/close/lifecycle check without video if useful, clearly not physical qualification. No binaries/libraries/WAV/secrets committed. Final clean index/worktree with exact commits, ref, worktree, tests, remaining differences and integration APIs in durable handoff. Generate FULL binary-capable candidate patch relative d911ee4 at scripts/2026/10/13/full-gameplay-integration/integrated-candidate.patch; leave that patch uncommitted (exclude itself) and give its exact absolute path/hash in handoff. Parent must be able to apply/inspect without guessing. If infrastructure fails preserve state, report exact blocker, no fallback.

**Assistant interpretation:** Integrate the recovered source systems, finish narrowly named missing native seams and correctness fixes, supply host evidence and an independently inspectable full patch; no physical qualification.
**Inferred user intent:** A coherent candidate that preserves the demonstrated rendering fix without hiding remaining source/native differences.
**Commit (code):** be02692, ab51329, c32856a, 6e38dcf, 7b92284, 7ea04cb — corresponding cherry-picks in requested order.

### What I did
- Verified clean initial HEAD d911ee4 and managed ref/worktree.
- Read all required handoffs and parent investigation entries completely.
- Cherry-picked six requested commits; repaired two renderer conflicts without changing frozen shaders.

### Why
- Parent fix is essential at 58.6 km, not optional historical code. Relative projection must coexist with absolute world shading and source feedback.

### What worked
- All six commits are now durable intermediate checkpoints on the integration ref.
- Diagnostic flag and runtime shader projection substitution survive along with six source palettes and rig/objects/feedback.

### What didn't work
- Exact first command: `git cherry-pick a30784e7eae813fc46063440efea87693066ac29` failed with `CONFLICT (content): Merge conflict in src/renderer.cpp` and `CONFLICT (content): Merge conflict in src/renderer.hpp`. Selected the lane bodies then explicitly reapplied every parent relative-coordinate seam, staged only the resolution and continued normally.
- No compile or runtime validation claimed at this merge checkpoint.

### What I learned
- The source camera shake must enter the same relative frame as scene/post/sun, while fog/shadows remain absolute.

### What was tricky to build
- Camera roll is recovered from the active view VP rotation. Both sequential views independently build their relative frames; no shared render origin leaks across views.

### What warrants a second pair of eyes
- Relative origin/shake/up consistency and continued source shader identity.

### What should be done in the future
- Complete native lifecycle/input/title, source transient correction and 2x final saturation; run narrow host gates.

### Code review instructions
- Inspect Renderer::render and initialize, renderer.hpp and parent precision probe; compare parent 7eac73f against merged bodies rather than only checking absence of conflict markers.

### Technical details
- Managed base: d911ee4. No main/CMake integration yet in this checkpoint; all later integration changes belong to this same worktree.
- Frozen authority and shader assets remain unchanged. GL/SDL video remains main-thread-only; compositor worker still performs IPC only.

## Step 2: Integrate one native frame, lifecycle, title demo and source transients

I wired all source CPU units into CMake, retaining the profiler, exclusive lock, signals, SDL/GL ownership, sequential shared-context two-window rendering, first swaps before IPC and upper-view redraw for capture. The native input/frame seam now owns one snapshot and one clamped fixed-step/feedback pipeline per frame; title runs the original autopilot policy and starts the same simulation through generation-invalidating restart. Audio starts lazily on drop-in, never on title, and is closed/joined before SDL teardown.

Source inspection showed meaningful restart/recovery differences in the lane: newRun preserves prior jump/grab edges, slowmo/flash/shake/HUD and recovery timers; placeOnCourse preserves charge/crouch/grab amount instead of calling resetRider. These are now retained. GROTESK audio is event-latched; steady snapshots cannot cancel it during bail or trigger duplicate landing sounds. One deliberate native correction turns the audio GROTESK/grind latch off on a new run, since the browser's newRun clears gameplay state but omits the corresponding Music calls.

### Prompt Context
- **User prompt:** See Step 1 (verbatim).
- **Assistant interpretation:** Complete the named native seams and correct source mechanical divergence without extending browser DOM/touch scope.
- **Inferred user intent:** A single coherent game/audio runtime with source controls and inspectable contracts.
- **Commit (code):** Recorded in subsequent checkpoint after committing this milestone.

### What I did
- Added runtime.hpp/cpp: NativeInputSnapshot/Mapper, source TitleAutopilot, NativeFrameClock and one publish_feedback event drain.
- Added game/world/pose/particles/audio/runtime CMake library and four CPU test targets, preserving terrain target.
- Main uses SDL_GetPrefPath("Falllinie","Falllinie")/best-score; no cwd/repository save path. New-run uses a deterministic seed sequence derived from initial --seed and logs each full seed for replay.
- Native controls retain source keyboard WASD/arrows, SPACE, SHIFT/B and J/K/U/I. Measured raw pad map: A0, boost1/2, grabs4–7, dpad13–16, START9 pause, SELECT8 new-run, chord8+9 quit before individual edges. F3 debug, C six palettes, M mute, F10 quit. ESC follows source pause, not old prototype quit.
- Added title/start/pause/mute/controller-connected versus keyboard feedback; pad detach/reopen is main-owned. Focus loss/minimize pauses an active run.
- Corrected source transients and generation-invalidating start; native runtime has no legacy Game::reset call.
- Added tests/test_integration.cpp for mapping/chord precedence, clock/cap/combo ownership, restart/place edges/transients, title pops and one event/audio lifecycle.

### Why
- Frame ordering and event ownership cannot be validated by linking only. A small CPU seam exercises production input/frame functions without video or a second simulation.
- Source's title demo steps physics but has no audio context; native audio is similarly opened only when dropping in.

### What worked
- `bash scripts/2026/10/13/full-gameplay-integration/check-gameplay-host.sh`: all three source oracle suites and integrated host link passed, no device run.
- `cmake -S . -B build/integrated-source && cmake --build build/integrated-source -j4`: passed with no warnings.
- Corrected targeted rerun `cmake --build build/integrated-source -j4 && ctest --test-dir build/integrated-source --output-on-failure`: all 5 tests passed (terrain, gameplay, world, pose, integration).

### What didn't work
- First CTest command failed only integration: `single combo tick: actual=1.000000 expected=0.991667`. The test restarted after slowmo then assumed one full-rate tick; the now source-correct restart intentionally retains slowmo. Set slowmo=0 explicitly for the isolated combo-clock test; production behavior unchanged. All five tests then passed.

### What I learned
- Source newRun is not an all-state reset. Manufacturing fresh grab edges can retrigger an uber after restart; clearing recovery charge changes the following release pop.

### What was tricky to build
- Commands must not be drained per view. Input resolves final physical chord before start/pause/new-run; fixed clock performs begin_frame once, at most 12 ticks, one real feedback hook, then main updates renderer once and publishes/clears events once.
- Lazy-open audio reinitializes its prepared queue: main reapplies pre-start mute immediately after successful open.
- Source camera shake is applied to eye, not look; relative frame rebuild now preserves that source view direction.

### What warrants a second pair of eyes
- Lazy-open/error path, pause/resume command ordering, pre-start mute, pad hotplug and explicit native key adaptations.
- Source transients are preserved, but source/native sound lifecycle differences are deliberately documented rather than hidden.

### What should be done in the future
- Restore final 2x waveshaper with an anti-alias path, targeted CPU/sanitizer tests and optional dummy-audio lifecycle check; parent owns graphics/physical acceptance.

### Code review instructions
- Begin with runtime.cpp, main loop, Game::reset/place/start, AudioEngine::Impl::commands and test_integration.cpp.
- Reproduce host script, integrated CMake build/CTest; no video is opened by tests.

### Technical details
- Raw dt clamp .05; scaled accumulator uses pre-tick begin_frame result; fixed DT=1/120; cap12 discards backlog. Pause neither accumulates nor steps; real-time visual/UI feedback continues as source.
- Restart does not reopen audio or reset song. Mute does not pause clock. Audio closure is explicit normal-path plus RAII exception-path, before global SDL_Quit.
- Native title/pause typography and bounded HUD are not DOM identity. Browser notes/touch markup and mathematical X-ray overlay remain unported; scene X-ray palette remains available.
