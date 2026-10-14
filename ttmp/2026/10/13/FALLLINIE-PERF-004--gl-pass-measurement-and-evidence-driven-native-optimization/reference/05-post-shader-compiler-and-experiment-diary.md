---
Title: Post shader compiler and hardware experiment diary
Ticket: FALLLINIE-PERF-004
Status: active
Topics:
    - performance
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://src/renderer.cpp
      Note: Optional exact glShaderSource input dump
    - Path: repo://ttmp/2026/10/13/FALLLINIE-PERF-004--gl-pass-measurement-and-evidence-driven-native-optimization/scripts/04-shader-analysis/05-analyze.py
      Note: Actual ABBA metrics and image equality evidence
    - Path: repo://ttmp/2026/10/13/FALLLINIE-PERF-004--gl-pass-measurement-and-evidence-driven-native-optimization/scripts/05-gpu-vault-report/validation.json
      Note: Scoped report publication and content receipt
ExternalSources: []
Summary: Actual Mali Offline Compiler results guide a bounded post-shader experiment; no assumed static-to-runtime speedup.
LastUpdated: 0001-01-01T00:00:00Z
WhatFor: ""
WhenToUse: ""
---

## Step 1: Compile actual adapted shader bytes and compare focused candidates

Continued directly from the real GPU-counter campaign. Downloaded Arm's current official Performance Studio archive into ignored local storage, verified its publishedSHA256 and extracted only the offline compiler subtree without installing system packages. The actual compiler now supplies instruction/register evidence, rather than merely being a suggested tool.

Added an optional developer shader-dump environment variable at the exact glShaderSource boundary. A single host fixture frame obtains the default adapted shader inputs; this is input acquisition, not a performance test. Compiler reports reveal that two individually plausible changes interact strongly through register allocation, which determines the candidate worth trying on hardware.

### Prompt Context
**User prompt (verbatim, JSON-escaped to preserve the line break/trailing space):** "commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill). \n\nThen continue, don't spent time overcautiously testing / preemptively testing, we are debugging / developing right now anyway, no need to test for the test."
**Assistant interpretation:** Use the offline compiler concretely, choose a focused source-preserving candidate, then judge it with actual device measurements rather than repeated preliminary checks.
**Inferred user intent:** Make useful optimization progress and preserve the reasoning/results in commits and diary.

### What I did
- Kagi located Arm's compiler learning path and official product/download pages. Defuddle saved the useful learning path in sources/malioc-learning-path.txt. Browser inspection recovered the actual download link after the previous product-page extraction was empty.
- Downloaded official Arm Performance Studio2026.5 Linuxx86-64 archive via01-fetch-compiler.sh. SHA256f9a776cdabdeb56a228c1d2f153eac6067a38a55d7330d6ac8de84824bcd6c3f matched Arm's response header. Archive/binaries remain in ignored.work; only receipts/reports/scripts are tracked.
- Added FALLLINIE_SHADER_DUMP_DIR in Renderer compile(): when explicitly set, write exact source bytes before glShaderSource. Disabled by default; no per-frame work and no source rewriting. Dump failures fail the requested diagnostic explicitly.
- Built existing host target once and ran one frozen fixture frame to collect8actualshader inputs, including highp depth sampler, wrapped stable sky and relative projection adaptations.
- Compiled every input for Mali-G52;03-variants.py builds baseline, lazy-color, fog-skip and combined diagnostic assets/reports. Canonical assets/shaders are unchanged.

### Why
Analyzing the asset file alone would miss runtime adaptations, particularly the170-entry sky uniform array. Identical GLSLbytes do not mean identical driver compiler: this offline compiler's G52 backend is r51p0, while the target kernel is r54p2 and userspace has its own identity. Static reports rank hypotheses, not actual frame savings.

### What worked
Mali Offline Compiler2026.5.0 build922df8 executed, targeting G52r1p0/Bifrost/OpenGLES with r51p0-00rel0 compiler. Baseline post reports32workregisters/100%occupancy,126uniformregisters,no stack,0%16bitarithmetic; total A/LS/V/T10.63/8/0/3.5 and arithmetic bound. Scene fragment is also estimated arithmetic bound. No broad CPU/audio test matrix was run merely for the tooling.

