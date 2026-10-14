---
Title: Tracing investigation and publication diary
Ticket: FALLLINIE-TRACE-005
Status: active
Topics: [performance]
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Actual documentation request, source research, failures, validation and scoped publication evidence; target tracing remains unperformed.
LastUpdated: 2026-10-13T21:25:00-04:00
WhatFor: Preserve investigation reasoning and verifiable publication history without inventing a tracing experiment.
WhenToUse: When reviewing the article or resuming the future capability and attribution gate.
---

## Step 1: Establish evidence semantics before drafting the tracing report

The parent created TRACE005 at `7ad1c7d6595afde830434d597d337605c6652e5b` and delegated documentation/publication only. I read the full vault-writing and textbook-authoring skills, docmgr and diary skills plus investigation format, default vault exemplars, native AGENTS, source profiler/main/renderer/game_renderer/audio timing, and PERF004 designs/diaries/scripts README/analyzer. The vault began clean on main at `3d20222c1340f28f48c32907ce77a0fb2da29b0d`. Native had only the explicitly identified parent-owned untracked PERF004 target runner/evidence, which I preserved.

The report follows one synthetic output scope through wall duration, thread CPU, blocked/runnable scheduler intervals, an ioctl and a delayed GPU query. It separates that teaching example from actual parent-collected Mali measurements and from unimplemented main-thread instrumentation. The current `cpu_ms` scope field is wall duration; no perf, ftrace, eBPF, device capability inspection or GL thread change was performed.

### Prompt Context
**User prompt (verbatim):** 'Ok create a new docmgr ticket and  write a detailed project report for the obsidian vault as a deep dive technical analysis blog post using a textbook writing style (no analogies, see skill). Commit and push the bsidian vault when done (go-go-parc vault).'
**Assistant interpretation:** Complete the new ticket's research/design and publish a detailed source-grounded textbook article with original figures and explicit future experiment gates.
**Inferred user intent:** Preserve a technically useful answer to whether Linux tracing can distinguish render CPU work, driver blocking, scheduler delay and GPU time without conflating measurements or guessing an optimization.
**Commit (code):** No code change. Runtime source remains `532081682d36f1eaf5eb0daf9a45b17f4b60f136`; diff to ticket base has no src changes.

### What I did
- Read all named source/doc seams and selected logs/receipt/analysis from eight parent-collected20s PERF004 cases. Recomputed live07/live08 analysis through the existing analyzer in memory and compared stored objects after removing only the path string; both matched. No PERF004 file was edited.
- Wrote design01, brief index orientation and a new append-only vault note. Defined wall/thread CPU/on-off CPU/runnable/blocked/driver/ioctl/fence/queue/GPU elapsed first. Added serial main-loop Mermaid, equations, pseudocode, decision table, bounded command examples and five original SVGs.
- Added ticket-local research/asset/validation helpers. Raw downloaded sources and rendered PNGs go only under ignored scripts/.work; bounded provenance tracks24 source/evidence hashes and8 successful official-source snapshots plus two failed endpoint attempts.
- Read the entire article independently, verified synthetic versus actual/proposed wording, rendered all five SVGs with installed rsvg-convert and inspected them. Fixed directional arrowheads in the scheduler-state figure before the editorial gate.

### Why
A long GL API wall duration may contain CPU execution, blocking and runnable delay. CPU sample stacks cannot account for off-CPU duration; GPU elapsed queries cannot supply device utilization or CPU-aligned execution boundaries. The measured six-pass mean sum exceeding the all-frame interval is a reason to investigate sample populations and tiled/query/driver behavior, not a reason to label a shader bottleneck.

A report can be complete as research/design while target capabilities remain unknown. This avoids turning the user's publication request into permission for kernel instrumentation, tool installation or a source rewrite. PERF004's final measured report remains separate and is not published by this task.

