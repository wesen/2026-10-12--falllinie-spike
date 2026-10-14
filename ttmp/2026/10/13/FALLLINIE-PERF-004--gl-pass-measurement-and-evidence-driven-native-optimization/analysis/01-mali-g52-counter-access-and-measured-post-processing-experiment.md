---
Title: Mali G52 counter access and measured post-processing experiment
Ticket: FALLLINIE-PERF-004
Status: active
Topics:
    - performance
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Actual G52 hardware counters work through libGPUCounters; post bypass improves this live workload but is non-parity, and counter activity is not GPU wall-time utilization."
LastUpdated: 2026-10-13T22:56:27.563060381-04:00
WhatFor: ""
WhenToUse: "Selecting the next precise GPU measurement and interpreting the first actual Mali counter capture."
---

# From CPU driver samples to GPU hardware events

The previous report established that much of the main thread's executing CPU time was inside libmali. That is different from observing the GPU itself. This investigation adds a separate process using Arm's libGPUCounters to read hardware event counts through `/dev/mali0`. The process does not submit GL work or change the game's renderer; it asks the driver for counts accumulated over sampling intervals.

The route works on the actual device. The database advertised **240 counters** for the detected GPU. All13requested metrics configured successfully, and sampling/start/stop completed without reported errors. This is stronger evidence than a product compatibility table, but it does not prove that every advertised counter has been exercised or independently calibrated.

## Target identity

| Component | Actual observation |
|---|---|
| Device/SoC | Anbernic RG DS Plus / Rockchip RK3568 |
| GPU | Mali-G52, one shader core, r1p0, ID0x07040002 |
| Core mask | Available0x1 |
| Kernel | Linux7.2.7 aarch64 |
| Kernel driver | mali_kbase r54p2-02eac0, UK interface11.46 |
| Userspace object | libmali.so.1.10.0; SHA256605e3a5caf1be62bb48d97b1168434257355d56db5e3be9b515cfa742c173118 |
| Counter node | /dev/mali0, already accessible; no permissions changed |
| Existing tool | renderdoccmd executable present, capture not tested |

Kernel and userspace driver versions are independent identities. Another installed filename contains`g29p1`; it must not replace the observed running kernel-module version in reports.

## Why libGPUCounters before Streamline

Arm describes libGPUCounters, formerly HWCPipe, as an embeddable frontend/backend library exposing public Mali counters available to Streamline. Its documented target range includes Bifrost products such as G52 and its README specifies the commercial driver. We pinned revision`378c7b59e2dae87cbbffb0ac9cf96ed13c939823`, built a standalone probe with the already available ARM64 toolchain, and used it without installing a system package or service.

| Tool | Evidence/status here | Appropriate purpose |
|---|---|---|
| libGPUCounters | Built and executed successfully;13metrics sampled | Hardware activity/bandwidth observations in existing workflows |
| Streamline/gatord | Official target-agent documentation saved; not installed or executed | Richer combined CPU/GPU timelines and counter selection, subject to target setup |
| Mali Offline Compiler | Official URL discovered; page extraction returned empty content despite exit0; not installed/executed | Static shader resource/instruction analysis, not runtime timing |
| RenderDoc | renderdoccmd already installed; compatibility untested | API/frame/resource inspection, not inherently hardware utilization |
| Graphics Analyzer | Mentioned in earlier discussion only; not qualified here | Future API-tracing candidate if needed |

The missing FTRACE/KPROBES facilities still prevent the earlier scheduler/driver tracing route from being assumed available. They do **not** prevent this direct Mali counter route: actual sampling succeeded without them. Gator uses perf for much collection and ftrace for additional events; a full Streamline timeline cannot be promised from the successful GPU-only probe.

# What the counter names mean

Arm's G52 reference defines `MaliGPUActiveCy` as cycles with pending work in at least one processing queue. It continues counting while stalled on external memory. It is not a count of successful arithmetic operations and is not automatically a wall-time busy percentage.

The selected fragment/nonfragment/core/tiler counters describe different overlapping hardware states. They must not be added as mutually exclusive phases. Derived`MaliFragUtil` normalizes fragment cycles by GPU-active cycles and core count. Texture utilization divides texture issue cycles by core-active cycles; arithmetic utilization divides instruction counts by core-active cycles using the mapped G52 formula. These are different denominators, not competing shares of one100%budget.

External byte counters are derived from external bus beats multiplied by bus width. They represent traffic attributed by the hardware interface, not all CPU/system DRAM traffic. Tiny bus-stall ratios do not by themselves rule out a memory problem; they measure particular backpressure events.

Source copies of the counter map, formulas, sample API and timestamp helper are preserved alongside Arm's complete G52 HTML reference. The downloaded implementation remains ignored. This keeps the definitions reproducible without adding a third-party library to the game.

# The bounded campaign

The already qualified TRACE005 game binary was unchanged: SHA256`2872ede7219ccd91890de4c0d47d5af8fab687f2b5b2cf9605e25ecb1dfcb1d2`, runtime209a6d0. The independent counter probe samples at10Hz and permits at most120seconds. Capture uses unique candidate directories and explicit timeouts, and never attaches to or kills an existing game/collector.

One campaign ran idle reference, normal no-counter baseline, live with counters, live post-bypass with counters, and closing no-counter baseline. Game runs are30seconds with dual384×288, no-vsync/fps60, autoplay, overlay, buffered1024 source-vinyl DSP muted at output, paired clocks at cadence60 and GPU elapsed queries **off**. Idle sampled30seconds. Counter-enabled game cases sampled45seconds to include startup and termination without assuming identical initialization duration.