### What didn't work
The attempted search for a standalone compiler tarball returned no results. Arm currently distributes it within the larger763MiB Performance Studio archive; extracted only the compiler locally. No failed build or shader compilation occurred.

Individual candidate expectations were contradicted by compiler output: lazy-color alone increases total A10.75/T5.0; fog-skip alone uses60registers/50%occupancy. Combined uses32registers/100%occupancy, total A10.80/LS8/T3.0, with shorter fogged path A2.70/T1.0. These are compiled estimates, not claims of improvement.

### What I learned
Moving a source-level sample inside a branch need not make compiled work cheaper. Register allocation and scheduling couple local transformations, so checking a complete candidate matters more than counting source expressions.

### What was tricky to build
Fog skip computes the same center linear depth, then only bypasses the neighborhood when zc>=uFogP.y. In the baseline that region multiplies the edge by zero. All original edge expressions remain in the contributing path; black=max(black,edge) remains outside. Lazy color avoids the center color sample on sky pixels where that value is unused. Sampling is nearest/clamp without mipmaps; no claim about arbitrary sampler settings or invalid depth/uniform inputs is made.

### What warrants a second pair of eyes
Compiler changes can alter floating-point scheduling even when formulas are retained. A bounded same-device image comparison and live measurements are needed; static register/cycle estimates alone cannot accept this shader. The sky's8load/storecycles and arithmetic-heavy paths do not establish measured per-pass GPU times.

### What should be done in the future
Run baseline/combined ABBA live comparisons with counters and a matching frozen fixture image pair. Keep the combined asset diagnostic-only until evidence supports adoption; reject it if it regresses or alters required appearance. Do not mix uniform-location caching or other renderer changes into this comparison.

### Code review instructions
Inspect src/renderer.cpp optional dump and scripts/04-shader-analysis/{02-compile-runtime.sh,03-variants.py}. Compare inputs/runtime/3.frag to generatedbaseline.frag (identical), and inputs/assets/combined/post_fs.glsl to canonicalpost_fs. Raw text/JSON compiler output and source hashes are inevidence/. No new remote compiler/library is installed on the handheld.

### Technical details
Baseline compiler longest path A9.13/LS8/T0.5; combined A9.17/LS8/T0.5. Shortest A3.53/T3.5 versus A2.70/T1.0. Longest/shortest paths are compiler paths, not measured image coverage. Fog endpoint is existing380m; precision, formulas, sky hash sequence and source constants remain unchanged. Diagnostic assets live under the ticket, not canonical game assets.

## Step 2: Finish the ABBA experiment and reject promotion of the candidate

The complete four-run baseline/combined/combined/baseline experiment did not produce a convincing speedup. The pooled postwarmup baseline mean is18.262742ms and candidate18.317052ms, a descriptive difference of+0.054310ms (about0.30%) in the slower direction. Sequential live runs and uncontrolled clocks do not justify treating that tiny difference as a proven regression, but they certainly do not justify promoting it as an optimization.

The same-device fixed seven-cell terrain fixture produced byte-identical scene, depth and final print BMPs between baseline and candidate. That is a useful bounded image result, not universal gameplay parity. The canonical shader remains unchanged; diagnostic assets and all results are retained for the requested detailed report.

### Prompt Context
**User prompt:** SeeStep1.
**User prompt (verbatim):** "keep detailed notes sa you work so we can create solid report. (and use your diary)"
**User prompt (verbatim):** "and then when done, stop for the night"
**User request (subsequent):** Write a detailed textbook-style deep-dive project report in the Obsidian vault, commit/push it, then stop. Publication is the final action sequence; no further experiments are planned tonight.
**Assistant interpretation:** Finish current evidence, record the rejected optimization honestly, publish the requested technical narrative and stop.
**Inferred user intent:** Preserve learning and concrete data without extending the debugging session indefinitely.
**Commit (compiler/tooling):**837bb32 — Analyze exact G52 shader inputs and prepare compiler-guided post variants.

