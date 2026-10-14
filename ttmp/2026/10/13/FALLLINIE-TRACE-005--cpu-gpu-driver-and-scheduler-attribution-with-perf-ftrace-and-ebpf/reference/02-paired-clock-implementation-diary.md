---
Title: Paired clock implementation diary
Ticket: FALLLINIE-TRACE-005
Status: active
Topics: [performance]
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: src/paired_clocks.cpp
      Note: Fixed-capacity paired recorder and versioned sidecar
    - Path: src/profiler.cpp
      Note: Existing render-scope integration without CSV redefinition
    - Path: src/main.cpp
      Note: Explicit flag and non-render frame stages
    - Path: tests/test_paired_clocks.cpp
      Note: Deterministic clocks and emitted analyzer fixtures
    - Path: ttmp/2026/10/13/FALLLINIE-TRACE-005--cpu-gpu-driver-and-scheduler-attribution-with-perf-ftrace-and-ebpf/scripts/paired-clocks/analyze.py
      Note: Independent sidecar validity/count/identity analysis
ExternalSources: []
Summary: One coherent opt-in paired-clock tooling slice, one host feature-boundary validation, and explicit parent-owned hardware/perf follow-up.
LastUpdated: 2026-10-14
WhatFor: Preserve implementation decisions, actual validation and review limits.
WhenToUse: Reviewing the paired-clock commit or building the parent measurement campaign.
---

## Step 1: Build the entire paired-clock measurement slice before the host gate

Implemented the approved measurement path in the isolated managed worktree based on `c9c65f9`, after reading AGENTS, profiler/header, main, audio timing, TRACE005 design01 and parent diary Step 3. The new opt-in records paired thread CPU and absolute monotonic timestamps for existing render scopes plus events, physics, player including stream, context switches, swaps and pacing. It does not change gameplay, rendering algorithms, shaders, RNG, GL ownership or audio callbacks.

The complete recorder, integration, analyzer, representative tests and usage guide were built first, then validated together once. All 15 host CTest tests passed, including unchanged profiler lifecycle checks and the new paired/analyzer tests. Hardware/perf operation, native/vault pushes and independent acceptance review remain with the parent. No claim of GPU-wait attribution or measured ARM observer effect is made.

### Prompt Context
**User prompt (verbatim):** "also don't overcautiously test and measure, we can run proper runs once all the things are built pretty quickly, no need to test every little change that won't actually bring us closer to proper measurements. this is not production software but debugging / dev tooling"

**Earlier implementation request (summary, not verbatim):** Implement the approved tracing plan, commit at meaningful boundaries, and maintain a detailed diary in the skill's investigation format. The original bytes of that earlier request were unavailable to this implementation handoff; no reconstructed quotation is asserted here.

**Assistant interpretation:** Ship one bounded optional paired-clock developer tool with useful main-thread scope coverage and an independently checkable sidecar; avoid repeated validation matrices and defer proper device experiments to the parent.

**Inferred user intent:** Get directly to practical CPU-versus-non-running measurements instead of elaborating a general tracing framework or spending cycles on incremental test ceremony.

**Commit (code):** The coherent feature commit containing this entry; exact resulting hash is recorded in the implementation handoff (not recursively embedded in its own contents).

