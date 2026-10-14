---
Title: Measured Cortex-A55 PMU baseline without audio
Ticket: FALLLINIE-TRACE-005
Status: complete
Topics:
    - performance
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://src/main.cpp
      Note: Audio-disabled diagnostic mode and receipt
    - Path: repo://ttmp/2026/10/13/FALLLINIE-TRACE-005--cpu-gpu-driver-and-scheduler-attribution-with-perf-ftrace-and-ebpf/scripts/10-cpu-pmu/03-target-case.sh
      Note: Main-thread PMU groups and bounded lifecycle
    - Path: repo://ttmp/2026/10/13/FALLLINIE-TRACE-005--cpu-gpu-driver-and-scheduler-attribution-with-perf-ftrace-and-ebpf/scripts/10-cpu-pmu/05-analyze.py
      Note: Coverage validation and counter ratio aggregation
ExternalSources: []
Summary: Five main-thread PMU groups and a cycles profile succeed; driver-heavy low-IPC baseline does not yet establish a cache bottleneck.
LastUpdated: 0001-01-01T00:00:00Z
WhatFor: ""
WhenToUse: ""
---

# Actual CPU PMU results

## Workload and provenance

Eight bounded device cases completed under `/storage/apps/falllinie-pmu-20261014T144623Z`. Source implementation commit323b095 adds `--no-audio`; deployed binary SHA256 is `c2b0fc135c880766a2f1dde2ed819a5aa922654ddcbae460c9a2e36cc141c9c1`. Every recorded src hash was verified against that commit. The build originally recorded the earlier parent HEAD because the code was committed after building; qualification.json makes that distinction explicit.

Each game ran32s, with6s startup/warmup before a20s collection. Fixed seed42, dual384×288, no-vsync,60FPS target, autoplay, overlay, paired clocks cadence60, GL timer queries off. **Audio device, producer, callbacks and ongoing DSP are disabled**, not merely muted. Inert AudioEngine storage and game-side feedback calculations remain. No shaders, production deployment, affinity, governors or kernel settings changed.

The thread inventory in all eight cases contains one main `falllinie-pmu` thread and Mali workers; no audio threads. Counters attach to the directly launched game's mainTID using `--no-inherit -t PID`; kernel and userspace execution charged to that thread are included. Other threads are not counted, although they can still affect shared resources. This is a reduced diagnostic workload, not full-game performance acceptance.

## Counter results

Explicit `armv8_cortex_a55` events avoid generic `cycles` expanding to the unrelated `rockchip_ddr/cycles/` alias. The latter reported `<not supported>` during initial qualification. Five pairs each produced20valid one-second intervals, with100% reported running coverage and no unsupported/not-counted rows.

| Pair: denominator → numerator | Denominator total | Numerator total | Ratio |
|---|---:|---:|---:|
| cpu_cycles → inst_retired | 14,300,334,687 | 2,078,852,033 | **0.14537 instructions/cycle** |
| l1d_cache → l1d_cache_refill | 695,609,378 | 31,780,725 | **4.5688%** |
| l2d_cache → l2d_cache_refill | 164,525,671 | 40,142,728 | **24.3991%** |
| br_retired → br_mis_pred_retired | 367,486,825 | 35,451,416 | **9.6470%** |
| l1d_tlb → l1d_tlb_refill | 690,887,026 | 32,798,337 | **4.7473%** |

Each ratio divides totals from its simultaneously scheduled pair. Pairs were measured in **different runs**, so do not combine their totals into a synthetic same-run cache hierarchy or instructions-normalized statistic. L2 and L1 events need not describe identical traffic populations; L2 refills greater than L1 refills are not evidence of a counting error by themselves. Cache refill is not synonymous with DRAM access, and refill/access is not a measured latency fraction.

100% running coverage means no observed PMU multiplexing loss while the event was enabled for the thread. It does not mean100% CPU use: the cycles group accumulated10.0136s of event running time across20.0062s elapsed. Its14.3003billion cycles imply approximately1.428GHz averaged over that running time. The recorded governor remained ondemand; point frequency snapshots are context, not frequency control.

