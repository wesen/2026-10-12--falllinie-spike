---
Title: CPU PMU qualification and audio-disabled render-loop measurement
Ticket: FALLLINIE-TRACE-005
Status: active
Topics: [performance]
DocType: design-doc
Intent: long-term
Summary: Explicit Cortex-A55 PMU, main-thread counting, and a genuinely audio-disabled diagnostic workload.
---

# CPU PMU measurement design

## Objective
Determine whether main-thread rendering work is instruction-heavy, associated with cache/TLB refills, or branch activity before choosing an optimization. This adds CPU hardware event counts to existing thread clocks and GPU measurements; it does not dump general-purpose registers or prove a stall's cause.

## Workload and scope
Add `--no-audio` to prevent AudioEngine::open on autoplay and interactive starts. No SDL audio device, producer thread, callback or ongoing DSP starts. Retain the inexpensive game feedback state/events and their normal draining; graphics, game RNG and simulation authority remain unchanged. AudioEngine construction may retain inert storage. Record audio-disabled mode in logs and receipts; muted remains a separate DSP-enabled mode. This is an explicitly reduced diagnostic workload, not an integrated-game performance qualification.

Use a separate candidate deployment, fixed seed42, dual384x288, no-vsync,60FPS target, autoplay, paired clocks cadence60, queries off. Do not change shaders, governor, affinity, kernel policy or production launcher. A no-counter run brackets the counter cases. Main TID equals the launched process PID; attach `perf stat --no-inherit -t PID`, not process-wide child inheritance. Record thread inventory and exact collection bounds. Allow warmup before collection, and compare only paired scopes fully within perf's recorded collection envelope.

## PMU qualification and collection
Read `/sys/bus/event_source/devices/armv8_cortex_a55/{type,events,format}` and perf version. Qualify explicit PMU event syntax rather than generic `cycles`: this target also exposes rockchip_ddr, and generic cycles expanded to an unsupported DDR event during reconnaissance.

Collect small simultaneously scheduled groups in separate runs:
1. cpu_cycles + inst_retired (IPC).
2. l1d_cache + l1d_cache_refill (refill/access ratio).
3. l2d_cache + l2d_cache_refill (same event-domain ratio, not DRAM probability).
4. br_retired + br_mis_pred_retired (retired branch events).
5. l1d_tlb + l1d_tlb_refill (translation activity).

Use explicit kernel-exposed aliases, not guessed raw event IDs. Perf interval output at1s provides counts accumulated continuously in hardware, not1Hz event observation. Preserve time-running percentages and errors; reject unsupported/not-counted/zero-denominator intervals and do not silently treat multiplexed data as full-coverage. Results include kernel activity charged to the main thread; no whole-device or GPU attribution.

Optionally collect a bounded cycles-based199Hz PC/frame-pointer profile after counting succeeds. This is overflow-based instruction-location sampling, not register-state capture, precise load-latency tracing or proof that the sampled instruction caused a cache miss. Preserve sample/lost diagnostics; no demand for accurate stacks in stripped driver code.

## Evidence and decision
Keep commands, PMU event encodings, executable/source hashes, thread inventories, raw perf interval CSV, application logs/receipts/paired records and derived summaries under scripts/10-cpu-pmu/. Runtime build artifacts remain ignored. Record unavailable capabilities explicitly. Ratios are descriptive for this workload, not sufficient causal proof. No optimization is bundled with measurement implementation.

## Validation boundary
Build the feature once, run existing CPU tests, verify no-audio live startup and no audio producer/callback thread on target, then execute the bounded PMU campaign. Reconcile perf coverage and sample validity, main PID/TID, paired-clock completeness and GL health. Retain initialization versus measurement boundaries; report frame scope separately from counter scope. Audio-enabled performance and listening remain later work.
