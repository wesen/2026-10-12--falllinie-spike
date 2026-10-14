---
Title: Measured rendering report publication diary
Ticket: FALLLINIE-PERF-004
Status: active
Topics: [performance]
DocType: reference
Intent: long-term
Summary: Same-worker textbook report combining PERF004 measurement integrity with actual TRACE005 CPU/perf results, editorial approval and scoped vault publication.
---

## Step 1: Resume the retained implementation context and author from final target evidence

Resumed the same worker that implemented PERF004 and corrected the two evidence-integrity findings. The retained native worktree remains based on `09430e4`, but the report explicitly uses the parent evidence checkout at `f400ffa7c658086cf775fd6d27c28403ba09c2bb` and measured runtime `209a6d092885ff3f0a1efd1efab4252cb4ad2b56`. This distinction prevents reporting the old isolated worker source as the deployed paired-clock implementation.

Wrote the requested new textbook-style project report before any optional research. The report develops wall time, thread CPU time, residual interpretation, GPU query validity and CPU sampling denominators before presenting actual measurements. It preserves the original PERF004 target results and reviewer corrections, then follows the TRACE005 live campaign into source-driven next experiments. No source optimization, new device run or web search was performed.

### Prompt Context
**User prompt (verbatim):** "you can use websearch and save userful ldocumentation in the sources/ folder of the ticket, maybe that will help. But first,  write a detailed project report for the obsidian vault as a deep dive technical analysis blog post using a textbook writing style (no analogies, see skill). Commit and push the bsidian vault when done (go-go-parc vault)."
**Assistant interpretation:** Complete the deferred same-worker report using the supplied final evidence, obtain parent editorial approval, and publish only the authorized new vault note/assets.
**Inferred user intent:** Preserve implementation/test reasoning and measured results in a durable technical account without losing context or overstating unresolved GPU attribution.
**Commit (code):** Native publication helper/receipt/diary commit is recorded in the final handoff; no runtime source changes belong to this publication milestone.

### What I did
- Read `obsidian-vault-writing` and `textbook-authoring` skills completely and the full investigation-entry format. Inspected the vault project-note exemplar and related Falllinie note filenames.
- Verified initial vault main at `da2c32a7a2978e26c875c553652378c4cdbee617` with a clean worktree; the unrelated Francois report and all existing history were preserved.
- Read the current PERF004 source cost map and original target records; TRACE005 measured analysis, reference01 Step4, paired-clock usage/analyzer, current main/profiler/paired-clock source, campaign summary, raw selected records, perf text reports and all figure provenance. The main checkout was read-only to this worker.
- Drafted exactly one new note at `Projects/2026/10/13/PROJECT REPORT - Falllinie Rendering Measurements - CPU Execution, Mali Driver Work and GPU Timer Limits.md` and a unique colocated `_assets/falllinie-render-measurements/` subtree. The final note has 5,758 prose words, two Mermaid diagrams and all five actual measured SVGs, plus four self-contained evidence JSON assets.
- Included original PERF004 eight20-second case frame means, TRACE005 eight30-second case distributions, the full-DSP ownership constraints, and the actual frame60/top-output sample:6.105253ms wall,1.080042ms CPU and5.025211ms residual. Explained that the paired sidecar's schema1 is separate from PERF004 stats schema2.
- Wrote `scripts/02-vault-report/publication.py` for one append-only asset preparation and useful boundary validation, not benchmark execution. It checks figure/input hashes, raw frame and paired arithmetic/counts, numeric anchors, links/XML, word count and exact new-file scope. All new useful helpers/receipts remain under this ticket, not main source or vault-global scratch.

### Why
- The earlier tracing article was a design/research checkpoint. This report must teach what was implemented and actually measured, with enough precise definitions for readers to distinguish CPU execution from unresolved non-running causes.
- The final evidence adds strong driver-execution observations but does not resolve the GPU elapsed semantics. Source hypotheses therefore remain conditional rather than being promoted to fictional measured savings.
- Copying existing measured figures and provenance into the note's dated asset directory makes the report readable without cross-repository image dependencies while retaining reproducible source identity.

### What worked
- Source data/provenance checks reconciled13,133 total frames,12,653 postwarmup frames,3,360 valid paired records and the actual same-row sample. GPU sums27.272/42.174ms, four startup anomalies, initial PERF004 means, and perf28.06/39.28 denominator were validated against retained records.
- Pre-publication validation passed with5,664 words for the initial draft, then5,758 after the two parent-approved editorial additions/corrections. All five SVGs matched original SHA256s and parsed as XML; relative assets and related-note links resolved.
- No broad native test matrix or graphics workload was repeated. The publication relies on preserved measurements, not fabricated re-execution claims.

