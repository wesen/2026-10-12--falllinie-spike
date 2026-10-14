---
Title: Paired clock collection and analysis
Ticket: FALLLINIE-TRACE-005
Status: active
Topics: [performance]
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: src/paired_clocks.cpp
      Note: Bounded main-thread clock recorder
    - Path: src/main.cpp
      Note: Opt-in CLI and frame stage boundaries
ExternalSources: []
Summary: TRACE005 JSONL v1 semantics, commands and interpretation limits.
LastUpdated: 2026-10-14
WhatFor: Collect thread CPU and monotonic wall pairs without redefining PERF004.
WhenToUse: Before parent-controlled paired clock and perf experiments.
---

# Collect

```sh
./build/falllinie-spike --autoplay --dual-screen --seconds 20 \
  --profile /tmp/live.csv --paired-clocks --profile-cadence 60 \
  --gpu-profile-mode off
T=ttmp/2026/10/13/FALLLINIE-TRACE-005--cpu-gpu-driver-and-scheduler-attribution-with-perf-ftrace-and-ebpf
python3 "$T/scripts/paired-clocks/analyze.py" /tmp/live.csv.paired.jsonl
```

`--paired-clocks` requires `--profile`; omission performs no paired wall/thread clock or PID/TID reads and creates no sidecar. Existing audio clocks are unchanged. GPU profiling is independent: use off first, then view/passes when desired. All paired scopes use the existing cadence (`frame % cadence == 0`, including frame 0). Select finite frames/seconds and a unique output prefix per run; files are truncated on opening. RAM storage is fixed at 64 completed records, not duration-dependent; disk output grows with the explicitly bounded run. Defaults and all legacy CSV headers/field meanings are unchanged.

# Schema and boundaries

The separate `PROFILE.paired.jsonl` contains one `meta`, zero or more `sample`, and exactly one terminal `final` record, every line newline-terminated. All records carry `schema:1`, Linux `pid`, `tid`, and `run_start_ns` (a successful CLOCK_MONOTONIC startup read). These identify this main-thread process run within a host boot, not globally across boots. Preserve boot ID, source commit/build ID, executable/maps, perf invocation and target configuration separately with the experiment. Run `perf record --clockid mono` only if the installed perf/event supports it; do not assume default perf timestamps share this clock. No GPU clock alignment is claimed.

Example shape (constructed, not measured):

```json
{"record":"meta","schema":1,"pid":1200,"tid":1200,"run_start_ns":1000000,"cadence":60,"capacity":64,"wall_clock":"CLOCK_MONOTONIC","cpu_clock":"CLOCK_THREAD_CPUTIME_ID"}
{"record":"sample","schema":1,"pid":1200,"tid":1200,"run_start_ns":1000000,"sequence":1,"frame":0,"view":0,"pass":"output","stage":"render","wall_start_ns":2000000,"wall_end_ns":3000000,"cpu_start_ns":100000,"cpu_end_ns":400000,"wall_status":"valid","cpu_status":"valid"}
{"record":"final","schema":1,"pid":1200,"tid":1200,"run_start_ns":1000000,"attempted":1,"written":1,"dropped":0,"invalid_wall":0,"invalid_cpu":0,"active":0}
```

- Render stage: original `view`, `scene`, `post`, `output`, `dynamic_prep`, `dynamic_upload` pass identities, views 0/1. Existing GPU query begin/end remain outside both inner clocks. Paired wall clocks bracket the unchanged legacy steady-clock body boundaries; a few boundary read/instruction costs therefore differ. Nested scopes are **nonadditive**, and parents include child instrumentation costs.
- Events (`view:-1`, `pass:none`): event polling, input mapping and input actions; excludes top context restoration and profiler flushing even though the existing legacy events wall interval includes those. It can include audio-open/start and input-triggered game transitions.
- Physics (`view:-1`): controls/autopilot and frame-clock advance; emitted only when the existing play-and-not-quit branch executes.
- Player (`view:-1`): native feedback plus all `update_player`, **including streaming CPU work and GL uploads**, rider updates/uploads, HUD/camera updates. Stream is not a separate paired child; legacy nested `stream_ms` is unchanged and cannot be subtracted from paired CPU.
- Context (`view:0/1`): per-frame MakeCurrent and SetSwapInterval for dual-window switching. Absent for single-window mode; startup/capture switches are not sampled.
- Swap (`view:0/1`): SDL_GL_SwapWindow. Lower absent without lower window.
- Pacing (`view:-1`): deadline calculation and optional sleep; exists even when the cap is disabled (small valid interval).

