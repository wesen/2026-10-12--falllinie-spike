---
Title: Tracing architecture evidence and bounded experiment plan
Ticket: FALLLINIE-TRACE-005
Status: active
Topics:
    - performance
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://src/audio_timing.hpp
      Note: Existing separate audio thread CPU precedent
    - Path: repo://src/game_renderer.cpp
      Note: Player and streaming uploads outside renderer pass scopes
    - Path: repo://src/main.cpp
      Note: Main-thread GL context and serial frame scopes
    - Path: repo://src/profiler.cpp
      Note: Existing wall and GPU query semantics, not thread CPU
    - Path: repo://src/renderer.cpp
      Note: Scene post output and nested dynamic scope boundaries
    - Path: repo://ttmp/2026/10/13/FALLLINIE-TRACE-005--cpu-gpu-driver-and-scheduler-attribution-with-perf-ftrace-and-ebpf/scripts/evidence-provenance.json
      Note: Read-time bounded source and parent target evidence hashes
ExternalSources: []
Summary: Source-grounded CPU wall, thread execution, scheduler and GPU attribution model with explicitly unperformed bounded target experiments.
LastUpdated: 2026-10-13T21:25:00-04:00
WhatFor: Prevent ambiguous renderer wall measurements from becoming unsupported GPU or threading conclusions.
WhenToUse: Before authorizing target profiling or selecting CPU, driver, presentation or GPU optimizations.
---

# Implementation authorization update

The user subsequently authorized implementation: “Ok, implement. commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)”. The historical publication-only scope below describes the earlier checkpoint; it no longer prohibits scoped instrumentation and bounded target experiments under the same safety constraints. Parent owns device execution, independent review and acceptance. No system installs, policy weakening, kernel changes or takeover of existing tracing collectors is authorized implicitly.

Initial read-only inventory (scripts/evidence/capabilities/initial.log): perf7.2.7 exists; CONFIG_PERF_EVENTS=y and BPF/BPF_SYSCALL/BPF_JIT=y; CONFIG_FTRACE and CONFIG_KPROBES explicitly disabled; /sys/kernel/btf/vmlinux absent; no trace-cmd,bpftrace,bpftool in PATH. No readable trace root was found. These observations constrain the plan to thread clocks and supported perf events initially, not an invented ftrace/kprobe execution success. BPF configuration alone does not prove required hook/helper availability.

# Purpose and authority (historical publication checkpoint)

This ticket fulfills the user request for a new docmgr ticket and a detailed textbook-style Obsidian project report, without analogies, followed by scoped vault commit/push. It is a documentation/research/design publication. It does not authorize device commands, kernel tracing, source instrumentation, profiling policy changes, tool installation, or rendering workers. PERF-004 remains a separate measurement/report task with its original worker; its files are read-only inputs here.

Publication: `/home/manuel/code/wesen/go-go-golems/go-go-parc/Projects/2026/10/13/PROJECT REPORT - Falllinie Tracing - CPU Execution, Driver Waits and GPU Attribution.md`, with five original self-contained SVGs and provenance under `_assets/falllinie-trace-005/`. The article is the full pedagogical treatment. This design owns the native evidence/architecture and future experiment contract. Publication status belongs to the diary, not an invented target acceptance receipt.

# Terms and inference boundaries

Wall time is elapsed clock time between scope boundaries. Thread CPU time is execution charged to that thread, including its charged user/kernel execution. On-CPU means scheduled execution; off-CPU includes both blocked (not eligible until a condition changes) and runnable (eligible but not selected) states. Scheduler events reconstruct those state transitions. Process CPU time aggregates threads, so it cannot substitute for render-thread CPU time when audio producer/callback work runs concurrently.

The user-space graphics driver translates API operations and manages resources. A kernel ioctl invokes device-specific operations; a fence signals asynchronous completion. Neither the duration of an ioctl nor a blocked stack alone identifies a GPU-only wait. GPU elapsed queries have completion semantics distinct from busy counters; utilization requires an actual busy-fraction definition and observation window. No target busy counter has been established.

For identical render boundaries, `wall - thread_cpu ≈ blocked + runnable + residual`. The residual includes scope mismatch, clock resolution/overhead and accounting conventions. Interrupt/softirq accounting means charged CPU time and scheduler residency need not coincide exactly. High thread CPU may be useful work, driver execution or spinning. Low thread CPU does not automatically imply GPU wait.

# Current source evidence

Read `src/profiler.cpp/.hpp`, `main.cpp`, `renderer.cpp`, `game_renderer.cpp`, and `audio_timing.hpp` in full. Current runtime is `532081682d36f1eaf5eb0daf9a45b17f4b60f136`; `git diff 5320816 7ad1c7d -- src` is empty. Base `7ad1c7d6595afde830434d597d337605c6652e5b` adds this ticket, not runtime tracing.

