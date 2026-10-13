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
