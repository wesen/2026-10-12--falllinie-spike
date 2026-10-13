# Independent source-completeness review

## Candidate inspected

- **HEAD:** `6338bf566ef96e095729a43609e8e7a0e65b3b26`, verified by reading the managed worktree’s HEAD/ref.
- **Base:** `d911ee412501a527a6c4a1a9c2f95042924ba46a`.
- **Ref:** `pi-subagents/integrate-source-systems-f2e8644-ca55-s0-t0`.
- **Worktree:** `/home/manuel/code/wesen/worktrees/2026-10-12--falllinie-spike/pi-worktree-f2e86446-dd09-4f97-b977-042e9fde09a4-s0-0`.
- Inspected its full patch inventory, implementation files, source authority, both lane handoffs/diaries, integration diary, tests, recorded validation and parent jitter investigation. Paths below refer to this candidate, **not parent main**.

Read-only review: no tests, Git commands, edits or device operations performed. Recorded test successes are evidence inspected, not independently rerun; patch hash and clean-index claims were not independently verified.

## Review

### Correct

- **Gameplay coverage is cohesive.** Ten grabs, six ubers, completion/unfinished landing, scoring/repetition, combo banking/loss, rail capture/slide/pop, trees and bail/recovery have corresponding production branches (`src/game.cpp:8–24,110–194`). Source-derived tests exercise these systems rather than only invented expectations (`tests/test_gameplay.cpp`, `tests/test_gameplay_world.cpp`, `tests/test_gameplay_pose.cpp`; oracle generation executes frozen HTML functions).
- **Frame ownership matches the requested contract.** One finite/clamped raw interval, pre-tick slowmo sampling, fixed 120Hz steps, cap/discard, post-tick feedback and one event drain are present (`src/runtime.cpp:48–61`; `src/main.cpp:130–134`). No extra main-loop combo tick or per-view event consumption was found.
- **Native lifecycle is wired.** Quit-chord precedence, measured controls, title autopilot, pause, generation-invalidating start/restart, reproducible new-seed logging and app-owned save location are implemented (`src/runtime.cpp:3–46`; `src/game.cpp:34–57`; `src/main.cpp:89–94,123–125`). Save uses closed temporary-file replacement (`src/game.cpp:214–218`).
- **GROTESK is correctly event-latched.** Ride snapshots cannot activate/cancel it; duplicate On does not manufacture another landing reaction (`src/audio.cpp:190–213`). Integrated tests specifically check snapshot-before-event ordering, bail retention and restart without resetting the music clock.
- **Audio architecture is callback-safe by inspection.** Numeric bounded SPSC transport, cursor-snapshot consumption, fixed voices/pumps and preallocated DSP avoid callback allocation, locks, strings, I/O and GL. Pause/close join ordering and subsystem ownership are coherent (`src/audio.cpp:26–121,190–216,307–391`). Final saturation genuinely interpolates, shapes at **2x**, filters and decimates after compression (`src/audio_dsp.hpp:54–86`; `src/audio.cpp:350–354`).
- **Parent rendering repair survives.** Scene projection subtracts the render origin while shading remains absolute; post IVP/eye and sun use the same relative frame (`src/renderer.cpp:40–45,134–165`). First-person up and normal/exception restoration are preserved (`src/game_renderer.cpp:74–80`).
- **Resource ownership is sound in inspected paths.** Nested rail generation owns copied features; active rails own their definitions. Restart discards the terrain job before its next use, retires both GPU maps, and rebuilds essential cells. Dynamic meshes are reused and destroyed with a live GL context (`src/world.cpp:3–16`; `src/game.hpp:63`; `src/game_renderer.cpp:38–54`; `src/renderer.cpp:207–230`).
- Recorded final evidence reports **CTest 6/6**, gameplay/world/pose and terrain checks, audio CPU/ASan/UBSan, allocation watching and relative-coordinate probes. The tests are useful but not exhaustive or physical qualification.

### Findings — P2, non-blocking

1. **Acid waveshaper omits the source’s endpoint clamping.**
   `src/audio.cpp:340` evaluates `tanh(b.acid * .5 * 3)` without constraining the waveshaper input. The source constructs its WaveShaper curve over `[-1,1]` and routes the half-gain acid bus into it (`fixtures/falllinie.html:1273,1290`); out-of-range WaveShaper input uses the endpoint value. Thus an overdriven branch differs from the source transfer, beyond lookup-versus-analytic interpolation.
   **Smallest fix:** clamp the half-gain acid input to `[-1,1]` before tanh; add an over-range scalar-transfer check. No audible impact was measured here.

2. **Malformed best-score text can be accepted as a valid prefix.**
   `src/game.cpp:213` accepts the integer extraction without checking remaining input. For example, `123garbage` loads best `123`, whereas the source numeric conversion rejects that string (`fixtures/falllinie.html:817`). Existing malformed-load coverage tests only a negative value (`tests/test_gameplay.cpp:43`).
   **Smallest fix:** require only trailing whitespace/EOF after the integer and add a malformed-suffix test.

### Acceptable documented adaptations

Native HUD/title/pause typography, omitted DOM notes/touch/X-ray overlay, board-nose/up lower-camera policy, seeded randomness, native oscillator/filter/compressor internals, FIR implementation/latency, partitioned reverb latency and bounded resource limits are explicitly disclosed. They do not justify browser-bitstream, waveform or complete presentation-parity claims.

### Merge verdict

**OK with notes — ACCEPT this exact HEAD for parent integration.** No P0/P1 blocker was established; retain the two P2 corrections above.

This is **not deployment or release acceptance**. Parent still owns fresh combined GPU/performance/controller qualification, real audio negotiation/underrun checks/listening and final human playtest. The old prototype’s physical results do not qualify this candidate. Parent should independently rerun the documented host scripts, CMake/CTest and precision probe, and verify patch hash/applicability before applying it.

<summary>
This turn: Reviewed exact candidate HEAD and found two non-blocking source-correctness notes.
Session so far: Completed fresh read-only gameplay, audio, lifecycle, rendering and evidence review.
Issues: Acid endpoint clamping and malformed best-score suffix validation need small corrections.
Next steps: Parent may integrate with notes, rerun verification and own physical acceptance.
</summary>