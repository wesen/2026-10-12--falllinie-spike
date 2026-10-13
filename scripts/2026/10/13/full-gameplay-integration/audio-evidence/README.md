# Source audio CPU evidence / integration contract

Authority: `fixtures/falllinie.html:1251–1451` and the Music call sites, SHA256 `cb9b4befd0252859d9c45246f94fbd7dd1662c6d034c6ccb95fbe16947e8b556`. These are host CPU checks, not audible/device acceptance.

## Reproduction

From the repository root, without an audio device:

```sh
scripts/2026/10/13/full-gameplay-integration/check-audio.sh
AUDIO_SANITIZE=1 scripts/2026/10/13/full-gameplay-integration/check-audio.sh
scripts/2026/10/13/full-gameplay-integration/check-audio.sh --render /tmp/falllinie-source-audio.wav 120
```

The script uses the installed SDL2 and C++17 compiler, a temporary build directory, and removes its executable afterwards. It never opens SDL audio/video. WAV rendering is optional PCM16 evidence, not a gameplay asset. The 120-second file is deliberately **not committed** (23,040,044 bytes, SHA256 `aabb21cf8f6fbd2cb1862ce029f6ae548da8f83e79585fef8d3041eec56cc253`). Timings will vary by host. The WAV is deterministic for the recorded seed/rate/scripted commands, not identical to browser random realizations.

Environment: x86_64; g++ Ubuntu 13.3.0; SDL2 2.30.0. Final logs: `cpu-check.log`, `sanitizer-check.log`, `offline-render.log`. `initial-compile-failure.log` retains the test-only missing-GameEvent-text-initializer error before it was corrected. An earlier script root calculation also failed to find `src/audio.cpp`/`tests/test_audio.cpp`; it was corrected from four to five parent directories.

## Results

- Exact generated 2.6s stereo-IR convolution reference at 8kHz: 20,800 taps/channel, max absolute error 9.29e-9; nonzero tail beyond two seconds. FFT roundtrip checked.
- Twenty sections / 2,434 steps: intro → verse → hook, weighted non-repeating subsequent sections, all five types, source chord voicings/11ms strums, kick/backbeat/jitter/swing deadlines, fractional sixteenth clock, all arrangement voices, finite bounded non-silent output. Peak 40 voices, no normal-budget note drops or steals.
- Source air/landing/pop/uber/bail/GROTESK/grind responses, carve/wind quantization and smoothing, mute/pause/restart continuity, ignored combo/recovery events.
- Overflow injection: queue high water 256, exact first 44 drop-newest commands; deterministic voice stealing caps at 128. Concurrent numeric SPSC producer/consumer check (ASan/UBSan, **not** a race-detector proof).
- 44.1kHz output/block-size identity; 48kHz reactivity and optional render. Zero watched render allocations; no callback locks, strings, I/O, logging or GL/SDL video calls by inspection.
- 120s / 48kHz stereo render: all 16 instrument/SFX types; 689 steps / 6 sections; peak 42 voices; queue high water 2; no command/note drops, steals or nonfinite samples. Peak 0.746482, RMS 0.132355.
- External host timing of 512-frame render calls: mean 0.747223ms, p95 1.25212ms, max 2.26112ms; 10.6667ms nominal block interval. This excludes setup/WAV I/O and is **not RG DS profiling** or SDL backend callback qualification.

## Integration API and ownership

Add `src/audio.cpp` to the parent-owned CMake executable/test inputs; the existing direct `src/*.cpp` build already discovers it. Link the existing SDL2 library; no new dependency. `src/audio_dsp.hpp` is a local implementation helper, not a separate build unit.

Create exactly one `fl::AudioEngine` for both views. `open()` initializes SDL_INIT_AUDIO if needed, opens a default F32 native-endian stereo device requesting 48kHz / 512 frames, accepts only frequency changes, prepares all buffers at the **obtained** rate, then unpauses. On failure it returns false; the main thread may report `SDL_GetError()`. Keep the engine alive until `close()`/destruction has joined the callback, before global SDL shutdown. SDL/GL video remains untouched and main-thread owned.

All command-producing APIs and lifecycle methods are **single main-thread producer only**. `set_ride` and `trigger` convert to bounded numeric commands; they never enqueue `GameEvent.text`/points. Drain Game events once, not once per view. Prefer publishing ride state then events for the frame so a Land/Bail restoration takes effect last.

