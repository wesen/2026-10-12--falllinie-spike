---
Title: Detailed performance investigation diary
Ticket: FALLLINIE-PERF-004
Status: active
Topics: [performance]
DocType: reference
Intent: long-term
Summary: Parent investigation decisions, actual target evidence and follow-up gates.
---

## Step 1: Define pass-level measurement before proposing renderer changes

The user requested GL performance measurement and an investigation of optimization, parallelization and precomputation opportunities. We already know that most measured main-loop wall time sits inside two renderer calls, but not which GPU passes or driver operations cause it. This checkpoint creates a separate ticket and explicit measurement contract; no runtime performance change or new device measurement is claimed.

### Prompt Context
**User prompt (verbatim):** "Ok, let's do the GL performance measurement and figuring out ways to make this all tighter / finding the different potential culprits / optimizations / parallelizations / precomputation points."
**Assistant interpretation:** Implement reviewed optional measurement/diagnostic tooling, collect controlled target results, then rank concrete optimization opportunities.
**Inferred user intent:** Recover credible60FPS headroom while retaining original appearance/behavior, rather than moving cheap physics into JS or guessing at shaders.

### What I did
Inspected clean native state and existing src/profiler.cpp/.hpp. Created FALLLINIE-PERF-004, three tasks and a design contract. Existing profiler uses asynchronous elapsed queries around whole views every60frames, polls availability and marks disjoint/pending samples; it cannot simply wrap nested pass queries inside its existing outer elapsed query. Latest native publication helper commit2928b61 did not change runtime source.

### Why
GL call wall time is not pure GPU time. A tile-based renderer may defer work or block later, so pass measurements and controlled bypass comparisons are both necessary. Timing instrumentation can perturb rendering and must have its own overhead gate.

### What worked
Repository was clean. Current query implementation and current two-view boundaries are directly readable. Baseline evidence remains available in AUDIO-003 target/buffered; useful new scripts will be ticket-local.

### What didn't work
No new failure observed; existing hardware GPU-pass measurements do not exist yet. No optimization has been accepted based only on historical9.55/8.10ms view wall times.

### What I learned
Queries must be explicitly nonnested, sparse and nonblocking, with real frame/view/pass attribution and valid/disjoint/missing accounting. Stream time is nested within player time and must not be summed twice.

### What was tricky to build
The same renderer is called twice through a shared context and shared offscreen targets. Per-pass timings and hypothetical two-view overlap need source/driver evidence, not an assumption that two CPU threads can issue GL safely.

### What warrants a second pair of eyes
Timer query lifecycle/disjoint handling; uninstrumented defaults; deterministic scene boundaries; diagnostic bypass correctness; no GL calls off main; no source precision/RNG changes disguised as performance improvements.

### What should be done in the future
Isolated worker builds tooling and source map, fresh reviewer inspects exact candidate, parent integrates and performs bounded ARM64/device comparisons. Rank opportunities with supporting measurements, effort and fidelity risk, then select implementation.

### Code review instructions
Read design-doc/01, src/profiler.cpp/.hpp, src/main.cpp, src/renderer.cpp and src/game_renderer.cpp. Check historical AUDIO-003 CPU CSV rather than interpreting the empty GPU CSV as zero cost.

### Technical details
Baseline dual384x288 mean interval19.7839ms;60FPSperiod16.6667ms;topGLwall9.5481,bottomGLwall8.1037,physics.1557,player.6613 includingstream.0866ms. Separate audio producer is not serial frame cost. No governor/affinity/driver change, JS integration, vault publication or reMarkable delivery in scope at this checkpoint.

## Step 2: Preserve the same worker's context for the requested vault report

The user explicitly requested that the agent implementing and testing the measurement tooling also author the final technical report. Publication is now authorized as a follow-up, after corrected tooling and actual parent-owned target results are available. This preserves implementation knowledge without publishing a report that mistakes host tests for Mali measurements.

### Prompt Context
**User prompt (verbatim, JSON-escaped to preserve whitespace):** `"ask theagent running the tests to  write a detailed project report for the obsidian vault as a deep dive technical analysis blog post using a textbook writing style (no analogies, see skill).      \n Commit and push the bsidian vault when done (go-go-parc vault).    when they are done so we don't lose the context window information"`
**Assistant interpretation:** Route deferred report/publication to the same retained worker session and record a durable continuation task.
**Inferred user intent:** Preserve detailed reasoning, test failures and experimental context in a readable permanent technical article.

### What I did
Delivered acknowledged steering to current resumed worker bca0a52e-f8da-4813-ba76-8f19864ee53a, original implementation child2db35646, requesta2f2b5bd-0022-4c41-af2b-91e45fa17813. Added taskwvpl for same-worker report, validation and scoped vault commit/push. Requested report-ready outline in worker diary/handoff before current correction completion.

### Why
Actual GPU hardware measurements remain pending. The worker should finish schema2 CPU-scope accounting and evidence wording now, then receive parent device receipts before writing final measured conclusions.

### What worked
Steering delivery was acknowledged with automatic recovery disabled; no new writer was started. Deferred report scope is explicit and durable.

### What didn't work
No delivery failure. Initial `git diff --cached --check` rejected trailing whitespace in the verbatim multiline request; encoded that quote as a JSON string to preserve whitespace without a trailing-whitespace line. Current evidence still lacks actual new Mali pass measurements; report publication must not imply they already exist.

### What I learned
The report should preserve instrumentation semantics and experiment limitations alongside optimization proposals, not just collect favorable timing numbers.

### What was tricky to build
Same-worker knowledge continuity and future vault push authority are separate from current no-device/no-native-push boundaries. Parent retains device acceptance and will pass exact final revision/receipts before publication.

### What warrants a second pair of eyes
Report must distinguish CPU wall/GPU elapsed, static fixture/live gameplay, missing/invalid samples, uninstrumented capture repeatability and deferred query-active visual equivalence. Proposed parallel work must retain main-thread GL and one authoritative simulation.

### What should be done in the future
Resume this worker session after correction review/integration and target qualification; load vault-writing/textbook skills, create new append-only dated report with self-contained assets, validate, stage only intended files, commit/push go-go-parc without force and independently verify remote success.

### Code review instructions
Use taskwvpl and worker's forthcoming report-ready handoff. Preserve historical JS/audio reports and unrelated vault state. New useful figure/report scripts belong under PERF004/scripts.

### Technical details
Vault /home/manuel/code/wesen/go-go-golems/go-go-parc. No analogies. No native push or reMarkable request. Do not mark report task complete until actual publication and verification.
