## Review

**Candidate:** `0ff0876ef3e0bafbad9a04c3c34b08f76aacc1f8` → `c6e4624ef50365b3fd28445bff4bde58cb17fc53`.

Reviewed the durable handoff, complete `candidate-review.patch`, candidate sources, full ticket guide, diary, API reference, tests and recorded evidence in:

`/home/manuel/code/wesen/worktrees/2026-10-12--falllinie-spike/pi-worktree-7c532c54-8195-46ce-a832-0eea5d97d995-s0-0`

Read-only Git metadata confirms the final HEAD and four-commit sequence. No commands, tests, device access or edits were performed. Clean status and patch SHA256 were not independently recomputed.

### Correct

- **SPSC ownership is sound.** Both producer copies precede release publication; the consumer acquire-loads publication and release-retires only frames already copied. Partial reads therefore cannot expose unread storage for overwrite. Every requested byte is zero-initialized, including incomplete stereo bytes; shortage advances neither musical nor delivered time. See `src/audio_pcm.hpp:22–41`.
- **Buffered callback is bounded and copy-only.** Its branch reaches copies, zero-fill, numeric atomics and clocks—not DSP, allocation, mutex/CV operations, notification, strings or GL. Direct DSP remains an explicit separate branch. See `src/audio.cpp:409–422`, `src/audio_timing.hpp:47–52`.
- **Unique DSP ownership and lifecycle ordering are coherent.** Offline preparation/render/trace reject live owners. Startup establishes obtained-request-aware prefill before unpausing SDL. Pause waits until any in-flight block is published, retains queued PCM and freezes production. Close quiesces SDL, wakes/joins the worker, then closes the device; caller-owned SDL initialization survives. Reviewed failure/timeout cleanup and rapid pause/resume ordering. See `src/audio.cpp:115–119,386–406,429–485,503–524`.
- **Sound math remains scoped.** The supplied diff leaves oscillator, scheduler, RNG, sorted mixing/stealing and `audio_dsp.hpp` unchanged. Vinyl defaults remain enabled; disabling it zeros the contribution only after source filtering and position advancement (`src/audio.cpp:368–371`). The reproduction helper preserves historical evidence, whose WAV hashes match the new manifest.
- **Diagnostics are honest and bounded.** CPU measurements use the calling thread’s clock, with unavailable readings counted rather than inferred from wall time. Producer timing excludes its deliberate waits. Running snapshots are advisory; final logging follows acknowledged pause. Main-owned formatting remains 18×82 with the existing 4 Hz cache and unchanged controller logic. See `src/audio_timing.hpp:10–31`, `src/developer_overlay.cpp:9–12,40–52,74–76`, `src/main.cpp:193–207`.
- **Tests exercise the important mechanisms.** PCM tests cover stereo FIFO, partial retirement, wrap, full rejection, malformed requests, shortage counters and concurrent transport. The actual-worker fixture checks bit identity, retained pause, allocation watch and empty/full/paused-full shutdown (`tests/test_audio_worker.cpp:37–62`). Lifecycle tests cover buffers 256–4096, direct/offline exclusion, reopen, mute, preparation allocation failure and SDL ownership (`tests/test_audio_lifecycle.cpp:21–75`).

### Finding

**P1 — Playback qualification remains unmet; not a demonstrated transport/lifecycle defect.**

The committed 60-second integration log records **1 starved callback / 1024 missing frames**, despite zero callback-budget misses:

`ttmp/2026/10/13/FALLLINIE-AUDIO-003--buffered-native-audio-worker-and-callback-deadline-qualification/scripts/host-sustained-integration.log:103`

The guide explicitly rejects starvation as normal-playback acceptance (§7, line 149; §12, line 306). The failure is correctly preserved in the handoff and documentation.

**Smallest corrective action:** keep playback acceptance open; qualify matched runs on the actual backend, investigate scheduling/throughput and measure response latency before any evidence-driven adjustment. Do not substitute fast callback timing, silent larger lead or a mode fallback for that gate.

### Evidence limits

Recorded logs support **12/12 CTests**, **5/5 ASan/UBSan tests** and **30 repeated transport/worker/lifecycle executions**. These were inspected, not rerun.

The actual-worker identity fixture covers 32,768 frames; the separate 90-second raw-float/trace comparison is **offline** evidence. Preparation allocation failure is injected, but thread-creation failure and prefill timeout are not independently exercised by those tests.

`…/scripts/tsan.log:5,10,15` confirms all three TSan attempts aborted before test execution with unexpected-memory-mapping errors. This provides **no race-sanitizer pass**.

### Merge verdict: **OK with notes**

Suitable for parent source integration and further hardware qualification. No additional source-code correctness defect was identified. This verdict does **not** establish release readiness, zero starvation, crackle-free playback, physical presentation performance or 60 FPS.

<summary>
This turn: Reviewed the exact worker candidate, guide, tests and recorded evidence read-only.
Session so far: Transport, DSP ownership, lifecycle, source fidelity and diagnostic paths inspected.
Issues: Playback starvation remains P1 qualification work; TSan never executed tests.
Next steps: Parent may integrate source and qualify actual backend scheduling, latency and listening.
</summary>