### What I did
- Added an ASSETS override to the bounded counter case runner, preserving its original default; deployed only separate baseline/combined asset folders, unchanged209a6d0game and the qualified counterprobe.
- Obtained matching static seven-cell/seed42/384×288 captures to include farther terrain. Compared exactBMP SHA256 and decoded RGB bytes. Scene/depth/print all identical; printSHA256360a0b856157478e030a43a8c8c264bdb2300273c471c3efe0799c6635d809e8, zero changedpixels.
- Executed four30-second live cases, each with45-second standalone counter collection. Cases returned0. No production launcher or canonical shader changed.
- Added05-analyze.py to reconcile paired final counts,450counter sequences×13metrics per run, actual frame distributions, matched central counter windows and image hashes. Generated actual ABBA-CDF and compiler-comparison SVGs, exact summaries and provenance.

### Why
The combined candidate avoids the compiler-estimated register occupancy loss of fog-skip alone, but only a full workload can decide whether the changed path matters on this game. The successful image fixture establishes a limited correctness observation; it cannot replace timing data or imply broader appearance proof.

### What worked
Four game runs: GL_error0, callbackover_budget0,starvation0,missingframes0. Fourcountercaptures:23400rows,zero sampling errors,successfulstop; all central selected rows finite. Counter windows283/285/285/285intervals. Paired records complete with zero invalid/dropped samples. Fixture images byte-identical.

Postwarmup frame means/p95/p99 inms:
- Baseline01:18.217710/32.837417/33.893343,1581frames.
- Combined02:18.306310/33.054196/34.086162,1576frames.
- Combined03:18.327795/33.111946/34.087039,1576frames.
- Baseline04:18.307947/33.077488/34.046289,1575frames.

Hardware activity likewise remains similar: GPU-active rates805.55/804.11/803.66/803.25Mcycles/s and fragment rates801.04/799.29/798.65/798.50Mcycles/s. These approximate host-window rates still have the clock/frequency qualifications of diary04, not calibrated GPU utilization.

### What didn't work
- First deployment omitted the shared parent-directory font/number atlases. Renderer initialization failed before the fixed capture with `fatal: missing source HUD font atlas`. Shader programs had linked successfully. Preserved failure log at evidence/target-20261014T031958Z/capture-baseline.log. Added originalhud-font.bmp/glyphs.bmp to the private package and ran the full campaign once; no partial measurement matrix was repeated.
- The candidate did not improve frame distributions relative to baseline. This is the result, not a reason to discard the experiment or keep tweaking until a favorable run appears.
- Exploratory Pillow image comparison emitted a getdata deprecation warning for a future release. Byte hashes and decoded pixels agreed; final reproducible analyzer uses raw byte equality and does not depend on that deprecated call.

### What I learned
The compiler avoided one bad candidate choice but could not predict a runtime win. Cheaper shortest-path estimates matter only if that path has sufficient coverage and impact; we have not instrumented coverage. Similar measured rates and frame tails leave the broader post workload as a target, not this local rewrite as a successful fix.

### What was tricky to build
The shader directory is not a self-contained runtime asset package: renderer also resolves ../hud-font.bmp and ../glyphs.bmp. Keep package structure explicit rather than interpreting initialization failure as shader incompatibility. The program binary remained identical across all measured cases; only postassetselection changed.

### What warrants a second pair of eyes
No causal claim that fog coverage was low, no proven0.30%regression, no universal parity claim. Compare baseline drift with the candidate values before making a decision. Static compiler backendr51p0 is not claimed to match installed userspace codegen.

### What should be done in the future
Do not promote this candidate. Future sessions may investigate actual path coverage, queue-wait counters or shader work with stronger evidence. Tonight: publish the report and stop, as explicitly requested.

### Code review instructions
Read evidence/experiment-summary.json/.txt and raw evidence/target-20261014T032113Z, compare inputs/assets/combined/post_fs.glsl against canonical assets/shaders/post_fs.glsl, and inspect05-analyze.py. The prior non-parity bypass campaign is a different experiment; do not substitute its speedup for this candidate's null result.

