---
Title: Source audio integration diary
Ticket: FALLLINIE-GAME-002
Status: active
Topics:
    - falllinie
    - audio
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://src/audio.cpp
      Note: Source arrangement, synthesis, mixing graph, SDL lifecycle and numeric SPSC commands
    - Path: repo://src/audio_dsp.hpp
      Note: Preallocated DSP primitives and complete source-shaped stereo impulse convolution
    - Path: repo://tests/test_audio.cpp
      Note: CPU arrangement/reactivity/queue/allocation checks and optional offline WAV render
    - Path: repo://scripts/2026/10/13/full-gameplay-integration/audio-evidence/README.md
      Note: Reproduction, integration mapping, evidence and explicit native numerical differences
ExternalSources: []
Summary: Independently implemented original Music arrangement and reactive native DSP with bounded host evidence; target integration and listening remain open.
LastUpdated: 2026-10-13
WhatFor: Preserve audio-lane source mapping, ownership decisions, validation failures and integration requirements.
WhenToUse: Review or integrate the source-reactive soundtrack without confusing host evidence with audible target acceptance.
---

# Source audio integration diary

## Step 1: Port the original score and reactive WebAudio graph to bounded native DSP

The audio lane began at native `main` base `c99143ebc76a3c4ebc775e22f95e6ad3418abbe7`, in its managed worktree. I read the source Music body, its movement/air/rail/landing/bail/uber/GROTESK callers and startup/pause/restart/mute lifecycle, the supplied intern guide, `AGENTS.md`, the shared numeric `feedback.hpp` contract, and the diary skill/investigation format. This implementation migrates the live original composition; it does not substitute demo music, a generic loop or a prerecorded gameplay asset.

The feature now provides one independently integrable `fl::AudioEngine`, preallocated instruments/effects, source arrangement and reactive commands, plus closed-device CPU tests and optional offline audio evidence. The narrow host checks pass, including ASan/UBSan and zero watched C++ render allocations. The 120-second render contains all sixteen instrument/SFX families and normal event reactivity. This is not an audible verdict or RG DS callback qualification; main/build integration, target profiling and the final human playtest remain the parent's responsibility.