### What didn't work
- One exploratory read used `05-perf-stat/perf-stat.txt`; exact diagnostic: `head: cannot open '/home/manuel/code/wesen/2026-10-12--falllinie-spike/ttmp/2026/10/13/FALLLINIE-TRACE-005--cpu-gpu-driver-and-scheduler-attribution-with-perf-ftrace-and-ebpf/scripts/evidence/target-20261014T020124Z/05-perf-stat/perf-stat.txt' for reading: No such file or directory`. Listing showed the actual file is `perf-stat.csv`, which was then read and hashed. This was a local file lookup error, not a workload or measurement failure.
- No publication validation, git commit or push failure occurred. The reported SSH authentication, perf addr2line/SIGPIPE and impossible GPU startup readings are historical parent campaign evidence, not failures newly executed by this worker.

### What I learned
- The sampled main-thread Mali-driver share is71.44% only after conditioning28.06% of all-thread sampled CPU periods on39.28% main-thread total. It cannot be applied directly to frame wall time or GPU utilization.
- The output residual is a useful location for further investigation, not proof that an output call caused all preceding dependency work or that deleting it would remove the corresponding delay.
- Static capped frame means near16.667ms can conceal workload differences. They do not establish that post or the second view is free, nor do they prove robust live60FPS.

### What was tricky to build
- Three schema/history layers must remain distinct: legacy unavailable GPU evidence, corrected PERF004 stats schema2, and the separate TRACE005 paired JSONL schema1. The report explains each without rewriting historical receipts.
- Prose had to retain exact failure and capability limits while avoiding the cramped missing-space style of source diary notes. It uses definitions, equations, an actual row, tables and dependency-ordered diagrams rather than analogies or procedural padding.
- Main evidence source is not the retained worker source. Native source files were never edited or pushed; only publication helpers and this new diary are added in the isolated worker worktree. Parent-owned analysis/changelog edits later appeared in main and were left untouched.

### What warrants a second pair of eyes
- Interpretation of impossible API-valid GPU results, nonadditive GPU totals and lack of exact scheduler/fence attribution.
- Distinction between instrumented frame populations, whole-recording perf samples and startup-excluded plots. Counts reconcile, but controlled causal comparisons remain limited by sequential runs and variable clocks/trajectories.
- Preservation of the withdrawn query-active image-equivalence claim: only post-run uninstrumented fixture repeatability was tested.

### What should be done in the future
- Follow the parent's clarified next investigation into exact GPU/driver identity, available counter interfaces and Streamline/gatord compatibility. Offline Compiler/API/frame tools are secondary; documentation can be saved in ticket `sources/`. None of those tools was researched, installed or executed in this publication step.
- Measure a concrete source-preserving change with exact before/after hashes and controls before describing any proposal as an optimization success.

### Code review instructions
- Read [publication helper/README](../scripts/02-vault-report/README.md), [validation manifest](../scripts/02-vault-report/validation.json) and [publication receipt](../scripts/02-vault-report/publication-receipt.json).
- The helper is a pre-publication gate bound to the recorded base, so it intentionally refuses to treat an already published note as a new draft. Post-publication verification compares hashes in the manifest and remote commit identity instead of rerunning benchmark matrices.
- Review the vault note itself for prose/measurement qualifications; all original report files remain unchanged.

### Technical details
- Final evidence checkout: `f400ffa7c658086cf775fd6d27c28403ba09c2bb`; measured runtime: `209a6d092885ff3f0a1efd1efab4252cb4ad2b56`; runtime binary SHA256: `2872ede7219ccd91890de4c0d47d5af8fab687f2b5b2cf9605e25ecb1dfcb1d2`.
- TRACE005 figures copied byte-for-byte:01-cpu-wall,02-frame-cdf,03-query-observer,04-perf-cpu,05-gpu-elapsed. Extra assets:campaign-summary.json,provenance.json,representative-sample.json,report-evidence.json.
- Report figure populations match current provenance: frame/paired warmup excludes frame<60; perf covers whole recording. Four API-valid startup readings near1.84467441e13ms remain in raw records and are not hidden with a custom outlier filter.

## Step 2: Obtain editorial approval and publish the exact scoped note/assets

Parent read the full initial draft and approved publication conditional on two small changes: remove “following” from figure03's alt text because the actual plot compares selected versus other populations, and add a clearly future Mali-profiling research gate. Both changes were applied, and the existing validator was run once again at the final publication boundary.

Committed only the ten intended vault files and pushed `origin main` without force. An independent `git ls-remote origin refs/heads/main` then matched local HEAD. The previous `da2c32a` commit remains the direct parent, and the vault is clean with no staged files. The report request is published, not merely drafted.

### Prompt Context
**User prompt (verbatim):** See Step1.
**Assistant interpretation:** Editorial approval is the final authority boundary before the explicitly authorized scoped vault commit/push.
**Inferred user intent:** Preserve the technical account durably while respecting historical notes and unrelated vault work.
**Commit (code):** Vault `c7b1535f1d2eb951d03001cc6ee8273904f56359` — "Document Falllinie rendering measurements and Mali timer limits".