### Technical details
6308postwarmup frames in thisABBAcomparison;3156baseline/3152candidate. All four45s sampler runs have450sequences,13metrics,5850rows. Image equality is for the explicit staticfixture only. No new hardware run is required for publication.

### Visual evidence
![Static compiler resource tradeoffs](../scripts/04-shader-analysis/evidence/compiler-comparison.svg)
![Actual ABBA frame distributions](../scripts/04-shader-analysis/evidence/frame-abba.svg)

Raw cases, compiler versions/inputs and decision are preserved so the report can explain both useful findings and the rejected optimization.

## Step 3: Publish the technical report and stop for the night

Synthesized the GPU-counter campaign and rejected shader experiment into a new append-only vault report. The4606-word prose narrative explains CPU/GPU measurement distinctions, counter definitions and clock-domain limits, a worked actual counter interval, the non-parity bypass result, exact-input compilation, register tradeoffs and the null ABBA outcome. It uses two Mermaid diagrams, four reproducible SVGs and the bounded image fixture, without analogies.

Committed exactly13new vault files and pushed main successfully. Independently compared the remote main hash with localHEAD; both ared769550d9730853d3cb221bec8bdbee777139487. The vault is clean. This completes the user's publication request and is the stopping point; no further experiments are running or scheduled by this work.

### Prompt Context
**User prompt (verbatim, JSON-escaped):** "write a detailed project report for the obsidian vault as a deep dive technical analysis blog post using a textbook writing style (no analogies, see skill).      \n Commit and push the bsidian vault when done (go-go-parc vault).    \n\nthen stop"
**Assistant interpretation:** Publish a detailed, evidence-grounded textbook-style follow-up, preserve earlier reports, commit/push only the new note/assets, and stop afterward.
**Inferred user intent:** End the night with a durable, readable account of the findings rather than additional optimization attempts.
**Commit (measured experiment):**70c164a18ee10a6c321c767ea00e64e3faa8dc7e.

### What I did
- Followed the vault-writing and textbook-authoring skills: concept ordering, concrete evidence, explicit hypotheses, prose-first explanations and no analogies.
- Authored `Projects/2026/10/13/PROJECT REPORT - Falllinie GPU Profiling - Hardware Counters, Shader Compilation and a Rejected Optimization.md` in go-go-parc.
- Copied a self-contained12-asset subset under `_assets/falllinie-gpu-shader-investigation/`, including actual counter example, source hashes, summaries, fourfigures and two identical fixturePNGs.
- Ran one publication-boundary validator for relative links, data/assets, exact numeric means,6308frame total and interpretation boundaries; viewed the actual fixtureimage. Preserved receipts in scripts/05-gpu-vault-report/.
- Staged only the intended13newfiles, checked the scoped diff, committed, pushed origin/main and independently verified with gitls-remote. Existing vault history and notes remained untouched.

### Why
The post-bypass improvement and combined candidate's null result answer different questions. The report explains that distinction rather than letting the more dramatic bypass numbers imply a successful source-faithful optimization.

### What worked
Publication validation passed;13relative links resolve; fourSVGs parse, allJSON parses, bothPNGs verify, copied hashes match, and report means match raw-derived summaries. Two Mermaid diagrams explain execution evidence and the optimization decision. The source evidence stays in the native repository; the vault contains enough actual data and figures to read independently.

### What didn't work
No publication failure occurred. The previously recorded asset-package failure and null optimization remain part of the report rather than being omitted from the narrative.

### What I learned
A useful technical report can end with rejection of a candidate. The important result is a qualified inference, not a claim that every experiment improved runtime.

### What was tricky to build
Preserved separate identities for game binary/runtime, measurement-tool commits, pinnedcounterlibrary, offlinebackend and installeddriver. Report formulas use declared denominators and an actual timestamp-mismatched record, not a synthetic percentage. The static fixture is explicitly not a universal parity proof.

