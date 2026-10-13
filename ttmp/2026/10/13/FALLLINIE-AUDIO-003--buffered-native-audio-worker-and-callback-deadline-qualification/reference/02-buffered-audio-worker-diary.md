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