IPC is low in this workload, but these counters alone do not separate dependency stalls, instruction mix, memory latency, synchronization/polling or driver-specific behavior. The data supports further attribution, not a declaration that the cache is the bottleneck.

## Main-thread cycles profile

A separate199Hz requested frequency-mode CPU-cycles recording collected **1,873 samples, zero reported lost**. Frequency-mode event periods adapt; perf's percentage is cycle-period-weighted attribution, not a raw sample-count fraction and not whole-frame GPU utilization. Actual sample rate need not equal199per wall second because only one thread's executing activity is selected.

| DSO | Reported main-thread cycle weight |
|---|---:|
| libmali.so.1.10.0 | **71.81%** |
| falllinie-pmu | 10.65% |
| libc.so.6 | 6.99% |
| kernel | 4.79% |
| libwayland-client | 1.91% |
| SDL2 | 1.50% |
| libm | 1.40% |

One unsymbolized libmali offset,`0x42d6dc`, accounts for16.89% in the symbol report. That identifies a location worth inspecting; it does not identify a GL operation or prove the instruction at that address caused a cache miss. Driver symbols and precise event skid remain limits. The application also exposes rider geometry/render/mesh-building functions in the profile.

This is actual hardware event/PC sampling, **not high-frequency general-purpose register dumps**, precise data-address sampling or per-instruction cache latency. The earlier99Hz cpu-clock study and this cycles-weighted study support a similar driver-heavy interpretation, but their percentages come from different events and workloads.

## Frame and health context

Postwarmup frame means for no-counter opening/closing brackets were18.267517/18.275653ms. Counter runs ranged18.199694–18.303829ms; the profile run averaged17.991269ms. Corresponding p95 values remained approximately27.65–28.21ms. These finite sequential runs show no obvious gross observer slowdown, but do not establish zero overhead or a causal audio-off improvement relative to older campaigns.

All eight runs completed withGL_error0 and audio_disabled=true; paired sample totals/final counts reconcile with zero dropped/invalid/active scopes. All thread identities match the directly launched PID. Whole-run frame statistics use frame>=60; they are not silently divided into the shorter PMU interval. Paired scopes are additionally summarized only when fully inside the recorded outer collection envelope. That envelope includes perf startup/teardown and is not an exact hardware enable boundary.

## Review and reproduction

- `scripts/10-cpu-pmu/01-inventory.sh`: actual PMU aliases/encodings and explicit group qualification.
- `02-build.sh`: existing host/ARM toolchain, CPU tests, source/executable receipts.
- `03-target-case.sh` and `04-campaign.sh`: process ownership, event groups, warmup, bounded lifecycle and private deployment.
- `05-analyze.py`: interval/scope validity, coverage, ratio aggregation and identity checks; small valid/invalid fixtures exercise rejection of unsupported or multiplexed counts.
- `evidence/target-20261014T144623Z`: rawCSV, logs, receipts, thread inventories and decoded perf output.
- `evidence/summary.json`, `analysis.txt`, `analysis-provenance.json`: reproducible aggregates and input hashes.

Raw perf.data remains on the private device deployment, with a retained SHA256; decoded reports are committed. Build receipts were explicitly included despite the repository's generic build-directory ignore rule. Fifteen CPU tests and the host no-audio smoke passed at the feature boundary. The actual target campaign qualifies PMU access and autoplay no-audio behavior; interactive audio-start exclusion is code-guarded, not separately key-injection-tested.

## Next justified step

Use the PMU baseline for a single future optimization comparison. Before assuming uniform lookup cost, consider a refill- or branch-event location profile and interpretation/disassembly of the prominent libmali hotspot. Such samples are not precise load blame; event skid and private driver code must remain explicit. No shader rewrite, uniform cache or other optimization is bundled into this measurement work. Audio integration and listening remain separate later acceptance work.
