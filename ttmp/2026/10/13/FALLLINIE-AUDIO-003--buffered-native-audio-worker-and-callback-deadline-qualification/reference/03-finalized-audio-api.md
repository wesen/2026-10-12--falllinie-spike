---
Title: Finalized native buffered audio API and semantics
Ticket: FALLLINIE-AUDIO-003
Status: active
Topics: [audio, performance]
DocType: reference
Intent: long-term
Summary: Implemented configuration, ownership, frontiers, timing and qualification limits.
---

# Finalized API

The uploaded intern guide is historical design. This reference names the implemented interface; host tests do not qualify device deadlines.

```cpp
enum class AudioRenderMode : uint8_t { BufferedWorker, DirectCallback };
struct AudioOpenOptions {
 int buffer_frames = 512;
 AudioRenderMode mode = AudioRenderMode::BufferedWorker;
 bool source_vinyl = true;
};
bool AudioEngine::open(int buffer_frames = 512);
bool AudioEngine::open(const AudioOpenOptions&);
bool AudioEngine::prepare_offline(int sample_rate = 48000,
                                 uint32_t seed = 1,
                                 bool source_vinyl = true);
```

- Old `open(int)` remains source-compatible, selects worker and authored vinyl. Closed-engine validation allows power-of-two requested buffers 256..4096; obtained float stereo requests must be 256..4096, obtained frequency must satisfy existing 8000..192000 bounds. Invalid render enum is rejected.
- Active `open` is idempotent **before** validation: new options, even invalid ones, do not restart or reconfigure an open engine. Close/reopen is the configuration boundary.
- Producer blocks: 1024 frames. PCM storage: 8192 stereo frames. Target: `(ceil(obtained.samples/1024)+1)*1024`, normally 2048 and at most 5120. Actual depth varies and may be below target; capacity is not latency policy.
- Mode/vinyl/rate are immutable while either live owner exists. Live worker consumes the existing numeric command SPSC; direct callback consumes it only in explicit direct mode. Offline prepare/render/trace are excluded while device/worker exists.
- Startup: prepare all DSP/PCM, launch worker, wait up to 5 seconds for target prefill with SDL paused, unpause. Allocation/thread/preparation/prefill failures cleanly close/join and return false. **No fallback** render mode.
- Pause quiesces callback then acknowledges worker after its in-flight block is completely published; retains PCM, freezes the produced frontier. Resume consumes retained PCM first. Stop interrupts worker CV waits, joins it and closes SDL before destruction. Caller-owned SDL subsystem survives.
- Musical time is `AudioDiagnostics::frames` (produced frontier), not shortage silence or simulation ticks. Restart does not reset it; mute keeps synthesizing, with a smoothed future-output gain and queue/backend latency. Neither pause/resume nor starvation skips unheard generated samples.
- Vinyl off zeros only the filtered vinyl contribution before final compression/saturation. Noise generation/filter processing and RNG histories continue; source math, full 2.6-second stereo IR/1024 wet latency/2x49-tap saturation/sorted voice mixing and stealing remain unchanged.

## Numeric diagnostics

`AudioDiagnostics` keeps existing source and command counters. `queue_depth/high_water` refer to **commands**, not PCM. New fields:

| Fields | Semantics |
|---|---|
| `render_mode`, `source_vinyl` | Actual immutable open/offline preference. |
| `frames` | Cumulative produced musical frames, existing source clock. |
| `delivered_frames` | Cumulative generated frames handed to SDL, excludes silence shortages. In direct mode equals source frames. Not confirmed speaker presentation. |
| `pcm_depth`, `pcm_target`, `pcm_capacity` | Advisory current queued frames, selected target and allocated frames. |
| `pcm_low`, `pcm_high` | Minimum observed post-callback depth and maximum producer-observed post-publication depth. Sampled extrema, not fabricated continuous occupancy; before callback, low equals current depth. |
| `starved_callbacks`, `missing_frames` | Cumulative callbacks short of complete stereo frames and total absent frames. Malformed partial-frame bytes are initialized to silence but not a musical frame. |
| `callback_count/total_ns/max_ns/over_budget` | Cumulative wall timing of selected callback path; budget uses actual callback frames/sample rate, not desired request. No driver XRUN inference. |
| `callback_cpu_count/total_ns/max_ns/unavailable` | Cumulative measured calling-thread CPU samples/times; unavailable explicitly counted. |
| `producer` (`AudioTimingDiagnostics`) | Analogous cumulative wall and CPU fields for rendered+published 1024-frame blocks. Producer block-budget exceedance is not a callback failure. No wait time is included inside measured render blocks; scheduler descheduling during a block appears in wall time. |

Wall clock is SDL performance counter. On Linux CPU clock is `CLOCK_THREAD_CPUTIME_ID`; unsupported/failed clock readings increment `cpu_unavailable` and do not invent utilization. Valid CPU counts are the denominator of CPU means. Timing/cursor snapshots are independently atomic, advisory during live transfers, not a coherent exact schedule. Paused acknowledged frontier identity is exact: `frames == delivered_frames + pcm_depth` in buffered mode. PCM shortage never increments delivered/produced frames.

## CLI and panel

`--audio-mode buffered|direct`, `--audio-buffer N`, `--no-vinyl`, `--source-vinyl`. Last vinyl option wins. Executable defaults source vinyl; handheld launcher explicitly selects `--no-vinyl`, appended override restores source mix. The main-owned panel remains 4 Hz, 18 ASCII rows <=82 columns. Buffered rows show callback wall/CPU, PCM lead/frontiers, distinct shortage counters and producer wall/CPU/exceedances; direct rows keep legacy callback/commands/voices. Exit log includes full numeric aggregates, clock availability and actual configuration after pause acknowledgement.

## Qualification checkpoint

- Full 12-test CMake suite passes on host; actual worker/callback PCM bit identity, malformed/partial/wrap transport, allocation watch, deterministic full/empty stop, source default/full effects, dummy SDL buffers256..4096, direct/offline exclusion, failure cleanup and lifecycle are tested.
- 60-second software-GLES/dummy run completed GL_error=0 and no callback budget misses, **but 1 starved callback/1024 missing frames**. Producer maximum wall 48.956 ms versus maximum thread CPU 4.369 ms. No source/timing fix or 60 FPS acceptance is claimed.
- ASan+UBSan: 5/5 audio tests passed; repeated transport/worker/lifecycle:30/30 passed. TSan:0/3 ran successfully, all terminated before test execution with `FATAL: ThreadSanitizer: unexpected memory mapping ...`; race-sanitizer acceptance is unavailable here.
- Exact base0ff0876→candidate 90-second 48kHz/seed19 float PCM and source trace hashes match; migrated A/B renders match both historical WAV hashes. See reference/02 and ticket `scripts/source-identity.json`, `scripts/vinyl-probes/manifest.json`.
- Parent owns independent review, target build/deployment, real backend zero-starvation throughput/latency and listening qualification. Requested textbook-style Obsidian report is deferred until those final results and scoped vault commit/push authorization exist; report-ready outline is in diary02 Step4.