- `Profiler::Scope` records `std::chrono::steady_clock` duration into `cpu_ms`. This is CPU-observed wall time, **not thread CPU execution**. Query begin precedes the inner clock start, query end follows its stop; parent scopes include child instrumentation overhead.
- Scene contains dynamic prep/upload CPU-only children. Post/output are separate passes; output includes drawable copy and conditional HUD/overlay. Whole-view and per-pass elapsed modes are alternatives, not nested elapsed queries. Query slots and CPU records are bounded at 32; original frame/view/pass survives delayed query availability.
- Polling checks availability, handles disjoint conservatively and never waits for completion. Schema2 emitted CPU counts, parent closure and GPU counterparts validate record integrity; they do not prove causal GPU pass attribution or query non-perturbation.
- Main owns the GL context across both windows. Bottom submit includes context/head-camera setup excluded by inner renderer view; next frame top context restoration occurs early. Different sampling populations prevent precise overhead isolation by subtracting unrelated means.
- Player scope includes streamed-cell and rider uploads outside renderer scopes; stream is nested in player. One authoritative simulation is retained. Audio thread CPU timing already exists, but no main-render-thread CPU clock or kernel marker is added here.

# Bounded existing measurements

Historical AUDIO003 dual384×288 buffered120s: mean interval19.7839ms, top wall9.5481, bottom8.1037, physics0.1557, player0.6613 including stream0.0866, swaps0.8962, pace0.0058. GPU CSV has no observations, not zero work. The separate PERF004 designs/diaries explain provenance and current scope.

Parent-collected new PERF004 target files were untracked at read time and are preserved without staging or modification. Read selected logs across eight20s cases and metadata/receipt/analysis/raw inputs for live07/08. The manifest `scripts/evidence-provenance.json` hashes 24 relevant source/evidence files, stores the bounded live08 excerpt, labels uncommitted status, and records official documentation retrieval hashes. Recomputed live07/live08 analysis matches stored JSON after removing only the location string; no input was rewritten.

Live08: 1034 postwarmup intervals, mean18.242673ms. Top wall scene2.563042/post0.667464/output4.887863ms; GPU elapsed10.159333/7.067552/5.111182ms,18 samples each. Lower GPU8.509051/7.457996/3.412806ms. The approximately41.718ms six-mean sum exceeds the all-frame mean; it is **not an additive budget or proof of shader dominance**. Sparse sampled population differs from all frames; query perturbation, tiled semantics and driver/scheduling behavior remain questions. Khronos explicitly warns of tiled timing interpretation, but its implementation notes do not establish this exact driver mechanism.

Both live logs report zero starved callbacks and callback over-budget, each with one producer-over-budget block, and GL_error0. These are short-run diagnostics, not long-run audio qualification or GPU utilization. Static fixtures omit gameplay/dynamics/audio and duplicate camera. First20s live trajectory differs from historical120s. Query-active image equivalence remains untested: existing capture compares post-run uninstrumented redraws only.

# Proposed correlation architecture

The first implementation proposal is paired wall/thread CPU clocks at coarse existing main-thread scopes, opt-in and sparsely sampled with explicit validity. Use new unambiguous fields/schema rather than changing `cpu_ms` silently. Preserve unavailable/error counts, fixed storage, no per-scope serialization, and no audio callback instrumentation beyond its existing numeric clocks/counters. No implementation is claimed.

A proposed identity tuple is `(run, source/build, process lifetime, thread lifetime, frame, view, pass, event kind)`. Markers or stable-boundary uprobes identify CPU scopes. Scheduler records add CPU/state/waker and event-loss data. Driver records can add context/submission/fence identities only when verified. Query results retain original scope IDs but elapsed-only output supplies no CPU-aligned GPU timestamp pair.

Measure CPU-clock alignment before and after collection. SDL counters require their measured frequency; steady_clock monotonicity does not guarantee an origin shared with tracefs/perf. Choose a cross-CPU comparable trace clock; document marker syscall timing uncertainty and CPU migration. GPU clock correlation is a separate problem. Do not insert glFinish or blocking result reads to force alignment.

A constructed teaching example follows frame420/view0/output through10ms wall and3ms CPU: running0–1, blocked1–5, runnable5–6.5, running6.5–8.5, runnable8.5–10ms. Runnable switch-out needs no wakeup, so wakeup-to-run alone misses1.5ms. An ioctl0.8–7.2 includes0.9ms CPU,4ms blocked and1.5ms runnable. A later8ms GPU query does not prove the blocked interval belongs to that pass or frame. This is synthetic, never target evidence.

# Instrument selection and constraints

**Thread clocks first.** Determine whether wall duration contains substantial execution or off-CPU time; measure empty-scope overhead and matched off/on distributions.

**perf next.** Counts/sample profiles locate executing application, driver or kernel code. On-CPU samples do not measure off-CPU duration. Frame-pointer/DWARF unwinding, stripped/proprietary libraries, architecture-specific features, build IDs, mappings and ASLR are explicit validity limits. Preserve mapping/object identity even when only module-level attribution is possible. Collect losses/throttling and hardware multiplex time-enabled/time-running metadata. User-only sampling intentionally excludes kernel execution.

