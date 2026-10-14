---
Title: Measured replay resolution and view-count scaling on Mali-G52
Ticket: FALLLINIE-GPU-006
Status: active
Topics:
    - performance
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://ttmp/2026/10/14/FALLLINIE-GPU-006--repeatable-gpu-workload-and-resolution-view-count-scaling/scripts/03-campaign.sh
      Note: Bounded key-authenticated matrix runner
    - Path: repo://ttmp/2026/10/14/FALLLINIE-GPU-006--repeatable-gpu-workload-and-resolution-view-count-scaling/scripts/04-analyze.py
      Note: Frame, replay, image and counter-window validation
    - Path: repo://ttmp/2026/10/14/FALLLINIE-GPU-006--repeatable-gpu-workload-and-resolution-view-count-scaling/scripts/evidence/target-20261014T180649Z/summary.json
      Note: Actual device measurements
ExternalSources: []
Summary: Eight deterministic live-rendering cases show resolution sensitivity in dual-view rendering and an apparent presentation ceiling in lighter configurations.
LastUpdated: 0001-01-01T00:00:00Z
WhatFor: ""
WhenToUse: ""
---

# Actual device results: repeatable resolution and view-count scaling

## 1. What this campaign establishes

The new replay fixture completed all eight device cases with the same selected-state trajectory signature and byte-identical endpoint scene/depth/print images within each internal resolution. We now have a repeatable live-rendering workload rather than timing-dependent autoplay or a terrain-only frozen scene.

Dual-view rendering is materially sensitive to internal pixel count. At 256×192 it averages 16.6202 ms per completed frame; the 384×288 opening/closing references average 17.3886/17.4077 ms; 512×384 averages 23.5738 ms. The largest dual-view case is about 35.5% slower than the mean of the two middle-resolution references. This supports investigating pixel-dependent rendering work, but does not separate scene shading, post-processing, texture traffic, render-target traffic or dependencies.

Every single-view case remains near 60.16–60.17 completed frames/s even though fragment activity increases substantially with resolution. Application pacing is disabled in these cases. The result is consistent with a presentation/compositor constraint masking available throughput in lighter configurations; it is not proof of the exact synchronization mechanism. `--no-vsync --fps 0` means the application requests no swap interval and performs no explicit FPS pacing, not that the entire output path is unthrottled.

No rendering shortcut is promoted. Lower internal resolution changes image quality and is a sensitivity test, not a claim that the same pixels can be generated more cheaply. Production defaults and launcher remain unchanged.

![Resolution and view-count scaling](../scripts/evidence/target-20261014T180649Z/scaling.svg)

## 2. Configuration and evidence identity

- Campaign: `target-20261014T180649Z`.
- Device deployment: `/storage/apps/falllinie-gpu006-20261014T180649Z`.
- Runtime implementation: `32b8bfd640f44c465d3f8b6311400f35eac07e81`.
- Executable SHA256: `90f124a7745a37f76175130bd79ef748b741852bccfe58305548f7946c0d82ed`.
- Counter probe SHA256: `1f346e0801c99672e1b22edf26cca5a840f97c5cafd6f041de23d052697c65e1`.
- Fixed seed 42, 600 frames, 1/60-second simulation step, 32 terrain row-work units per frame.
- Audio disabled; real game, rider, particles, trails, HUD and cameras retained.
- Ordinary indexed draws, original post-processing, developer overlay off, elapsed GPU queries off.
- Three 4:3 internal targets; output remains full-size compositor surfaces.
- Paired CPU/wall scopes every 60 frames; 45-second bounded device-counter capture per case.

The candidate was built before its implementation commit. `scripts/evidence/build/qualification.json` verifies all 31 captured source files against that commit; `local-binary.sha256` and `deployed.sha256` match. Campaign `source=` records the repository checkpoint at launch, not a claim that a later documentation commit produced different executable bytes. The key-only SSH runner was a tooling-only edit at launch; runtime source and binary remained unchanged.

The earlier failed attempt at17:46:58 is preserved separately. It ran no device workload. The successful campaign used the user's Ed25519 public key, installed with authorization, and noninteractive key-only authentication. No password value is stored in this ticket.

## 3. Repeatability and health checks

