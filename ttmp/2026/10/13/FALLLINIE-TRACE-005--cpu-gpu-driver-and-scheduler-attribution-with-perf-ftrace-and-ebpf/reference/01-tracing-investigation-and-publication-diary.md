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
Vault origin is `ssh://git@github.com/go-go-golems/go-go-parc`, branch main. Publication is append-only and owns `_assets/falllinie-trace-005/`. No source changes, no shaders, no new C++ tests, no device commands, no perf/ftrace/BPF attachment, no installed tools, and no settings changes are part of this ticket's completion.