### What worked
- Local-source inspection confirmed actual scope nesting, query lifecycle, shared-context ownership and existing audio thread-clock precedent. `git diff 5320816 HEAD -- src` returned zero lines at the base.
- Official kernel docs fetched successfully for ftrace, events, perf security, verifier, libbpf and licensing. Linux perf-record.txt and Khronos's official GitHub specification copy supplied the missing references.
- All selected target numeric excerpts were tied to read-time SHA256 hashes and explicit uncommitted status. Live08 has18 samples/pass and1034 frame intervals; no additive GPU cost or controlled historical improvement is claimed.
- Offline report validation passed after correcting the validator's decimal rounding, with5888 body words,6 local links,5 self-contained SVGs,24 matching input hashes, valid frontmatter and balanced fences. No analogies or unfinished placeholders found in full prose review.

### What didn't work
- Combined source `cat` exceeded the tool's50KB display limit; reread profiler/header/main portions separately so the complete named source was actually inspected. This was output truncation, not a source failure.
- `cat "$P/source-revision.txt"` reported `No such file or directory`; the actual per-case `metadata.txt` contains `source_rev=532081682d36f1eaf5eb0daf9a45b17f4b60f136`. Used that actual field rather than inventing a receipt filename.
- `python3 "$T/scripts/01-research.py"` recorded `HTTP Error 404: Not Found` for the guessed upstream perf-record.adoc path and `HTTP Error 403: Forbidden` for the Khronos registry URL. Corrected to official perf-record.txt and Khronos repository copy, preserving failed attempts in research-provenance.json. No installation or alternate agent/tool fallback occurred.
- Initial `python3 "$T/scripts/03-validate.py" "$NOTE"` failed on `AssertionError: {'n': 18, 'mean_ms': 5.1111815, ...}` because binary floating formatting gave5.111181 while publication used decimal half-up5.111182. Fixed the validation to `Decimal(str(value)).quantize(..., ROUND_HALF_UP)`; did not change the underlying evidence. Rerun passed.

- `docmgr doctor --ticket FALLLINIE-TRACE-005 --stale-after 30` initially reported `[ERROR] invalid_frontmatter` for the new scripts/README.md: `frontmatter delimiters '---' not found`. Added reference frontmatter to this ticket-local README and reran the doctor rather than modifying repository configuration or unrelated docs. The rerun had only `missing_numeric_prefix`; parent editorial review requested eliminating this warning. Renamed it to `scripts/00-publication-helpers.md` and updated links before final validation.

### What I learned
Schema-valid GPU records do not prove additive service time. Khronos's tiler caveat is more precise than claiming all asynchronous GPU durations contain queue wait: implementations may accumulate workload time, differ on tile-memory transfer inclusion, and must not be assigned a guessed overlap mechanism. The report states those limits rather than explaining the measured sum with an unsupported causal story.

### What was tricky to build
Scheduler reconstruction cannot pair every off-CPU interval with a wakeup: a runnable switch-out needs no wakeup. External wakers also mean filtering on current PID alone loses relevant records. The synthetic example retains both distinctions and shows an ioctl containing execution, blocked and runnable portions. Task lifetime/TID reuse, migration, clock alignment, dropped records and nested driver functions remain explicit future implementation requirements.

### What warrants a second pair of eyes
Parent editorial approval must inspect claims about thread CPU accounting, sample populations, ioctl attribution, tiled query semantics, tracefs ownership and security. Verify no statement claims a real BPF experiment, new threading, main-thread CPU instrumentation, GPU utilization or query-active image equivalence. Review the appendix's moving-version sources as references, not exact target feature claims.

### What should be done in the future
Obtain parent editorial approval, then scope-stage only the new vault note/assets, commit/push main without force and verify exact remote hash. Mark documentation/publication tasks only when their gates pass; leave gcop open. Any device capability check or bounded trace requires a separate authorized session.

### Code review instructions
Start design01 and the complete vault article, then scripts/03-validate.py and both provenance JSON files. Reproduce offline checks with the commands in scripts/00-publication-helpers.md. Inspect git status in both repos before staging. Parent's untracked PERF004 artifacts must remain unstaged; native push is forbidden.

