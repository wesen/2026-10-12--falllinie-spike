---
Title: Buffered audio worker implementation diary
Ticket: FALLLINIE-AUDIO-003
Status: active
Topics: [audio, performance]
DocType: reference
Intent: long-term
Summary: Isolated native implementation, failures, ownership and host evidence.
---

# Buffered audio worker diary

## Goal and prompt context
Implement the accepted same-process worker contract on base `0ff0876ef3e0bafbad9a04c3c34b08f76aacc1f8`. The actual authored request is recorded verbatim in [parent reference/01, Step 1](01-detailed-implementation-diary.md#step-1-separate-authored-vinyl-noise-from-callback-risk-and-establish-the-worker-contract); this lane does not invent additional user quotations. Parent already delivered the intern guide and owns source acceptance, target deployment, physical qualification and publication. No device/install/push/subagents or scheduling policy changes in this lane.

Read AGENTS.md, the **complete** design guide, audio.cpp/audio.hpp/audio_dsp source seams and existing tests, parent diary, and docmgr diary skill (`corporate-headquarters/docmgr/ttmp/skills/diary/diary.md` and `.codex/skills/cursor-diary/SKILL.md`). Keep parent ticket bookkeeping/history untouched; document finalized API separately.

## Step 1: Bound frame ownership independently of SDL

Implemented a fixed-capacity stereo PCM ring before changing device ownership. A frame-addressed ring permits a callback to retire only copied frames, including partial DSP blocks, with two bounded copies even across storage wrap. Worker publication is all-or-nothing; unread PCM is never overwritten. This is the guide's ownership contract with frame cursors rather than slot cursors.

### What I did
- Added `src/audio_pcm.hpp`, deterministic and concurrent `tests/test_audio_pcm.cpp`, and CMake CPU audio test targets (the existing source CPU suite previously was not registered with CTest).
- Ran `cmake -S . -B build-audio -DCMAKE_BUILD_TYPE=Release`, `cmake --build build-audio --target test-audio-pcm -j4`, `ctest --test-dir build-audio -R audio-pcm --output-on-failure`.

### Why
PCM memory transfers should be reviewable and testable without backend scheduling or synthesizer cost. Capacity and target lead are separate concepts; the helper does not impose fill policy.

### What worked
Host GCC 13.3.0 / SDL 2.30.0 configuration and test passed. Deterministic stereo, partial, wrap, full/empty, oversized/malformed requests, initialized shortage and 500,000-frame concurrent FIFO identity pass. Thread-local allocation watch reports zero consumer allocations.

### What didn't work
Initial broad home-directory skill search exceeded its 20-second inspection bound; targeted repository skill search located the documented format. No transport test failures.

### What I learned
Retirement can happen per frame rather than per block without weakening SPSC ownership: producer observes retirement only after copy completion. Advisory cursor snapshots need clamping to avoid fabricated unsigned-underflow depths during concurrent sampling.

### What was tricky to build
Release publication after both wrap copies and release retirement after both consumer copies. Incomplete stereo bytes must be zero, never read from a partially published frame.

### What warrants a second pair of eyes
Acquire/release cursor ownership, snapshot semantics and arbitrary byte-size consumption. Reset is only legal with both owners quiescent.

### What should be done in the future
Integrate the actual buffered callback with allocation/timing tests; retain independent concurrent sanitizer gate and parent device qualification.

### Code review instructions
Start with `PcmRing::push/consume`; run the commands above. Input frames use opposite-sign left/right values, so channel swaps and repeated frames cannot pass silently.

## Step 2: Give the synthesizer one live worker owner

Added one dedicated DSP producer with preallocated 1,024-frame scratch and an 8,192-frame ring. Startup prepares the unchanged DSP, starts the producer and waits for request-aware prefill before unpausing SDL. Ordinary 1,024-frame devices target 2,048 ready frames; 4,096-frame devices target 5,120. No fallback to direct mode occurs on failure. Direct callback remains an explicit diagnostic option; offline and live preparation are mutually exclusive.

Pause first quiesces SDL, then requests/awaits producer acknowledgement after any in-flight block is published. Only then is the source pause flag set. Queued PCM and both frontiers stay intact; resume clears that flag before waking the worker. Stop can interrupt paused and timed-full waits without any callback notification. Device close joins both owners before storage can be destroyed and preserves caller-owned SDL initialization.

**Prior transport commit:** `a71bf65` — Audio: add bounded stereo PCM transport and deterministic FIFO tests.

### What I did
- Added typed `AudioOpenOptions`, `AudioRenderMode`, source-vinyl offline argument and numeric timing/frontier diagnostics in `src/audio.hpp`.
- Reused `Impl::render` unchanged except explicit vinyl preference at the mix output (filter/RNG/generation still run). Worker consumes the existing numeric command queue.
- Added `src/audio_timing.hpp`: exact shared buffered consumer path, allocation-watchable without SDL device timing; Linux thread CPU clock with explicit unavailable counts, independent producer/callback wall-budget counters.
- Extended source tests with bit-exact offline→buffered stereo identity over variable requests and vinyl/default/RNG trace comparisons. Extended lifecycle tests across 256/512/1024/2048/4096 and explicit direct mode, failure injection, repeated open, mute, frozen/retained pause, reopen, destructor and caller-owned SDL lifetime.
- Ran `cmake --build build-audio --target test-audio-pcm test-audio test-audio-lifecycle -j4` and `ctest --test-dir build-audio -R 'audio-' --output-on-failure`; bounded evidence in `scripts/worker-boundary.log`.

### Why
The callback cannot wait for DSP or notify a condition variable. A 1 ms timed producer wait makes refill independent of callback synchronization, while main/worker CV notifications handle startup and lifecycle only. Mute affects future generated PCM after retained lead, not already queued output.

### What worked
All three audio tests passed (8.28 s total). Existing full source CPU branch ran, not only `--render`. Source transport is bit-identical for 32,768 frames. Callback helper allocation watch includes clocks and atomics; deliberate two shortages produce 2 starved callbacks and 17 missing frames, not synthesized time. Invalid options and injected `std::bad_alloc` unwind device/subsystem and allow retry.

### What didn't work
No test failures at this boundary. Sanitizer and sustained integration gates are still pending, not implied by Release tests.

### What I learned
Publication of an in-flight final block must precede pause acknowledgement. Setting the source pause flag before that acknowledgement could publish silent PCM without advancing the clock, violating frontier identity. Active `open` is idempotent even if supplied options differ; actual config remains unchanged.

### What was tricky to build
Order of SDL pause, producer acknowledgement, source pause flag and resume wake; transactional preparation/thread creation/prefill timeout cleanup; single-writer numeric timing sets; preserving filtered vinyl state despite inaudible mix.

### What warrants a second pair of eyes
`Impl::produce`, `AudioEngine::open/pause/close`, explicit callback branch and failure paths. The callback's buffered branch invokes only `PcmConsumer::callback` (copies/zero/atomics/clocks), no render/locks/allocations/notifications. Diagnostic snapshots are advisory, while paused frontiers are exact.

### What should be done in the future
Finish CLI/launcher/main-owned overlay and reproducible A/B helper migration, then sanitizers and sustained host gate. Parent must measure actual PipeWire/device starvation, latency and producer throughput under comparable game workloads; dummy SDL is not physical acceptance.

### Code review instructions
Run the three audio CTests; inspect the published pause invariant `frames == delivered_frames + pcm_depth`. Direct mode deliberately still performs DSP in callback and is never an automatic recovery path.

## Step 3: Integrate honest diagnostics and run the sustained graphics/audio gate

The executable now selects buffered/direct mode explicitly, reports the actual vinyl preference, and logs separate producer/consumer wall and measured thread CPU counters with frontiers/lead/starvation. The handheld launcher requests no-vinyl without changing source/library/offline defaults; appended `--source-vinyl` restores source mix. The 4 Hz 18x82 main-owned developer panel remains bounded; buffered rows replace the old audio rows, while the original direct-mode rows/controls remain intact.

A 60-second host llvmpipe/dummy integrated run **did not meet zero-starvation playback acceptance**: one 1,024-frame shortage occurred despite zero callback budget misses. This is retained as measured evidence, not hidden by silently increasing the queue or falling back to direct mode. The worker's largest wall block was 48.956 ms versus 4.369 ms maximum thread CPU, so wall delay is not equivalent to DSP CPU cost. Scheduling causality and actual handheld qualification remain the parent's responsibility.

**Worker/lifecycle commit:** `ef69e0a` — Audio: move live DSP to prefilled worker with retained pause and copy-only consumer.

### What I did
- Added `--audio-mode buffered|direct`, `--no-vinyl`, `--source-vinyl`, finalized numeric exit log (after pause acknowledgement), launcher preference and README contract.
- Added deterministic friend-fixture `tests/test_audio_worker.cpp`, compiled with audio.cpp in its own executable, to test the **actual** worker and callback: bit identity through partial reads/wrap/repeated pause, withheld producer starvation, empty/full/paused-full/refill shutdown and thread-local callback allocation watch. No new public testing API; the friend declaration is an internal test seam.
- Guarded offline rendering/trace against any joinable worker, not merely an SDL device. Added `tests/test_audio_diagnostics.cpp` for buffered panel clock-availability/ASCII/size semantics.
- Migrated the historical A/B helper to the supported offline vinyl API; new evidence destination is this ticket's `scripts/vinyl-probes`, original logs/manifest remain untouched.
- Ran `cmake --build build-audio -j6` and full `ctest --test-dir build-audio --output-on-failure`: **12/12 passed**, 13.63 seconds, in `scripts/host-ctest.log`.
- Ran `SDL_VIDEODRIVER=x11 SDL_AUDIODRIVER=dummy LIBGL_ALWAYS_SOFTWARE=1 timeout 90 xvfb-run -a -s '-screen 0 1024x768x24' build-audio/falllinie-spike --autoplay --dev-overlay --audio-buffer 1024 --audio-mode buffered --no-vinyl --seconds 60 --render-size 384x288 --fps 60 --no-vsync`; full bounded output in `scripts/host-sustained-integration.log`.

### Why
Timing must distinguish ready-PCM shortage from expensive callback DSP and driver XRUNs. The actual worker fixture makes empty/full shutdown reproducible without modifying backend request sizes or relying on chance timing. A/B reproducibility should select a supported numeric configuration rather than copy/edit production C++ and overwrite old experiment logs.

### What worked
Sustained process completed with GLES 3.2 Mesa 25.2.8 llvmpipe, 1,593 frames, GL_error=0. 2,815 callbacks: wall mean 0.004962 ms/max 0.550192 ms, CPU mean 0.002508 ms/max 0.010498 ms, no callback-budget misses. 2,816 producer blocks: wall mean 2.130376 ms/max 48.956 ms, CPU mean 1.852573 ms/max 4.36897 ms. Final exact paused frontiers: produced 2,883,584, delivered 2,881,536, depth/target 2,048/2,048. No drops, steals or nonfinite output. Full CMake suite and new internal worker/diagnostic tests passed.

### What didn't work
The relevant integration **zero-starvation gate failed**: 1 starved callback, 1,024 missing frames, producer budget exceedances 2; observed PCM low 0. Software-rendered frame p50/p95/p99 were 29.064/99.506/122.664 ms, not 60 FPS. Swap interval was unsupported (`warning: swap interval: That operation is not supported`) under Xvfb; no physical presentation claims. Initial targeted main.cpp edit rejected duplicate oldText (`Found 2 occurrences of edits[4]`); tool made no partial edits, corrected by using enclosing call-site context.

### What I learned
Small finite lead protects ordinary scheduling gaps, not arbitrary worker descheduling. Fast copy callbacks can still starve; callback budget alone is not the acceptance criterion. The final paused snapshot makes the transport equality exact even though running overlay snapshots are advisory.

### What was tricky to build
Keeping 18x82 formatting without moving strings into either audio owner. Preserving historical reproduction artifacts while changing the helper. Creating deterministic engine full/empty fixtures without shipping runtime knobs or permitting actual live/offline overlap.

### What warrants a second pair of eyes
Callback branch and fixture, CPU-unavailability semantics, lead formula versus obtained request size, producer/block-budget versus callback-failure labels. The sustained host shortage is a residual qualification issue, not a source/DSP-math test failure.

### What should be done in the future
Run remaining sanitizers and exact base-source hash comparison. Parent review/target qualification should account for scheduling tails, responsiveness, actual backend and listening; no silent larger default or affinity change in this lane.

### Code review instructions
Read the two new tests and host logs, compare timing counters independently. The full/empty fixture intentionally permits full storage internally to exercise shutdown, while production options target at most 5,120 frames.

### Follow-up request (deferred)
Parent relayed a new authored request for a detailed textbook-style technical Obsidian project report (no analogies), commit/push to go-go-parc **after** implementation, independent review and parent integration/qualification. This lane records the follow-up only; no vault edits/push now. Parent will provide finalized native commits/device evidence and scoped authority. The later report must load obsidian-vault-writing and textbook-authoring skills, preserve existing reports/unrelated state, and cover PCM/SPSC publication, DSP/FFT clocks, ownership/lifecycle, tests and measured failures/limits with self-contained figures and code/API references.