- `set_ride(RideSound)`: `carve`/`wind` are already source `Music.carve(l)` / `Music.wind(l)` inputs, **not raw speed**. Source carve is `clamp(speed/28,0,1)*(0.4+abs(steer)*0.6+brake)`; takeoff clears it. Wind is `clamp(speed/30,0,1)` in air, zero at landing. `sustained_air` must reflect source `air_time > 0.35`, not every hop. `grinding` and `grotesk` control persistent layers. `speed` and `air_time` are deliberately not reinterpreted in the DSP.
- `trigger(Land)`: `amount` = source `big=clamp((air_time-0.4)/2,0,1)`; `flag` = whether a real source trick result exists. Rail scoring alone must not invent a Land/chop reaction.
- `Pop`, `Uber`, `Bail`, `GroteskOn/Off`, `GrindOn/Off` implement only the corresponding source Music reactions. GroteskOn deliberately invokes source `land(1,true)` once, in addition to any separate actual landing event.
- `ComboAdded`, `ComboBanked`, `Recovery`, `Restart` are ignored: no source Music reactions/jingles. Restart never resets the scheduler. Publish next ride state to clear stale continuous layers intentionally at restart.
- `pause(bool)` suspends SDL audio processing (or offline rendering) and freezes all musical/DSP time. Do not multiply the audio clock by slowmo. `set_muted(bool)` smoothly targets output 0 / 0.85 with 0.05s time constant; clock continues.
- `diagnostics()` reads relaxed lock-free atomics: advisory recent per-field values, **not** a coherent transactional snapshot. It reports frames/steps/section, queue high water/drop count, voice peak/steals/note drops, nonfinite count, smoothed ride buses and voice-family counts. No time measurements/syscalls inside callback.
- `prepare_offline(rate,seed)`, `render_offline(interleaved,frames)`, `take_trace(into,capacity)` are **closed-device, single-thread render/trace** APIs. Explicit offline preparation resets the song. No trace reading during live audio. Trace retains the latest 4096 scheduled starts; event sample/step/note/velocity/section are numeric.

## Native/WebAudio differences and limits (not waveform identity)

Original score tables, 86 BPM, 128-sixteenth sections, probabilities, swing, jitter, envelope times/levels, formants, filter parameters and branch gains/sends remain explicit. A bounded conditional weighted draw implements the same distribution as the source's rejection loop, without an unbounded callback loop. A dedicated seeded xorshift stream replaces browser Math.random; noise/IR generation uses another stream. Random draw order/realizations differ (including finite 32-bit randomness); no source random-bitstream claim.

- Sine voices use an interpolated 4096-entry table; saw/square use polyBLEP; triangle is analytic. Browser oscillator band-limiting/phase behavior is not identical. Signed instantaneous FM frequencies are supported; extreme frequencies are bounded to 0.45 sample rate.
- RBJ biquads map WebAudio low/high-pass Q from dB to resonance; bandpass Q is linear. Dynamic coefficient updates every 16 samples, exact sample-rate-independent target smoothing, sample-rounded envelope knots. Browser AudioParam interpolation/filter implementations may differ.
- The original stereo random IR `(rand*2-1)*(1-i/length)^3.4`, 2.6s length and WebAudio-style RMS/sample-rate normalization are retained, with stereo sends and no truncated tail. Self-contained preallocated radix-2 FFT overlap-add uses 1024-frame partitions. **Wet-only added latency is 1024/rate seconds** (21.33ms at 48kHz); dry voices remain sample-timed. Work bursts every 1024 frames. Four spectra banks occupy about 7.6MiB at 48kHz, plus noise/vinyl/effect/voice storage. No O(IR-length) work per sample and no third-party FFT/library.
- Delay is rounded to nearest sample (three sixteenths), original 0.38 filtered feedback / 0.35 pumped return. Rhodes stereo panning and reverb sends are retained; mono voices expand to both channels. Pads are decomposed into four linear-filter voices instead of one shared node graph.
- Linked peak-envelope soft-knee compressor retains -16dB / 10dB / 3.5 / 6ms / 200ms but does not recreate WebAudio's hidden detector/lookahead/reduction algorithm. Final saturation evaluates the original clamped tanh transfer analytically, rather than a 2048-point WaveShaper curve, and **does not implement the browser's 2x final-saturation oversampling**. Glitch quantization and acid tanh retain source branch levels. Audible differences/alias energy require integrated listening.
- 128 preallocated composite voices, 64 pending pump automations, 256 SPSC commands; overflow drops newest commands (visible counter), steals the earliest-ending composite voice, or reports pending-pump overflow. This is a bounded native resource limit, not an original browser limit. Normal test/render scenarios did not exhaust it; hostile event floods do.
- Rate support 8–192kHz; low test rates deliberately cap filters below Nyquist. Host tests do not open a physical/dummy audio device. SDL format negotiation, backend underruns, target callback cost and actual sound quality remain the parent's integrated target/human gate.