This is deliberately not a complete frame partition: profiler writes/polling, overlay refresh, feedback publication/logging, drawable-size/setup work outside render view, frame statistics and shutdown are not paired. Legacy interval/stage wall columns continue to include their original work; instrumentation itself can perturb all measurements.

Read order is wall-start, CPU-start, body, CPU-end, wall-end. Failed reads are `null`, **not zero**; timestamp zero is a valid successful read. Status per clock is `valid`, `start_failed`, `end_failed`, `both_failed`, or `regressed`. Initialization fails explicitly if monotonic run identity cannot be read. CPU unavailability remains an invalid sample, not a failed application startup.

The sequence increments at scope entry; nested scopes complete out of sequence. Samples are buffered at scope exit with no serialization/allocation/GL calls. At the next profiler poll (or final destruction), records flush only when no paired scopes are active. Overflow drops newly completed records and increments `dropped`, while validity counters include even dropped attempts. Final `attempted = written + dropped` and `active = 0` are required for completeness. A write error is logged; absent/malformed final record or mismatched counts rejects the collection. A terminal record establishes scope transport completeness, not successful game completion or a completed requested duration: preserve exit status/receipt separately, including exceptions and termination.

# Analyze

Analyzer validates schema, clock names, run/PID/TID identity, cadence, stage/pass/view identity, sequence and scope uniqueness, nullable timestamps/status, final counts and validity bounds. Truncation/mismatch returns nonzero. Report contains paired counts and mean wall/thread CPU/non-running-estimate nanoseconds grouped by exact stage/view/pass. Both durations in each mean come from the **same valid paired rows**. Invalid-only groups have null means, not zero. Known drops/invalid reads remain visible and make `all_attempted_samples_valid` false, even when `transport_complete` is true. Negative wall-minus-CPU is preserved and counted, never clamped away.

**Wall minus thread CPU is only an estimated non-running interval with read skew and accounting residual. It is not GPU wait, blocked time, runnable delay, GPU utilization or proof of a driver stall.** Distinguishing those requires supported external evidence. Target FTRACE/KPROBES absence does not prevent these clocks or supported perf sampling, but limits scheduler reconstruction.

# Host boundary / ARM build

```sh
cmake -S . -B build-paired -DCMAKE_BUILD_TYPE=Release
cmake --build build-paired -j2
ctest --test-dir build-paired --output-on-failure
```

C++ deterministic injectable-clock tests exercise failure/zero/regression paths, no-clock defaults, cadence, nesting, unwinding, drops, final counts and profiler integration. Python tests consume those emitted files and reject corrupted/truncated/count/identity variants, plus exact paired durations and signed residuals. Python is optional at configure time; it must be present to run the analyzer tests. No GPU or target is needed for these tests. Hardware/perf qualification and observer-effect runs belong to the parent.

`scripts/build-device.sh` compiles `src/*.cpp` and therefore automatically includes `paired_clocks.cpp`; explicit external source lists must add it. For useful perf callchains use the existing AArch64 toolchain plus `-g -fno-omit-frame-pointer` (optionally `-fno-optimize-sibling-calls`) and preserve the exact unstripped ELF/build ID. Keep optimization `-O2` for representative work. Do not add glFinish, link a new tracing dependency, or alter SDL/GL thread ownership. `clock_gettime`/gettid use libc/syscall on the current Linux target; an older libc toolchain may need `-lrt`. The supplied device build helper fetches libraries and builds a container, so this worker did not run it; parent can use already available toolchain/libraries without installs or device policy changes.