### Technical details
Local publication date is2026-10-13 (`date -I`); official source retrievals occurred2026-10-14 UTC. Vault path is `Projects/2026/10/13/PROJECT REPORT - Falllinie Tracing - CPU Execution, Driver Waits and GPU Attribution.md`. Assets are the unique `_assets/falllinie-trace-005/` subtree. Proposed trace hierarchy: paired wall/thread CPU → bounded perf counts/samples → scheduler events → targeted driver/BPF only for an unresolved dependency. Read-only inventory, ownership, bounded resources, loss accounting, privacy and exact rollback precede any attachment.

## Step 2: Pass editorial review and publish the exact scoped vault change

Parent read all12 report sections and approved the editorial/technical publication, specifically confirming the definitions, synthetic arithmetic, scheduler caveats, nonadditive GPU excerpt and actual/proposed boundaries. Parent requested removal of the numeric-prefix doctor warning; I renamed the operational guide to `scripts/00-publication-helpers.md`, retained proper frontmatter and updated links. Final doctor and offline publication validation passed before staging.

The vault commit contains exactly the new note, five SVGs and one provenance JSON. `git push origin main` completed without force; a separate `git ls-remote origin refs/heads/main` returned the exact local commit. Historical JS/audio/performance reports and all other vault files are unchanged. Publication taskymp7 was checked only after this verification; capability taskgcop remains open.

### Prompt Context
**User prompt (verbatim):** See Step1.
**Assistant interpretation:** Complete the authorized publication after the mandatory parent review, with scoped commits and independent remote verification.
**Inferred user intent:** A durable, pushed vault article rather than an uncommitted draft or an unperformed tracing claim.
**Commit (publication):** `e57938fa89a7f7210dab9bc1b684959c0381f7f8` — `Publish Falllinie CPU driver scheduler and GPU tracing deep dive`.

### What I did
- Revalidated report:5888 body words,6 relative links,5 XML-valid self-contained SVGs,24 current source/evidence hashes and exact rounded excerpt; report SHA256 `bda2f359e2abcd56acad5e7653e261cd1b6d8bf2bd1fb90355fb91a62faa0614`.
- Ran Python compilation on the three helper scripts, docmgr doctor (all checks passed), git diff/check, and strict porcelain scope assertions that the vault remained main at3d20222 with an empty index and only this note/assets untracked.
- Scoped `git add` to the exact note and its unique asset subtree. Staged diff-check passed;7 files/828 inserted lines were committed. Push moved origin/main from3d20222 to e57938f.
- Independently compared local HEAD against `git ls-remote origin refs/heads/main`: both `e57938fa89a7f7210dab9bc1b684959c0381f7f8`. Vault status and staged-file lists were empty afterward.
- Recorded exact publication receipt and validation output under this ticket's scripts, checked ymp7 and updated this ticket's changelog. No native push or target operation occurred.

### Why
An acknowledged push alone is weaker than comparing the advertised remote branch to the exact local commit. Task completion follows actual publication evidence. The warning correction keeps this new ticket's docmgr validation clean without changing repository-wide conventions or historical docs.

### What worked
All final gates passed. Parent editorial approval was obtained through the live coordination tool before publication. Scoped vault changes and remote identity are independently inspectable through the commit and `scripts/publication-receipt.json`.

### What didn't work
No commit, push, branch, remote or staging conflict occurred. Earlier research/validation failures remain recorded in Step1. No target tracing test was attempted or reported as passing.

### What I learned
Source/evidence hashes preserve the bounded excerpt's identity even though parent target files were uncommitted at read time. They do not convert those files into an immutable published measurement campaign, so the article retains that qualification.

### What was tricky to build
The native repository contains parent-owned untracked PERF004 runner/target artifacts. The final native commit must use only the exact TRACE005 path, never a repository-wide add, and must leave those artifacts untouched. The resulting native commit hash is recorded in the durable task handoff rather than recursively embedded in its own contents.

### What warrants a second pair of eyes
Independent acceptance can compare vault commit file inventory, remote main and report hash with the receipt, then confirm native changed paths are TRACE005-only. Future target feature availability and causal GPU/driver attribution remain unverified by design.