### What warrants a second pair of eyes
Read the counter rate versus frequency discussion and the ABBA decision together. Neither805Mcycles/s nor+0.30%mean difference is presented as a calibrated utilization or statistically established regression. Future work remains future work.

### What should be done in the future
Stop now, as requested. In a later explicitly resumed session, use the report/diary checkpoint; do not silently promote the diagnostic shader or reinterpret the bypass result as its benefit.

### Code review instructions
Vault commitd769550 contains exactly the new report plus12assets. Native scripts/05-gpu-vault-report/validation.json records their paths and the noteSHA256aca4540ea4556ed0ef993dc08ef629e68bfb355b7484b58fb40ace4753e8cbc1. `remote-verification.txt` proves the observed remote/local equality. Native source/tooling commits remain unpushed.

### Technical details
4606prosewords;2Mermaid diagrams;4actual/compilerSVGfigures;2fixturePNGs;12assetfiles;13totalnewvaultfiles. Prior vaultHEADc7b1535 was preserved as parent. Canonical shader and production launcher unchanged. No further hardware run was needed to publish this evidence.

## Step 4: Connect the reports, then broaden the index to general optimization

The user resumed with editorial requests, not further experiments. First connected all eight Falllinie reports with contextual navigation and a shared reading map. Then broadened that map into `MOC - Optimization and Performance Engineering`, retaining the full Falllinie series and adding older investigations, reciprocal links and external references.

### Prompt Context
**User prompt (verbatim):** "cross link all the optimization reports about falllinie we wrote, now that we hve a fair amount of information"
**User prompt (verbatim):** "oh well actually turn that into a general optimization MOC and look for older ebpf and other reports that would be related, and link to online resources as we ll"
**Assistant interpretation:** Extend navigation across the vault, preserve historical claims, distinguish measured work from plans/simulation/conversations, and publish the result.

### What I did and why
- Found eight Falllinie reports; added seven direct related-report links and one MOC link to each, with checkpoint-specific interpretation notes. Verified original report bodies were preserved and all64newnavigation links resolved. Published asf4810f67dec92c963f9179060f8f06b004bd023f.
- Searched beyond Projects and found the April x264/Go/CGO page-fault report under Research/Institute. Read its two errata before describing it: allocator interception was disproved, real page faults did not explain sustained CPU, and ignored encoder property settings were the final CPU diagnosis. Explicitly warned that lower historical sections retain earlier interpretations.
- Added11older reports spanning screencast metrics/methodology, x264/eBPF, X11 resize/paint, memory/RSS, ESP32 transfer/display, browserSQLite, Goja boundaries and researchctl simulation. Each receives a reciprocal general-MOC link. Two archived eBPF conversations are linked as discussions, not qualified experiments; transcripts were not edited.
- Renamed the index to the general MOC, kept the old title as an alias, updated all eight Falllinie links, and retained the complete specialized catalogue within the broader map.
- Used Kagi to discover methodology and official references. Linked USE, perf, off-CPU, flame graphs, versioned bpftrace docs/project, BCC, Go diagnostics, Arm counters and compiler; no claim that every tool works on the handheld.

### Results and validation
Validated54general-MOC wikilinks and21scoped changed paths. Published and pushed generalization commit487c23cba9680b5fac9c60de91cea383a32408cf; independent gitls-remote matched localHEAD and vault status was clean. Both navigation commits have receipts in scripts/06-report-links/.

### Failure and recovery
The first final diff-check found an existing whitespace-only EOF line in the SQLite article exposed by appending navigation: `Projects/2026/05/03/ARTICLE - SQLite in the Browser - Measuring and Fixing sql.js Performance in Static Code Review Sites.md:873: trailing whitespace.` Removed the two spaces on that blank separator, restaged only that article and reran the scoped check successfully. No substantive historical content changed.

### Review and stopping point
Inspect the general MOC, the eight report navigation addenda and11older-report backlinks. Original publication content hashes remain historical receipts; later editorial commits intentionally change note hashes without changing measurements or figures. No new hardware work, optimization or deployment occurred. Native remains unpushed; stop after the requested editorial publication.