### What I did
- Added `PairedClocks` and named `FrameStage` in `src/paired_clocks.hpp/.cpp`. Two injectable boolean-success clock functions let tests distinguish a valid zero timestamp from an unavailable reading. Runtime uses CLOCK_MONOTONIC and CLOCK_THREAD_CPUTIME_ID; Linux PID/TID are captured once on enabled initialization.
- Added `--paired-clocks`, rejected without `--profile`, using existing `--profile-cadence`. Default construction has no paired clock or identity reads, and opens no paired file. Existing audio timing is untouched.
- Integrated existing `Profiler::Scope` boundaries and added main-loop events, physics, inclusive player, context, swap and pacing scopes. Legacy steady-clock columns remain wall durations with the same workload meaning; no PERF004 analyzer/schema rewrite.
- Added separate JSONL schema 1 sidecar at `PROFILE.paired.jsonl`, fixed 64-record storage, begin sequence IDs, explicit failed/regressed clock statuses/nulls, overflow counters and final reconciliation. Writes occur during poll/destruction outside paired scopes. A failed output stream produces a stderr error and an incomplete/rejected collection, not a fabricated success.
- Added ticket-local `scripts/paired-clocks/analyze.py`, `test_analyze.py` and `00-usage.md`. Analyzer groups exact stage/view/pass pairs, validates transport/schema/identity/counts and reports signed wall-minus-thread-CPU only from same-row valid pairs.
- Added C++ fixture tests and CMake wiring. Python tests consume emitted C++ fixtures and mutate copies in temporary directories; no device fixture is invented.
- Ran one Release configure/build/full CPU CTest gate and small CLI/analyzer/diff checks at that completed feature boundary. No sanitizers, repeated mode matrices, device commands or installs.

### Why
An independent versioned sidecar avoids silently changing `cpu_ms` or coupling this feature to PERF004's sample closure analysis. A fixed array and deferred output avoid per-scope allocation or file writes. Keeping failed timestamps null prevents unavailable thread clocks becoming fake zero CPU and therefore fake wait time.

Stage scopes cover the important work outside renderer view boundaries without refactoring game or stream code. Player includes the full update (including streaming/GL uploads); no new stream child was needed to satisfy inclusive player attribution. Context stages separate per-frame main-thread context restoration from input events and lower rendering. This is useful coverage, not a claim that all frame time is partitioned.

### What worked
- `cmake -S . -B build-paired -DCMAKE_BUILD_TYPE=Release && cmake --build build-paired -j2 && ctest --test-dir build-paired --output-on-failure` succeeded. GNU C++ 13.3.0, SDL2 2.30.0 and Python 3.13.2 were already available. Application and all test executables built with no reported compiler warnings. **15/15 tests passed**, total CTest time **6.42 seconds**.
- Existing `profiler` test passed unchanged, preserving query pool/disjoint/pending/nesting/cadence and legacy CPU row counts.
- New C++ coverage passed: disabled profiler/recorder and unsampled frames perform no injected clock calls; successful zero CPU timestamp; nested begin/completion order; flush deferral while active; RAII exception unwind; start/end/both clock failures; regressions; fixed buffer drop reconciliation; render plus frame-stage integration; startup monotonic failure.
- New analyzer tests passed exact deterministic timing (`wall=100 ns`, `thread CPU=20 ns`, `non-running estimate=80 ns`), nested parent `300/60/240 ns`, invalid-only null means, 71 attempted / 65 written / 6 dropped, truncated and malformed output, identity/cadence/sequence/status/count corruption, records after final, and retained negative residuals.
- `./build-paired/falllinie-spike --help` advertises the new flag. `./build-paired/falllinie-spike --paired-clocks` exits 1 with `fatal: paired clocks require --profile CSV`, before video initialization; this is the expected guard, not a test failure.
- `python3 "$T/scripts/paired-clocks/analyze.py" build-paired/paired-clock-test/valid.jsonl` reports 4 paired rows, no invalid/dropped/active scopes and `transport_complete: true`. These are synthetic clock fixtures, not performance measurements.
- `git diff --check` passed. All generated builds/fixtures remain under ignored `build-paired`; no library, executable or target trace is committed.

### What didn't work
No implementation/build/test failure occurred in the feature gate. An initial inspection command also listed two possible skill directories; one nonexistent directory made that combined inspection command exit 2 despite successfully reading the source/docs. The actual diary skill exists under `/home/manuel/.pi/agent/skills/diary` and was read fully with its investigation template; no implementation was blocked.

Target graphics, actual CLOCK_THREAD_CPUTIME_ID cost, matched off/on runs, and perf event/unwinder support were intentionally not exercised by this worker. The current device lacks FTRACE/KPROBES and BTF/tooling according to parent inventory, not according to any new worker probe.