### What should be done in the future
Taskgcop: separately authorize read-only target capability inventory, then the minimum bounded paired-clock/perf/scheduler experiment warranted by those capabilities. Do not select a production optimization solely from the excerpt or add CPU workers as an assumed GPU-stall fix.

### Code review instructions
Read `scripts/publication-receipt.json`, `scripts/validation-results.json`, tasks.md and the vault commit. Repeat `python3 "$T/scripts/03-validate.py" "$NOTE"` while inputs remain unchanged; `docmgr doctor --ticket FALLLINIE-TRACE-005 --stale-after 30`; `git diff --check`; and `git ls-remote origin refs/heads/main` in the vault. Native git status is intentionally not fully clean because preserved parent PERF004 files are untracked; both repositories must have no staged files after commit.

### Technical details
Vault origin is `ssh://git@github.com/go-go-golems/go-go-parc`, branch main. Publication is append-only and owns `_assets/falllinie-trace-005/`. No source changes, no shaders, no new C++ tests, no device commands, no perf/ftrace/BPF attachment, no installed tools, and no settings changes are part of this ticket's publication checkpoint.

## Step 3: Authorize staged implementation and inspect actual tracing facilities

The user authorized implementation of the report's plan with meaningful commits and a detailed diary. We begin with optional paired render-thread clocks and capability-driven CPU profiling, not an unconditional custom BPF implementation. Parent ran a read-only inventory before choosing a kernel instrumentation path.

The installed kernel has perf but explicitly disables FTRACE and KPROBES. This rules out assuming the proposed tracefs scheduler/kprobe method is available. Kernel BPF options are enabled, but the absent BTF/tooling and unverified event hooks mean that BPF support alone proves no usable attribution path. Paired clocks and supported perf sampling remain useful without changing the kernel.

### Prompt Context
**User request (summary from compacted context):** Implement the previously discussed plan, commit at appropriate intervals, and maintain a detailed work diary using the skill format. Original authored bytes are unavailable in this context; the earlier reconstructed quotation was incorrect evidence labeling.
**Assistant interpretation:** Implement safe optional scope timing, choose tracing facilities based on actual support, run bounded reviewed target experiments and commit at meaningful milestones.
**Inferred user intent:** Determine whether drawing wall time is computation or waiting using reproducible evidence rather than a new theoretical plan.

### What I did
Read design01 and current profiler contracts; loaded full investigation diary format. Added ticket-local04-capabilities.sh, executed it over existing SSH via stdin (no remote file installation), and saved scripts/evidence/capabilities/initial.log. Added implementation tasks d8cq and90uh. Updated design authority prospectively without rewriting the published historical report.

### Why
Tools and kernel hooks are independent capabilities. A running recent kernel and enabled BPF_SYSCALL do not imply scheduler trace events or attachable driver functions. Avoiding unavailable mechanisms focuses effort on measurements that can actually run.

### What worked
Inventory returned perf7.2.7 at/usr/bin/perf,CONFIG_PERF_EVENTS=y,BPF/BPF_SYSCALL/BPF_JIT=y,SCHEDSTATS=y. Current target364832KiB available/no swap and no active Falllinie process in the bounded check. No configuration/collector/service was changed. Prior eight-run PERF004 evidence is now preserved unchanged in ae2a175; capability/authority checkpoint c9c65f9. Neither commit claims final GPU attribution.

### What didn't work
`git diff --cached --check` returned exit2 because eight raw stdout.log GL_EXTENSIONS lines end in spaces. Preserved the raw bytes rather than modifying hashed evidence; checked with command-local `git -c core.whitespace=-blank-at-eol diff --cached --check` and committed only intended PERF004 evidence. No repository-wide whitespace configuration was changed.

Not failed experiments: CONFIG_FTRACE and CONFIG_KPROBES are explicitly not set; BTFvmlinux absent; trace-cmd/bpftrace/bpftool unavailable; no readable trace root found. These are capability limits, not permission to install tools, mount tracefs, weaken policy or replace the kernel. perf_event_paranoid=2 does not establish whether this root context can use every event; bounded event tests remain required.

