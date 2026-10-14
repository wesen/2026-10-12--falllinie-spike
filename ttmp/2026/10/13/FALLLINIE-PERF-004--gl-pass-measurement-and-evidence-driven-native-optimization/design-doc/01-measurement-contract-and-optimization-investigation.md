---
Title: Measurement contract and optimization investigation
Ticket: FALLLINIE-PERF-004
Status: active
Topics: [performance]
DocType: design-doc
Intent: long-term
Summary: Evidence and authority contract for optional GL-pass instrumentation and controlled target experiments.
---

# Measurement contract

The current native game renders two camera views through one main-thread GL context. Recorded dual384x288 buffered-audio gameplay averaged19.7839ms per frame, exceeding the16.667ms60FPS period. About17.65ms lies inside the two renderer call boundaries, but these CPU-observed wall intervals include driver overhead and possible GPU backpressure. They do not identify shader, geometry or presentation cost individually.

## Questions and evidence

1. Is the main cost scene geometry, fragment shading/post, full-size output/HUD, dynamic uploads, context/presentation or CPU-side construction?
2. How much do the second camera and shared render targets serialize GPU work or duplicate CPU work?
3. Which costs scale with internal/output pixel count, visible geometry, dynamic vertices or streamed-cell jobs?
4. Where can immutable data, cached locations/state, prepared meshes or reused per-frame data reduce cost without changing source semantics?
5. What CPU work could move to a worker under immutable snapshot/generation-tag contracts while GL/uploads stay main-owned?

## Baseline

Retained source evidence: AUDIO-003 scripts/target/buffered/frames.csv. After60warmup frames: mean events.2540ms,physics.1557ms,player.6613ms (includesstream.0866ms),topGLwall9.5481ms,bottomGLwall8.1037ms,swaps.8962ms,pace.0058ms. Existing GPU CSV has no samples. These are historical live-autoplay values, not a replay of identical scene work.

## Required instrumentation properties

- Opt-in asynchronous GL_EXT_disjoint_timer_query support. Unsupported queries remain explicitly unavailable, never zero. No glFinish, blocking query-result reads or busy waits in render loop.
- Nonoverlapping elapsed queries: whole-view and per-pass elapsed cannot nest on the same target. Select one timing granularity or use disjoint frame schedules with explicit mode metadata.
- Bounded query pool; associate every sample with original frame/view/pass; handle pending slots/disjoint episodes, skipped sampling and unavailable results honestly. Destroy queries while valid context exists. Record query mode/cadence and overhead comparison.
- CPU timing boundaries separate preparation, upload/submit and context/swap where practical. Existing total timings must remain interpretable; nested values are not summed twice. Cumulative draw/triangle/upload counts can explain scope but are not GPU utilization.
- Production default behavior, shader math/precision, geometry, RNG, audio and simulation unchanged. Diagnostic pass/component bypasses explicitly label non-parity output and never become default or shipping optimizations by accident.

## Target experiment progression

First build/test/review host implementation, then parent ARM64 build and device smoke. Run bounded baseline queries-off versus queries-on at matched configuration to quantify observer effects. Use a reproducible frozen scene/view or deterministic replay for controlled comparisons; explain any difference from live gameplay. Start with single/dual view, baseline/pass timings, post bypass and one internal-resolution comparison. Expand geometry/dynamic/HUD/output experiments only when evidence points there. Do not run a combinatorial matrix by default.

Record binary/source hashes, exact arguments, engine/driver/extension information, warmup/sample counts, actual CPU-frequency/headroom snapshots, CPU interval percentiles, valid/disjoint/missing GPU counts, pass statistics and audio shortage counters. Current live game may be in use: inspect first, preserve saves, do not terminate processes blindly. Parent owns device operations. No system governor/FIFO/affinity/driver changes in this phase.

## Deliverables and decision boundary

Useful build/run/analyze scripts and bounded evidence live under this ticket's scripts directory. Worker source map and hypothesis table should identify concrete file/symbol/range, mechanism, supporting or missing measurements, estimated bound (not fabricated speedup), fidelity risk, implementation effort and next validation. Include batching/state/uniform caching, upload reuse, culling/LOD only as explicit fidelity tradeoffs, shader invariant precomputation, render-target/presentation hazards, terrain/particle CPU preparation and safe snapshot-based parallel work. No parallel GL, duplicated authoritative simulation or real-time callback work.

This request authorizes measurement tooling and scoped diagnostic experiments, not an unreviewed rendering rewrite. Select substantive production optimizations after evidence; small behavior-preserving fixes may be proposed with narrow tests. Final acceptance requires actual target evidence, not host llvmpipe timing alone. No automatic vault/reMarkable publication requested.