### What I learned
Record transport completeness is different from a completed requested game run: RAII can finalize a well-formed partial collection during exception unwinding. Exit status and application receipt remain required. Likewise known overflow is not truncation: the final counter lets the analyzer account for missing sequence IDs but still marks the attempted sample set incomplete.

A sampled player call measures streaming and GL uploads already outside render without touching that implementation. Changing measurement plumbing does not require moving GL or changing stream scheduling.

### What was tricky to build
Paired read order must be explicit: wall-start, CPU-start, measured body, CPU-end, wall-end. Legacy steady-clock body timestamps sit inside the paired wall bracket in renderer scopes, with original GPU query begin/end outside both. This retains legacy wall meaning while making read skew/overhead a visible limitation. Child instrumentation is included in parent duration and GPU query elapsed can be perturbed; no additivity or perfect same-clock boundary is claimed.

Nested scopes exit out of begin order, so sequence validation checks uniqueness/range instead of imposing monotonic output ordering. Buffer flush is skipped while any paired scope is active. Failure counters cover all completed attempts including overflow, so the analyzer validates observed invalid counts against total invalid counts with bounds for dropped samples rather than pretending dropped validity is known.

### What warrants a second pair of eyes
Independent reviewer should check boundary ordering, no-clock default, inclusive player coverage, missing-stage semantics, fixed storage/flush lifetime, final count reconciliation, output failure/truncation handling and honest interpretation. Confirm original shaders/render/game/audio files are not edited. No independent reviewer verdict is claimed by this worker.

The recorder is main-thread-only by contract, not a general multithreaded tracing API; PID/TID metadata is captured at initialization. Main-thread ownership is preserved by call sites. The sidecar alone does not carry a globally unique boot/build identity; parent collection must preserve boot ID/source/build/executable/maps and perf clock selection.

### What should be done in the future
Parent reviews/cherry-picks the coherent commit, builds ARM with debug information/frame pointers if useful, and runs proper short representative off/on hardware and perf experiments. Validate audio/GL health and quantify observer effect there. Do not infer blocked-versus-runnable state or GPU wait from wall-minus-CPU, and do not introduce source optimizations until the measurements justify them.

### Code review instructions
Start `src/paired_clocks.hpp/.cpp`, then `Profiler::Scope` in `src/profiler.cpp`, and the small main-loop/CLI diff. Read `scripts/paired-clocks/00-usage.md` for every emitted stage and intentionally unmeasured gap. Inspect C++ deterministic fixtures plus Python rejection tests; rerun the three host commands above only as the review gate, not a mode matrix. Parent-owned diary01/design01/tasks/scripts04/target evidence remain unchanged.

### Technical details
Worktree: `/home/manuel/code/wesen/worktrees/2026-10-12--falllinie-spike/pi-worktree-df25b05d-0fa1-4d07-ab90-c911e9566ea8-s0-0`. Branch: `pi-subagents/paired-clocks-df25b05-a876-s0-t0`. Base: `c9c65f9`.

Invocation: `falllinie-spike --autoplay --dual-screen --seconds 20 --profile /tmp/live.csv --paired-clocks --profile-cadence 60 --gpu-profile-mode off`. Sidecar is JSONL schema 1, identity `(pid,tid,run_start_ns)`, per sample `(sequence,frame,view,pass,stage,wall_start_ns,wall_end_ns,cpu_start_ns,cpu_end_ns,wall_status,cpu_status)`. Meta declares cadence/capacity/clocks; final declares attempted/written/dropped/invalid_wall/invalid_cpu/active. Disk size grows with run duration; use a finite duration and unique prefix. Fixed capacity is 64 in-memory completed records.

Existing `scripts/build-device.sh` uses `src/*.cpp` so it includes the new recorder automatically. External explicit lists must add `src/paired_clocks.cpp`. For AArch64 perf, keep `-O2`, add `-g -fno-omit-frame-pointer` and optionally disable sibling-call optimization, preserve the exact unstripped ELF/build ID and existing target libraries. No new tracing dependency is needed. The helper itself performs SSH/container work and was not run by this worker. No native/vault push occurred.