### What I learned
The first attribution layer can use existing thread CPU clocks and perf samples without requiring custom BPF. Scheduler blocked/runnable reconstruction may remain unsupported on this image and must be reported as such.

### What was tricky to build
Historical design says publication-only; the new request supersedes that authority boundary for implementation but retains rollback/privacy/main-threadGL constraints. Preserve existing untracked PERF004 actual target evidence before making an isolated worker branch so its provenance is not lost.

### What warrants a second pair of eyes
Paired boundary ordering, failed CPUclock readings, wall-minusCPU interpretation, disabled overhead, versioned schema, query nesting/context lifetime and trustworthy analyzer reconciliation. No averaging of unrelated sample populations into offCPU estimates.

### What should be done in the future
Single isolated writer implements bounded opt-in clocks and tests; reviewer checks exact source. Parent builds and runs matched hardware gates plus minimal perf counts/sampling supported by the device. No source rendering optimizations until measurements justify them. Record remaining scheduler/driver attribution limits rather than claiming GPU waits.

### Code review instructions
Start at04-capabilities.sh/initial.log, updateddesign01 and src/profiler.cpp/.hpp/audio_timing.hpp. Future worker diary is reference/02-paired-clock-implementation-diary.md; parent diary01/tasks/target evidence remain parent-owned.

### Technical details
Runtime base5320816; parent ticket publication5eb0135 changed docs only. Preserve noGLoffmain/noJScallback/oneworld/RNG invariants. Current fixed sampling cadence60; new paired fields must not redefine legacycpu_ms from wall to threadCPU silently. Raw traces, if any, remain private/ignored; bounded aggregates and object identities are sufficient for publication.

### Subsequent user direction: prioritize real measurements
**User prompt (verbatim):** "also don't overcautiously test and measure, we can run proper runs once all the things are built pretty quickly, no need to test every little change that won't actually bring us closer to proper measurements. this is not production software but debugging / dev tooling"

The implementation packet carries that instruction explicitly: build the whole instrumentation/analyzer slice, then one focused host validation rather than per-edit checks, sanitizer matrices or polish loops. Workflowf6aa12ba-ba2b-4961-a4fc-330ae41468be owns one isolated writer followed by a focused read-only measurement review. Parent prepared05-run-target.sh and06-parent-campaign.sh for one substantive live campaign with baseline/clocks/view/passes/perf counts/perf samples and baseline brackets. These scripts are preparatory and not yet executed; the new runtime flag remains to be confirmed against the worker's implementation.

A workflow-level steering attempt returned `Workflow 'f6aa12ba-ba2b-4961-a4fc-330ae41468be' has no live foreground child.` One status inspection showed the workflow still running/setup with its first child; no fallback process or duplicate worker was launched. Native completion/coordination will drive continuation. No additional device micro-probes were run.

## Step 4: Integrate paired clocks and execute eight substantive live runs

The complete instrumentation slice landed as209a6d0 (worker9097f8d), after one feature-boundary host build/test run and one focused source review. The parent then built one ARM candidate and ran eight30-second live cases: baseline, clocks, whole-view queries, pass queries, perf stat, perf record, clocks, baseline. No small-change hardware matrix was inserted.

The measurements now separate executing CPU time from non-running render-thread time. In the first unqueried clock run the top view averaged7.645ms wall/4.522ms CPU, and the lower view7.031/2.774ms across27postwarmup samples. perf also identifies substantial executing driver code: libmali accounted for28.06% of all sampled CPU periods, or71.44% of the main thread's sampled CPU share. Neither observation proves that all non-running time is GPU waiting.