All eight receipts contain selected-state signature **6081555844518115723**. The rolling signature covers ordered game position/velocity/time/score, seed/generation, top-camera eye/look and terrain vertex/index counts. It deliberately excludes real clocks and resolution. It is a consistency check over selected fields, not proof that every game field or GPU operation is identical.

The analyzer also compares final scene, depth and print BMP bytes among cases of the same resolution. All comparisons pass, including the single/dual-view and paced/unpaced 384×288 cases. Images at different resolutions are not expected to match. These are endpoint checks, not every-frame visual equivalence or complete source parity. The post-loop top-camera redraw is uninstrumented; captures read internal scene/print targets and a depth diagnostic, not the final full-size HUD/compositor surface.

There are 4,800 completed frames, 4,320 postwarmup frame intervals and 1,330 paired records. Every paired final record reconciles attempted/written counts; active/dropped/invalid counts are zero. All game/probe statuses are zero, all receipts report GL_error=0, and placement succeeded.

The probe produced 46,800 counter rows, with zero collection errors or nonfinite values in this campaign. 674 adjacent host-poll intervals are fully contained inside the postwarmup paired-clock window. The counter window is narrower than all 540 postwarmup frames because the last sampled frame is 540 rather than 599. No exact per-frame or per-pass counter attribution is inferred.

The existing driver prints the following nonfatal startup diagnostic, retained in the raw logs:

```text
Could not open module param file '/sys/module/bifrost_kbase/parameters/large_page_conf'
```

The game and probe still complete successfully. We did not change kernel modules or settings to suppress it.

## 4. Measured frame distributions

Every row summarizes frames 60–599, 540 intervals. Loop rate is 1000 divided by mean interval; it is not physical presentation or scanout FPS.

| Case | Internal size | Views | Requested FPS cap | Mean ms | p95 ms | Loop frames/s |
|---|---|---:|---:|---:|---:|---:|
| Opening reference |384×288|2|0|17.3886|24.3771|57.51|
| Small single |256×192|1|0|16.6211|18.1520|60.16|
| Small dual |256×192|2|0|16.6202|17.3690|60.17|
| Middle single |384×288|1|0|16.6182|18.2585|60.17|
| Large single |512×384|1|0|16.6221|18.4810|60.16|
| Large dual |512×384|2|0|23.5738|33.4894|42.42|
| Closing reference |384×288|2|0|17.4077|25.2859|57.45|
| Paced reference |384×288|2|60|17.3567|23.4675|57.61|

The opening/closing mean drift is about 0.110%, much smaller than the large-dual slowdown. Their p95 values differ by about 0.91 ms, showing that tails are not identical even when the replay matches. Nonreference cases have one run each; this is exploratory scaling evidence, not a confidence interval or a claim of stable long-session 60 FPS.

The paced middle-resolution reference remains close to the unpaced references. It does not establish that pacing is irrelevant in general. At this workload, requested 60 FPS does not overcome the rendering/presentation limit.

## 5. Hardware activity changes when frame timing does not

Rates use adjacent host completion intervals wholly inside the selected paired-clock extent. They include compositor activity and are not application-only. Decimal MB/s means 10^6 bytes/s.

| Case | GPU active Mcycles/s | Fragment active Mcycles/s | External reads MB/s | External writes MB/s |
|---|---:|---:|---:|---:|
|384×288dual opening|804.56|801.72|1498.58|770.00|
|256×192single|457.25|317.48|873.30|531.00|
|256×192dual|652.47|511.80|1385.05|685.85|
|384×288single|593.25|447.61|947.99|594.00|
|512×384single|745.46|634.78|1042.53|660.37|
|512×384dual|804.67|801.94|1159.59|652.11|
|384×288dual closing|803.69|799.69|1494.78|768.34|
|384×288dual paced|804.47|801.18|1502.56|771.70|

Single-view 256×192 to 512×384 quadruples internal pixel count while fragment activity rises from 317.48 to 634.78 Mcycles/s, approximately doubling, with almost no change in loop rate. This is an important counterexample to interpreting unchanged frame time as unchanged GPU demand. Fixed-size output and compositor work, and other non-scaling work, are still present.