The probe wrote15600counter rows:3900idle and5850each for live/bypass. It reported zero sampling errors and successful stop in all three captures. There were591nonfinite derived-ratio rows in idle/startup/tail data, which remain explicitly marked rather than replaced with zero. **None occur in the selected live/bypass gameplay windows.**

All four game runs completed withGL_error0, zero callback budget overruns, zero starved callbacks and zero missing frames. As before, these finite counters are not backend XRUN or latency measurements.

## Clock and interval boundaries

The backend's exposed sample-begin timestamps were about1.262–1.268seconds ahead of the probe's CLOCK_MONOTONIC completion timestamps. Directly joining them to the game sidecar would be incorrect. The library's timestamp helper uses CLOCK_MONOTONIC_RAW when available, but that source fact is not complete proof of the selected kernel backend's timestamp convention.

The analysis therefore uses host monotonic poll boundaries and restricts both adjacent polls to the game's measured central interval: from the first paired scope at frame60 to the last available paired scope. It excludes the first counter interval and uses no synthetic backend-to-host offset correction. The selected live window contains283intervals over28.3003host seconds; bypass279over27.9000seconds. Rates are approximate counter sums divided by those host durations, not claimed exact per-pass times.

This is **whole-device** sampling. The compositor and other GPU clients are included. The idle reference reported zero GPU-active cycles but a small external-write count, so even apparent idle is not assumed to be a zero-overhead instrument. Startup/driver/background contributions are not attributed to individual game frames.

# Measured results

![Mali activity and external traffic comparison](../scripts/03-mali-counters/evidence/counter-comparison.svg)

| Central gameplay observation | Normal live | Post bypass |
|---|---:|---:|
| GPU-active cycles/s, millions |803.27|788.42|
| Fragment-active cycles/s, millions |798.42|704.93|
| External reads, decimal MB/s |1431.82|1528.29|
| External writes, decimal MB/s |747.53|570.46|
| Fragment/GPU-active cycle ratio |99.40%|89.41%|
| Mean per-sample arithmetic utilization |70.66%|64.62%|
| Mean per-sample texture utilization |13.12%|7.89%|
| GPU frequency at every selected poll snapshot |800MHz|800MHz|

The first rate is slightly above800million cycles/s despite800MHz frequency snapshots. Do not clamp that into100%or report an exact busy percentage. Snapshot frequency is not integrated actual frequency, host polls are not exact hardware interval endpoints, and counter calibration is not independently established. The data supports high pending-work activity and a changed fragment/unit mix, not a calibrated statement that a specified percentage of wall time was productive GPU execution.

Bypass lowers fragment activity and external writes, but reads per second increase. This is not contradictory: the bypass case also completes more frames per second and changes the rendering pipeline, texture access pattern and live trajectory. Bytes per second cannot by itself measure the isolated traffic saved by the removed shader. Future per-frame normalization needs carefully matched windows/scenes, not subtraction of these two means.

![Frame interval distributions](../scripts/03-mali-counters/evidence/frame-comparison.svg)

| Case | Mean frame interval ms | p95 ms | p99 ms |
|---|---:|---:|---:|
| No-counter baseline |18.034|32.747|34.162|
| Normal live + counters |18.171|32.754|34.095|
| Post bypass + counters |16.666|16.818|17.392|
| Closing no-counter baseline |18.295|33.009|33.993|

This is a substantial whole-workload change, especially in the upper tail. It is **not an accepted optimization**: post bypass deliberately displays the raw encoded scene instead of source-faithful post-processing. It also removes framebuffer/uniform/draw work and changes dependencies, so the difference cannot be called isolated post-shader GPU milliseconds.

The counter-enabled normal mean falls between the two uninstrumented-counter baselines. That does not expose a large broad slowdown in this sequence, but it is not a quantified overhead bound. CPU/GPU policy was unchanged and uncontrolled, and the two live trajectories are not identical scene replays. Mean sampler-call wall costs were roughly0.18ms per100ms poll in normal live and0.15ms in bypass; these are probe call durations, not total system perturbation.

# What this changes about the next experiment

We no longer need to assume GPU counters are unavailable. We have a working standalone capture path, definitions for the measured quantities and actual live comparison data. The post-processing path is now a useful target for **source-preserving** experiments because omitting it changes both the frame distribution and fragment activity, not because unreliable elapsed-query sums assigned it a cost.

The next useful counter expansion is the G52 queue-dependency, descriptor-read and cache-flush wait counters documented in the saved reference. Those could distinguish particular GPU scheduling conditions without pretending to reconstruct CPU blocked/runnable intervals. Arithmetic-heavy shader changes should be supported by static compiler evidence or specific instruction/cycle counters, and retain original formulas, precision and appearance unless an adaptation is explicitly approved.

The impossible startup GL elapsed results and nonadditive pass totals remain unresolved. Hardware counters are an independent new observation, not a retroactive fix for that timer evidence. Streamline remains a possible richer interface, but installing it is not necessary merely to collect the counters demonstrated here.

# Reproduction and source locations

All tools are in `scripts/03-mali-counters` under this ticket. The numbered scripts inventory, fetch pinned Arm material, cross-build, run bounded cases, collect evidence and regenerate both graphs. Results live in`evidence/target-20261014T024712Z`; exact aggregates and source hashes are in`evidence/summary.json` and`analysis-provenance.json`.

Primary documentation is in`../sources/`: Arm's libGPUCounters README, G52 counter reference, gator setup README extraction and pinned API/formula source excerpts with license and checksums. The Offline Compiler fetch log records a malformed schema.org warning and an empty extraction; its empty file is not treated as useful documentation. No Streamline, compiler or frame capture has been executed in this checkpoint.