**Scheduler events next.** Decode actual sched_switch/wakeup formats and state encodings. Reconstruct running/blocked/runnable/unknown states across CPUs, including runnable switch-out, successful wakeup, duplicate wakeups, initial partial intervals, task exit and TID reuse. Wakeup filters must match target fields, not merely current/waker PID; switch filters must preserve both relevant sides. Waker identity is context, not proof of dependency. Lost records invalidate local reconstruction rather than becoming zero waits.

**Driver/BPF only for remaining questions.** Prefer available tracepoints before unstable kprobe/uprobes. Verify nested-function entry/exit pairing, recursion/depth, process lifetime and descriptor/object reuse. Do not invent Mali tracepoint names. BPF verifier acceptance does not imply causal correctness or acceptable overhead. BTF/CO-RE helps type relocation, not hook/helper availability. Check kernel configuration, architecture, capabilities, perf/BPF policies, lockdown, licensing/helper/program-type restrictions and tools. A7.2.7 release string alone proves none of these. No CPU tracing method measures shader instruction cost; hardware counters/vendor tools are a separate gate.

# Finite future experiments and acceptance

1. **Read-only capability inventory, separately authorized:** kernel/driver/build, installed perf/version/help, existing kernel config, readable perf/BPF policy, tracefs existence/permissions/ownership, available events/formats/clocks, BTF and unwind/object metadata. Missing/denied access is a result; no automatic mount/install/policy relaxation. Feature tools that load BPF are not assumed read-only.
2. **Paired scope clocks:** reviewed source patch, empty-scope cost, short matched off/on runs, explicit validity/drop counts, no change to production defaults. Require common boundaries and no audio regression before interpreting ratios.
3. **Five-second low-rate perf experiment:** identified main TID, proposed99Hz user samples, bounded pages/stack depth, explicit output directory and graceful end. Confirm supported syntax/event/unwinder first. Kernel-inclusive tracing requires separate scope approval.
4. **Five-second scheduler trace:** coarse frame/pass markers, dedicated instance if supported/approved, bounded total memory across CPUs, correct filters, measured clocks and lifecycle completeness. Unknown/lost intervals remain unknown. No per-draw storms or callback marker writes.
5. **One bounded custom correlation:** only if a named unresolved driver wait remains; verified event semantics and identifiers, max task/map cardinality/nesting/stack/output limits, state cleanup/expiry, ring-buffer failure/discard counters, safe detach.
6. **Optimization validation:** normal instrumentation-off repetitions with representative live audio/graphics, fidelity checks and measured improvement. CPU batching/jobs, GPU shader/bandwidth changes and buffer/presentation changes require different proof and authority.

For each comparison use baseline/instrumented/instrumented/baseline at minimum, with documented workload phase/order, audio/display state, resolution, query mode/cadence, frequency/thermal readings and whether these are snapshots. Short traces may miss tails and must not be generalized silently. Exact repetition counts and perturbation thresholds are to be agreed before the future run; this document does not approve target operation.

Acceptance requires provenance, clock definitions, bounded collection, reviewed filters, loss/unknown accounting, sufficient samples, quantified observer effect and no audio/fidelity regressions. A tool exiting successfully is insufficient. Stop if safe isolation or ownership cannot be established; stop if instrumentation perturbs behavior materially or data loss defeats attribution.

# Safety, rollback, privacy and architectural invariants

Never take over global tracing_on, clear another trace buffer, weaken perf/BPF policy, install tools, mount tracefs, unload modules or kill services implicitly. Dedicated instances isolate some controls, not all dynamic probe/global resources. Snapshot exact owned settings and restore only what this experiment changed. Close owned descriptors/attachments and verify normal behavior on completion, timeout or failure. External timeout syntax must be verified for the device before use.

Trace artifacts can expose other tasks, addresses and stack contents. Minimize filters without losing external wakers, avoid arbitrary memory/ioctl payload capture, restrict raw evidence access, and publish bounded redacted aggregates. Hashes identify data but do not remove confidentiality risk.

Maintain main-thread GL/SDL video, no JavaScript or marker allocation in audio callback, one authoritative world and deterministic ownership of RNG. Existing mutable Terrain/World caches are not worker-safe; any future CPU jobs require immutable or worker-owned inputs, generation-tagged bounded completions and main-thread uploads. Threads do not inherently fix GPU dependency stalls.

# Research and reproducibility

`scripts/01-research.py` downloads at most2MB per official source with20s timeout, raw bodies under ignored `.work/sources`, URL/date/hash/error metadata tracked. It retrieved kernel ftrace/events/perf-security/verifier/libbpf/licensing, Linux perf-record source documentation and official Khronos query text. An assumed perf `.adoc` URL returned404; correct `.txt` retrieved. Khronos registry returned403; official repository copy retrieved. Both failures remain in metadata.

`scripts/02-assets.py` generates five original SVGs and bounded evidence provenance. `scripts/03-validate.py` checks report frontmatter/fences/links/word count, SVG XML/self-containment and numerical excerpt/hash integrity without invoking any target tools. All helper scripts stay in this ticket; generated temporary outputs stay ignored `.work`. Publication requires parent editorial approval before scoped vault commit/push. Native commits are limited to TRACE005; no native push.