In dual view, fragment activity reaches a similar rate near 800 Mcycles/s at both 384×288 and 512×384 while loop throughput falls at the larger resolution. This is consistent with increased per-frame demand under a rendering-capacity constraint. It does **not** calibrate the GPU as 100% productively busy. Pending or stalled work may count as activity, and observed rates slightly above the 800 MHz snapshot already require caution about rate/clock semantics.

External byte rates are lower in the slower 512×384 dual case than the 384×288 dual references. That does not mean each frame necessarily transfers fewer bytes: fewer application frames complete per second, and compositor contributions remain. We do not divide these rates by the full-frame summary denominator because the windows differ; precise per-frame normalization is deferred. These measurements do not establish a bandwidth bottleneck.

## 6. CPU scopes and presentation limitations

For single-view runs, sampled top-view CPU means remain around 3.81–3.96 ms while top-output wall means rise from 9.76 to 11.88 ms across the resolution range. The output scope's CPU contribution is only about 0.92–1.03 ms. This indicates substantial elapsed time not executing on the main thread in that scope, but it does not identify whether the thread is blocked, descheduled or waiting on a particular fence/compositor dependency.

For small dual view, the top-output scope averages 0.978 ms wall while lower output averages 8.122 ms wall. The same apparent waiting can appear at different points in the sequential output path. That is why a cheap output function cannot be assigned the entire wall duration as its own isolated computation cost.

Each scope has only nine postwarmup samples per relevant view. They are selected frame boundaries, not continuous per-frame CPU traces. Compare scope means as supporting evidence; do not add nested scene/post/output scopes to their parent view scope.

## 7. Environmental and methodological limits

Selected GPU frequency snapshots are 800 MHz in all cases. Temperature snapshots span roughly 50.6–53.1°C across the campaign. CPU frequency snapshots vary, including 816/1104/1992 MHz, and the after snapshot occurs after the 45-second probe finishes, not at game exit. Neither before/after samples nor GPU snapshots prove fixed clocks throughout the workload. No governor, affinity, driver or display policy was changed.

Counter backend timestamps retain their previously observed mismatch with host time. The analyzer records offsets rather than silently fitting an alignment. Whole-device counters can include background/compositor work; lower display hardware is not powered off merely because only one game window is rendered.

The replay differs from production in deliberate ways: fixed simulation progress per rendered frame, fixed streaming work units, no audio, no physical gameplay input and no persistent score access. It supplies 600 raw frame-clock updates of 1/60 second for one seed. The original game time-scale and 120 Hz physics accumulator remain active; ten seconds of raw input does not promise exactly ten unscaled physics seconds. A longer route or a different game mode may expose other bottlenecks.

Observer overhead is not separately qualified in this matrix. All cases use the same paired-clock/counter setup, but that does not make its cost zero. The successful analysis is structural and consistency evidence; it cannot repair limitations in the underlying counter definitions or presentation timing.

## 8. Decision and next experiment

Retain the benchmark mode and its tooling as an opt-in diagnostic. Keep production resolution, shader formulas and indexed-draw defaults unchanged. The initial ticket's design, replay implementation and scaling measurement objectives are met; no claim of a newly accepted source-faithful frame-time optimization is made.

The next useful GPU experiment would hold this replay and a demanding dual-view configuration constant while changing one clearly labeled diagnostic component: for example a cheap copy instead of the print post shader, or a deliberately simplified scene fragment shader. Those would be non-parity sensitivity tests, not automatically acceptable optimizations and not isolated pass-time measurements. Separately, understanding the apparent 60 Hz presentation ceiling would improve uncapped throughput experiments, but that should be a bounded explicit investigation rather than an unannounced compositor-policy change.

## 9. Reproduction and review

From the native repository, use the ticket's `scripts/03-campaign.sh` with the configured SSH key. No askpass or password environment is required. Then run:

```text
python3 <ticket>/scripts/04-analyze.py <ticket>/scripts/evidence/target-<stamp>
python3 <ticket>/scripts/06-results-figure.py
```

The analyzer accepts a campaign directory; the figure helper deliberately pins this published checkpoint. Read summary.json and analysis-provenance.json for exact numbers and input hashes. Start code review with the benchmarkguards in main.cpp, TerrainJob's shared work-unit loop and the analyzer's frame/signature/image/final-record checks. The original implementation guide describes the architecture; the diary records the authentication interruption and this actual measurement checkpoint.
