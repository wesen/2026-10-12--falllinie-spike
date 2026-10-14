---
Title: Post shader compiler and hardware experiment diary
Ticket: FALLLINIE-PERF-004
Status: active
Topics: [performance]
DocType: reference
Intent: long-term
Summary: Actual Mali Offline Compiler results guide a bounded post-shader experiment; no assumed static-to-runtime speedup.
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