### Prompt Context
**User request:** Implementation/commits/diary summarized inStep3; exact subsequent direction there prioritizes real runs over excessive preliminary checking.
**User prompt (verbatim):** "how are things going, are we making progress? are we overly cautious maybe?"
**User prompt (verbatim):** "make graphs and such for the report and the diary."
**User prompt (verbatim):** "commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)"
**Assistant interpretation:** Complete real measurements promptly; preserve failures, results and uncertainty; make reproducible visual evidence usable in both report and diary.
**Inferred user intent:** Reach actionable diagnosis instead of spending the session on procedural validation.
**Commit (code):**209a6d0 — Add opt-in paired main-thread clocks and TRACE005 sidecar analysis.

### What I did
- Read writer/reviewer artifacts, inspected profiler/main diff and recorder implementation, cherry-picked9097f8d as209a6d0. Worker15/15tests passed6.42s; reviewer found no concrete P1/P2 issues. No parent repeat of unchanged host suite.
- Built with07-build-arm.sh using existing offline Docker toolchain and approved targetSDL. O2/g/frame-pointers/build-id; binary SHA2562872ede7219ccd91890de4c0d47d5af8fab687f2b5b2cf9605e25ecb1dfcb1d2. Main-thread GL, audio/RNG/shaders unchanged.
- Used05-run-target.sh/06-parent-campaign.sh under separate/storage/apps/falllinie-trace005-20261014T020124Z. Original game and launcher untouched. Every run had internal30s limit and external60s timeout+5s kill grace, bounded perf buffer, no systemwide attachment or policy changes. Preserved boot ID/config/frequency/thermal/binary/argv/exit receipts.
- Ran eight live autoplay dual384×288/no-vsync/fps60 cases with buffered1024/audio muted but full source-vinyl DSP and overlay. 13133totalframes/12653after60warmup per run;3360pairedrecords from sixinstrumented runs, zero invalid/dropped records, exact final reconciliation.
- Recorded perf cpu-clock at99Hz with mono timestamp clock and frame-pointer stacks;3973samples,0reported lost samples. Kept raw recording (perf reported0.467MB) in ignoredscripts/.work; retained text reports/build IDs. Recovered formatter without rerunning the captured workload, then completed cases07/08.
- Prepared09-graphs.py with raw-input hashes, fiveSVGs and exactJSON aggregates; frame/paired plots exclude frame<60 consistently, perf whole-recording aggregates include startup explicitly.

### Why
Paired clocks answer the immediate wall-versus-execution question without unavailable ftrace hooks. perf supplies where CPU execution occurred; it does not explain blocked duration. Baseline brackets and whole-view/pass cases test whether conclusions rely on one query mode, without claiming a randomized fixed-clock experiment.

### What worked
All eight workload receipts completed withGL_error0, callbackover_budget0,starved_callbacks0,missing_frames0. Mean postwarmup intervals18.047–18.408ms; p95~32.8–33.1ms. Paired final records all complete/valid. Raw perf survived report formatting failure and rendered successfully with inline expansion disabled. Source and deployment binary hashes match. Finite audio health is not XRUN/physical latency proof.

### What didn't work
- First SSH launch lacked the credential environment expected by the external askpass helper: `Permission denied, please try again.` then `Received disconnect from 192.168.0.202 port 22:2: Too many authentication failures`; exit255 before target runs. Supplied the existing credential environment for retry; no credentials stored in receipts/scripts.
- After successful sixth workload and perf recording, `perf report` exited141 with `cmd_addr2line_configure /storage/apps/falllinie-trace005-20261014T020124Z/falllinie-trace005: addr2line configuration failed`. Originalperf-report.txt and launch log preserved.08-finish-campaign.sh uses `--no-inline -g none`, retains symbol/DSO sample attribution and avoids broken inline expansion. Final two runs were delayed by postprocessing recovery; these are not uninterrupted brackets.
- Final scoped evidence staging found `scripts/evidence/build/compiler.txt:5: new blank line at EOF.` from raw compiler output. Preserved raw receipts and used command-local whitespace exceptions for raw evidence only; ordinary source/docs diff-check passed. Git also warned the SSH failure log CRLF would normalize toLF whenGit next touches it; the text diagnostic is preserved, not treated as a byte-identical capture hash. No repository whitespace setting changed.
- Four startup GPU readings across view/pass cases were about1.84467441e13ms despite API statusvalid. These are physically impossible for a30srun. Raw readings remain unchanged; common frame<60warmup exclusion removes them from postwarmup plots. All postwarmup query values are numerically smaller, but this does not validate their causal meaning.
- Whole-view mean sum27.272ms and pass mean sum42.174ms remain inconsistent as additive costs against~18msframe intervals. Do not call them utilization or use subtraction to price a shader. Numeric API validity is not sufficient physical validity.