### Prompt Context
- **User prompt (verbatim):** refer to [parent investigation diary Step 4](01-investigation-diary.md#step-4-deliver-report-and-guide-authorize-source-complete-integration), where the authored request is preserved. This lane does not reconstruct quotations from runtime/task summaries.
- **Assistant interpretation:** implement the original Music score, voices, graph and only original reactions in the owned audio files; expose the approved main-thread numeric API and sufficient offline diagnostics for independent review.
- **Inferred user intent:** make the source soundtrack respond to the same ride on both views without real-time ownership hazards or a generic replacement soundtrack.
- **Commit (code):** `d03b756ee353123d6baf822fd7003910df9fb462` — `Port source-reactive Falllinie audio score and bounded native DSP`.
- **Scope:** only `src/audio.hpp`, `src/audio.cpp`, `src/audio_dsp.hpp`, `tests/test_audio.cpp`, the dated audio-specific script/evidence, and this diary. No Game/renderer/main/CMake/feedback/shared diary/index/tasks/changelog edits, device access, deployments, dependency installs, pushes or subagents.

### What I did
- Retained the three PROG tables, five section-flag sets, CHOPS/VOW tables, source 86 BPM sixteenths, eight-bar sections, initial intro/verse/hook, weighted later transitions, swing, ±3ms jitter, 12ms-late backbeats and source note/probability patterns.
- Implemented kick/noise attack, snare/noise+triangle/crushed variants, hats, FM/detuned/panned Rhodes with tine and slow detune, bass, detuned pads, three-formant saw chops, square glitch blips and resonant Grotesk acid.
- Preserved named branch levels/sends, ride noise bands, air drum/filter targets, pump, three-sixteenth filtered-feedback delay, full generated stereo IR, vinyl crackle/hiss, source-level compressor controls and saturation transfers.
- Implemented only source pop/land/bail/uber/GROTESK/grind reactions. Combo/recovery/restart events generate no invented audio and do not reset the song.
- Added SDL native-float stereo setup honoring obtained frequency, numeric SPSC transfer, pause/mute semantics, advisory diagnostics and a bounded offline source-event trace.
- Added an allocation-free render check, IR/FFT numeric reference, source timing/chord/arrangement and reactivity checks, overloaded queue/voice checks, concurrent SPSC exercise, non-default-rate/block-size identity, and optional PCM16 WAV output with externally measured render-call cost.

### Why
- One sample clock prevents game slowmo, render frequency or the second view from changing the song tempo. SDL pause stops all musical/DSP time; normal restart intentionally does not touch it.
- Numeric commands let main-thread GameEvent strings stay outside the callback. A snapshot of the producer cursor bounds per-call command work; atomics are compile-time required to be lock-free.
- Uniform 1024-frame FFT partitions retain all 2.6 seconds of the original random stereo impulse. A generic reverb would change the source response, while direct per-sample convolution would multiply CPU cost by the entire impulse length.
- Composite voices retain the source synthesis/linear routing without recreating a WebAudio object graph or allocating oscillator/filter objects while rendering. Conditional weighted choice has the same accepted distribution as source rejection sampling but bounded callback work.
- No third-party package/FFT library was installed or vendored. Self-contained C++17 uses only the already installed SDL2 build dependency.

### What worked
Commands (from the managed worktree):

```sh
scripts/2026/10/13/full-gameplay-integration/check-audio.sh
AUDIO_SANITIZE=1 scripts/2026/10/13/full-gameplay-integration/check-audio.sh
scripts/2026/10/13/full-gameplay-integration/check-audio.sh --render /tmp/falllinie-source-audio.wav 120
sha256sum /tmp/falllinie-source-audio.wav
stat --printf='%s bytes\n' /tmp/falllinie-source-audio.wav
git diff --check
```

- CPU and ASan/UBSan: PASS; 20 sections, 2,434 steps, all five section types, peak 40 voices, finite bounded arrangement, no normal note drops/steals, all original reactivity/lifecycle checks; zero watched render allocations.
- Partitioned source-IR reference: 20,800 taps/channel at 8kHz, max error `9.28995e-09`; energy remains beyond two seconds. This checks the algorithm against the same generated mathematical response, not browser-random sample identity.
- Intentional overload: first 300 Pop commands produce exact 44 command drops from capacity 256. Voices remain capped at 128, with reported deterministic steals. Concurrent producer test stays numeric/finite; ASan/UBSan is not a thread-race proof.
- Offline render: 120s at 48kHz, 689 steps / 6 sections, all sixteen voice types, peak 42 voices, queue high water 2; no command/note drops, steals or nonfinite output. Peak `0.746439`, RMS `0.132359`.
- Host 512-frame render cost: mean `0.749866ms`, p95 `1.24088ms`, max `2.42457ms` against `10.6667ms` nominal interval. x86_64, g++ Ubuntu 13.3.0, SDL2 2.30.0. Timed externally around rendering, excluding setup/WAV I/O. **Not target profiling.**
- WAV: 23,040,044 bytes, SHA256 `44971dfe151c4e724584c69891ebf236730f31f97178a8cca4c4a7a364fde44d`. Kept outside the commit; reproduction option is committed. It has not been listened to by a human in this lane.
- Bounded logs and comprehensive limitations/integration mapping live under `scripts/2026/10/13/full-gameplay-integration/audio-evidence/`.

### What didn't work
- Initial direct compile command:
  `g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic $(pkg-config --cflags sdl2) -Isrc -c src/audio.cpp -o /tmp/falllinie-audio.o`
  succeeded but warned at the initial audio.cpp lines 149/218/220/386 with `[-Wmisleading-indentation]`. I separated adjacent statements; final script compiles with `-Werror`.
- Initial `scripts/2026/10/13/full-gameplay-integration/check-audio.sh 2>&1 | tee scripts/2026/10/13/full-gameplay-integration/audio-evidence/cpu-check.log` had the script root one directory too shallow:
  `cc1plus: fatal error: src/audio.cpp: No such file or directory` and the corresponding `tests/test_audio.cpp` diagnostic. Corrected four parent-directory components to five. The outer pipeline initially lacked pipefail; subsequent validation commands used `set -o pipefail`, and the script itself has `set -euo pipefail`.
- The next script compile failed `-Werror=missing-field-initializers` on test aggregate GameEvent literals, including `tests/test_audio.cpp:114:17: error: missing initializer for member ‘fl::GameEvent::text’`. Exact diagnostic was preserved in `initial-compile-failure.log`. Added a test-only event factory that explicitly initializes kind/amount/flag/points/text; did not modify the shared contract.
- No lane infrastructure/runtime/tool failure required changing execution modes. No physical/dummy device was opened. No unheard soundtrack or device qualification is claimed.

### What I learned
- WebAudio low/high-pass Q uses resonance dB, whereas bandpass Q is linear; the native coefficient setup must account for that distinction while retaining source parameter values.
- Rhodes' original FM depth can drive instantaneous carrier frequency negative. Clamping all oscillator frequency positive changes that timbre; the implementation therefore supports signed phase increments, with an explicit Nyquist safety bound.
- WebAudio WaveShaper clamps its input curve range. Native final tanh also clamps before applying the source transfer, retaining the source 0.85 output bound rather than extending the analytic tanh beyond the curve range.
- Music.air target changes cancel previous frequency automation. A real air-state transition cancels native bail-filter automation as well. Shared rhythm/effect state belongs to audio, not either camera.

### What was tricky to build
- Reverb setup must finish while SDL's newly opened device is still paused. Callback userdata remains alive through SDL_CloseAudioDevice; subsystem shutdown follows the close, never precedes it.
- The stereo Rhodes send cannot be collapsed into mono without changing the convolver input. Native reverb retains per-channel stereo history/IR banks; mono snare/chop sends expand to both channels.
- Original scheduling looks 140ms ahead, starts at 100ms and schedules offbeat swing/jitter as future voice starts. A fractional double deadline plus sample-rounded start indices avoids cumulative integer tempo drift; per-sample processing yields identical output for different callback block sizes.
- Source pad filters/envelope are shared linear nodes; splitting the chord into four two-oscillator composite voices is mathematically linear-equivalent aside from browser/native oscillator/filter precision. Nonlinear branches remain summed before source waveshaping.

### What warrants a second pair of eyes
- Source-to-native envelope/filter/voice/mix mapping, especially Rhodes FM/tine and stereo reverb, compressed/saturated branch levels, and source event flags/order.
- C++ SPSC cursor acquire/release and the single-producer restriction. Diagnostics are per-field relaxed atomic values, not a coherent transaction; offline trace APIs are not safe to consume during live callback operation.
- Partition burst cost and memory on the real RG DS: 1024-frame wet latency is explicit (21.33ms at 48kHz), roughly 7.6MiB of spectra plus other preallocated buffers. Host p95 alone does not prove target headroom or underrun behavior.
- Approximation acceptance: native sine/polyBLEP/analytic triangle, sample-rounded envelopes/delay and every-16-sample dynamic filters differ from browser primitives; compressor lacks WebAudio-specific detector/lookahead; final saturation lacks source browser 2x oversampling. Random realizations/phase/band-limiting and analytic versus 2048-point waveshaping are not waveform identity.

### What should be done in the future
- Parent: add audio.cpp to CMake, construct exactly one engine after appropriate startup/drop-in, publish one RideSound/event sequence per frame, wire pause/mute/restart/quit, close before global SDL shutdown. The existing direct `src/*.cpp` build discovers audio.cpp automatically.
- Parent: keep carve/wind source quantities and `sustained_air >0.35s` in Game. Land.amount is original big; Land.flag is actual trick-result existence. No generic rail/combo/chime mapping.
- Parent-owned integrated target profiling: negotiated format/rate, callback budget, underruns, simultaneous dual-view/streaming workload and event queue counters.
- Final human playtest: recognize source musical arrangement/timbres, ride/filter transitions, event audibility, mute/pause/restart continuity and audible numeric approximations. No physical gate was run here.

### Code review instructions
- Start with `src/audio.hpp` API/diagnostics limits, `src/audio.cpp` source tables → schedule/new_section → add/voice_sample → Buses/render → SDL lifecycle/SPSC transfer.
- Compare against Music.init/schedule/instrument/public reaction functions in `fixtures/falllinie.html:1251–1451` and caller lines 909/933/977–1008/1077/1095/1114/1777–1786.
- Inspect `src/audio_dsp.hpp` Envelope, WebAudio-Q mapping, oscillator signed FM, FFT and full-length normalized Reverb; run the three narrow commands above. Tests do not require hardware.
- Consult audio-evidence/README.md for exact integration/event mapping and all deliberate numerical/resource differences. Review must not equate passing numerical tests with human sound acceptance.

### Technical details
- `fl::AudioEngine`: default construct/destruct, open/close/pause/set_ride/trigger; optional set_muted; offline preparation/render/trace; advisory diagnostics. F32 native-endian stereo, 48kHz requested, returned rate honored; supported 8–192kHz.
- Source BPM 86, S16 `60/86/4`, section length 128 steps. Fixed numeric queue 256, voice capacity 128, pending pumps 64, trace 4096; overflow exposed via diagnostics. No callback dynamic allocation, strings, locks, I/O or video calls.
- Owned branch/ref: `pi-subagents/source-audio-29dd4e5-e6b1-s0-t0`; worktree `/home/manuel/code/wesen/worktrees/2026-10-12--falllinie-spike/pi-worktree-29dd4e59-3882-4bfc-8c59-091e36911ff6-s0-0`.
- Code commit `d03b756ee353123d6baf822fd7003910df9fb462` contains ten scoped files, 965 insertions. This subsequent diary checkpoint records that revision; shared index/task bookkeeping intentionally remains parent-owned.

## Step 2: Correct Rhodes detune ordering during final source review

Final self-review found a small but real signal-order discrepancy: WebAudio applies detune to the complete frequency AudioParam, including incoming FM. The first native version detuned the base frequency and then added the modulation unchanged. I corrected both carriers to `(base + modulation) * detune_ratio`, preserving negative instantaneous frequency and the original ±6-cent/LFO detune.

The same narrow CPU, sanitizer and optional-render checks pass again. Final evidence logs/README were refreshed; Step 1's recorded values remain historical rather than being rewritten. No integration or physical acceptance was added.

### Prompt Context
- User prompt: see parent diary Step 4 and audio diary Step 1; no new authored request.
- Assistant interpretation: complete source self-review and correct the local numerical routing error before handoff.
- Inferred user intent: recognizable original FM timbre, not an undocumented frequency-parameter ordering change.
- Commit (code): `bcd77043223b9ea181f521f0b6cf08a0f74b588a` — `Apply Rhodes detune to the full signed FM frequency`.

### What I did
- Corrected Rhodes carrier frequency composition in `voice_sample`.
- Added a direct negative-frequency sine phase check, protecting the source FM requirement.
- Re-ran the identical three narrow commands from Step 1 and refreshed their bounded logs.
- Updated audio-evidence/README.md with the final render measurements and hash.

### Why
- Detune and frequency modulation do not commute when modulation depth changes with time. The source frequency AudioParam must be combined before cents scaling.

### What worked
- CPU / ASan / UBSan: PASS, 20 sections / 2,434 steps, peak 40 voices, output peak 0.664208, zero watched render allocations; other source/IR/reactivity/budget checks unchanged.
- Final 120s render: peak 0.746482, RMS 0.132355; host 512-frame mean 0.747223ms, p95 1.25212ms, max 2.26112ms against 10.6667ms nominal interval.
- Normal-render diagnostics unchanged: 689 steps, 6 sections, peak 42 voices, queue high water 2, all sixteen instrument/SFX families, no command/note drops/steals/nonfinite.
- Final WAV SHA256: `aabb21cf8f6fbd2cb1862ce029f6ae548da8f83e79585fef8d3041eec56cc253`, still 23,040,044 bytes, outside the commit.

### What didn't work
- Self-review identified the first carrier composition as a source-routing error; this was not a compile/test/runtime failure. No new failed command.

### What I learned
- Preserve WebAudio's computed-frequency ordering, not merely its isolated frequency/detune constants.

### What was tricky to build
- The final carrier must allow negative full FM frequency after cents scaling; ordinary positive-only oscillator clamping is not valid for the original depth.

### What warrants a second pair of eyes
- Verify the corrected frequency composition directly against Music.rhodes; physical sound and target callback headroom remain unconfirmed.

### What should be done in the future
- Same parent-owned integration/profiling/human listening gate as Step 1. All documented compressor, oscillator, latency and oversampling differences remain explicit.

### Code review instructions
- Review `src/audio.cpp` Rhodes branch and the signed oscillator check in `tests/test_audio.cpp`; use the three Step 1 reproduction commands and final logs.

### Technical details
- Only six existing lane-owned files changed in this code checkpoint; no shared/main/build/device scope expansion.
- Final source code checkpoint is bcd7704 on the same managed branch; the diary is recorded separately after that code hash becomes available.
