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