### What I learned
There are two distinct targets: executing driver overhead (main-thread libmali samples) and non-running time (paired residuals, concentrated in output). Source CPU optimization alone cannot be assumed to recover all rendering wall time. New data does not show a large systematic slowdown on cadence-selected query frames; the observer-effect hypothesis remains unproven rather than being used as a convenient explanation for nonadditive GPU results.

### What was tricky to build
A single render scope contains nested scene/post/output scopes; counts and bars must not add parent+children. The sidecar uses explicit same-row clock pairs, and the plot generator applies the same warmup boundary to both clocks. perf share is a different denominator (all sampled process threads, including startup), so71.44% is main-thread executing samples, not frame time orGPU utilization. Debug-symbol inline expansion failed even though collection and application succeeded.

### What warrants a second pair of eyes
GPU timer interpretation and impossible startup readings; missing blocked-versus-runnable attribution; all-thread perf percentage denominator; sequential runs with variable clocks and slightly different autoplay trajectories. Frame-pointer stacks may be incomplete in third-party driver libraries; reports currently use flat attribution, not asserted complete call chains.

### What should be done in the future
Use the measured driver-heavy execution and output-heavy residual to choose batching/state-cache/output-path experiments. If exact wait reasons become necessary, obtain supported scheduler/driver tracing capabilities through a separately approved image/tooling change—not an unrequested kernel replacement. Do not broaden this into production observability work. DeferredPERF004report should consume these finalized evidence hashes rather than the initial pass totals alone.

### Code review instructions
Readsrc/paired_clocks.cpp, main.cpp instrumentation, scripts/paired-clocks/analyze.py andscripts09-graphs.py. Reproduce plots with `python3 "$T/scripts/09-graphs.py"`; raw data is inscripts/evidence/target-20261014T020124Z, build receipts inevidence/build. Reviewer artifact is retained asplain.txt, not Markdown without docmgr metadata. Historical vault tracing report remains the research/design checkpoint, not silently rewritten.

### Technical details
GPUqueriesoff in baseline/clocks/perf runs; cadence60 in all cases. Source/runtime209a6d0; all eight binaries identical. Initial/closing clock runs each have27postwarmup samples per named scope. Mean residuals preserve clock-read/accounting skew and offCPU together. Device governorondemand, no pinning/FIFO/affinity/kernel/sysctl/mount/service changes. Separate app process owns perf capture; other existing workloads are not attached.

### Visual evidence
![Same-row thread CPU and non-running estimates](../scripts/figures/01-cpu-wall.svg)
![Live frame interval distributions](../scripts/figures/02-frame-cdf.svg)
![Cadence-selected versus other frames](../scripts/figures/03-query-observer.svg)
![Sampled CPU by thread and object](../scripts/figures/04-perf-cpu.svg)
![Nonadditive GPU elapsed values](../scripts/figures/05-gpu-elapsed.svg)

The five SVGs parsed as valid XML; all ticket checks passed `docmgr doctor --ticket FALLLINIE-TRACE-005 --stale-after 30`. Updated task completion to name exactly the supported CPU/perf work, keeping unavailable blocked/runnable/driver wait attribution and unresolved GPU semantics as explicit open items. Build evidence is forced-added as text because the repository's generic build-directory ignore also matches scripts/evidence/build; no binary/rawperf is staged.

These figures are generated from actual retained data, not constructed examples. Exact values/filtering/inputSHA256s live beside them infigures/provenance.json andevidence/campaign-summary.json.