### What I did
- Sent the exact draft path,5,664-word count, asset/scope information and validation result through `contact_supervisor(reason=need_decision)` and waited for the reply before committing.
- Applied the parent's two edits, reran the existing publication validator once, and recorded the final5,758-word manifest.
- Independently checked origin/main was still `da2c32a` before staging. Compared the exact staged ten-file set against the validation manifest.
- Ran scoped vault `git commit`, `git push origin main`, and independent remote-HEAD comparison. Checked all published file hashes against the manifest and that the vault parent remained `da2c32a`.
- Recorded bounded commit/push logs and the complete receipt under `scripts/02-vault-report/`. No native push or target/system operation was performed.

### Why
- A full editorial read caught a figure-label mismatch that numerical checks alone would not identify. The future research paragraph must not imply confirmed profiling-tool support or executed web/tool research.
- Remote verification makes publication status independently observable rather than relying only on local commit success or push command text.

### What worked
- Parent explicitly approved scoped commit/push after edits; final validator passed.
- Vault commit and remote both equal `c7b1535f1d2eb951d03001cc6ee8273904f56359`; push log records `da2c32a..c7b1535 main -> main`.
- Vault worktree/index clean; all ten file hashes match the validated publication; all older JS/audio/tracing/game/Francois reports and history preserved.

### What didn't work
- No commit/push/remote-verification failure observed. No force push, credential workaround or unrelated staging was needed.

### What I learned
- The figure03 source aggregate includes following-frame statistics, but that does not mean its actual plotted SVG includes that population. Captions must match the plotted representation, not only the underlying JSON fields.

### What was tricky to build
- Approval was conditional, not an unconditional permission to publish the first draft. The final hash manifest records the edited version actually committed, and the receipt records the approval conditions.

### What warrants a second pair of eyes
- Remaining Mali timer/blocked-versus-runnable limitations are substantive research questions, not publication defects. The next investigation must preserve those qualifications until new evidence resolves them.

### What should be done in the future
- Parent may integrate the publication helper/diary commit from this worker worktree. Native runtime and parent task metadata remain outside this publication write scope.

### Code review instructions
- Compare vault commit to its direct parent and confirm exactly one new note plus the unique asset subtree. Read `vault-commit.log`, `vault-push.log` and the independent remote hash in the publication receipt.

### Technical details
- Vault branch: `main`; remote: `ssh://git@github.com/go-go-golems/go-go-parc`; no force flag used.
- Note and every asset hash are in `validation.json` and `publication-receipt.json`; native helper SHA256 is recorded there as well. Final native helper commit hash is supplied in the output handoff after committing this diary.

## Step 3: Parent verifies publication and integrates receipts

Parent independently verified local and remote vault main as `c7b1535f1d2eb951d03001cc6ee8273904f56359`, with a clean vault. Integrated worker publication commit5456d4e as5df50aa; no runtime source or native push occurred. The final report has5758prose words, five actual measured figures and two Mermaid diagrams.

### Prompt Context
**User prompt:** SeeStep1. Subsequent direction makes Mali-specific GPU profiling the next investigation and requests direct work instead of further delegation absent useful parallelism.
**Assistant interpretation:** Close publication bookkeeping and preserve unresolved measurement questions explicitly.
**Inferred user intent:** A durable, pushed report followed by useful GPU research, not more publication machinery.
**Commit:**5df50aa — Publish measured Falllinie rendering report with evidence receipts.

### What I did
Compared `git rev-parse HEAD` with `git ls-remote origin refs/heads/main`; checked all four completed PERF004 tasks against committed tooling, actual runs, conditional candidate ranking and publication. Added1kxj for unresolved Mali timer semantics/counter support. Preserved TRACE005 analysis relations/changelog alongside this checkpoint.

### Why
Publication completion and scientific completeness differ: the report is delivered, but GPU pass costs and exact wait causes remain unresolved.

### What worked
Exact local/remote identity and clean vault; helper-only integration succeeded. No new runtime test matrix or target rerun was needed.

### What didn't work
`docmgr doctor --ticket FALLLINIE-PERF-004 --stale-after 30` reported new `scripts/02-vault-report/README.md` missing frontmatter and unknown topic`documentation`. Added frontmatter to this new helper README and limited this new diary to existing topic`performance`. Two older findings remain in `scripts/README.md` and `scripts/evidence/review-corrections.md`; preserved historical content rather than broadening the publication patch. Those findings do not affect the already published vault report.

### What I learned
A scoped report validator does not imply ticket-wide metadata cleanliness. Keep those claims separate.

### What was tricky to build
The main checkout had parent-owned TRACE005 doc relation/changelog edits during publication. The helper-only cherry-pick did not overlap or discard them.

### What warrants a second pair of eyes
Next GPU research must verify actual driver/counter compatibility before suggesting installed or working Mali tools.

### What should be done in the future
Direct parent research of exact GPU/driver, counter interfaces and Streamline/gatord; preserve useful documentation in the ticket sources folder. No further delegation planned.

### Code review instructions
Inspect vaultc7b1535, native5df50aa and publication receipt; no re-execution of guarded asset preparation against the published directory.

### Technical details
All original measurement inputs stay atf400ffa/runtime209a6d0. This integration changes docs/tasks only and does not supersede raw evidence hashes.
