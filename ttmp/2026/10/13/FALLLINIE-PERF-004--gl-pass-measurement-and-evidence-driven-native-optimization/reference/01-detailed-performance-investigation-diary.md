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
Baseline dual384x288 mean interval19.7839ms;60FPSperiod16.6667ms;topGLwall9.5481,bottomGLwall8.1037,physics.1557,player.6613 includingstream.0866ms. Separate audio producer is not serial frame cost. No governor/affinity/driver change, JS integration, vault publication or reMarkable delivery in scope.
