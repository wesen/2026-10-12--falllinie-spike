---
Title: CPU PMU implementation and target investigation diary
Ticket: FALLLINIE-TRACE-005
Status: active
Topics: [performance]
DocType: reference
Intent: long-term
Summary: Main-thread Cortex-A55 counters with a genuinely audio-disabled diagnostic workload.
---

## Step 1: Design and implement a reduced-workload PMU campaign

The earlier measurements identify main-thread executing code and GPU activity, but do not measure CPU cache or branch events. Added a concise design and bounded tooling to collect explicit Cortex-A55 PMU groups on the main thread, plus a genuine no-audio option. Muting alone would retain the DSP producer and therefore would not satisfy the requested isolation.

Read-only qualification found working CPU counters, but generic `cycles` also expanded to a DDR PMU event that was not supported. The implementation explicitly names the CPU PMU to avoid conflating devices. No hardware policy changed.

### Prompt Context
**User prompt (verbatim):** "Ok, create a concise design doc for that, implement, commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)\n\nWe can potentially just disable audio for now and focus on the main game loop and rendering, for our optimization. We can tackle audio later on."
**Assistant interpretation:** Implement CPU PMU measurement and a reduced audio-disabled rendering workload, qualify actual access and record actual results before selecting optimization changes.
**Inferred user intent:** Obtain deeper CPU/cache evidence while temporarily excluding audio competition.

### What I did
- Created design-doc/02 and this diary through docmgr in TRACE005.
- Added --no-audio to src/main.cpp. Both autoplay and interactive-start paths skip AudioEngine::open; normal feedback publication drains game events without submitting to audio. Logs and JSON receipt identify the reduced workload. Default and --muted behavior remain unchanged.
- Added scripts/10-cpu-pmu inventory/build/target/campaign helpers and a tiny monotonic-clock executable for collection envelopes.
- Counter groups use `perf stat --no-inherit -t PID -I 1000` and two explicit PMU events per group. Planned pairs cover cycles/instructions, L1D, L2D, retired branches and L1D TLB. Optional199Hz cycles profile is also main-thread scoped.

### Why
Per-thread attachment excludes independent audio, driver-worker and compositor threads from CPU event counts, though shared resources can still affect the thread. No-audio additionally removes audio competition. Keep this clearly distinct from the integrated-game acceptance workload. One-second interval reads expose counts accumulated by hardware throughout the interval, not one sample of CPU state per second.

### What worked
`armv8_cortex_a55` PMU type9 and named events are exposed. Explicit grouped cpu_cycles/inst_retired successfully count. Host build and15/15CPU tests passed; ARM64 O2/debug/frame-pointer candidate built with the existing offline toolchain. Host two-second autoplay with --no-audio and deliberately invalid SDL_AUDIODRIVER completed with GL_error0, audio_disabled=true and no audio-open failure message. No new audio device was required.

### What didn't work
Initial `perf stat -e cycles,instructions -- sleep 0.1` reported CPU cycles but also `<not supported> rockchip_ddr/cycles/`. This is alias expansion across PMUs, not failure of the Cortex-A55 PMU. Explicit `armv8_cortex_a55/cpu_cycles/` and inst_retired worked. Preserve formal inventory in the upcoming campaign. No build/test failures observed.

### What I learned
The installed perf version7.2.7 exposes more than one hardware PMU. Explicit device selection is necessary even for a familiar event name. No-audio is a workload choice; muted audio is not equivalent.

### What was tricky to build
Do not attach to a wrapper PID or inherit all threads: launch the game directly, record its PID, and use that as the mainTID. Record clock envelope before/after perf separately from its relative interval timestamps; the envelope includes tool startup/teardown and is not exact PMU enable/disable time. Avoid overwriting earlier build receipts or binaries.

### What warrants a second pair of eyes
PMU group scheduling coverage, event denominators and supported aliases must be checked in actual CSV. No general-purpose register dumps, precise load addresses or causal memory-stall proof are claimed. AudioEngine retains inert construction/storage; --no-audio disables device/worker/callback execution, not all game feedback calculations.

### What should be done in the future
Run the bounded campaign, reconcile interval coverage and paired logs, preserve failures if encountered and interpret ratios alongside frame/CPU data. Keep source shaders and all optimization candidates unchanged.

### Code review instructions
Read design-doc/02, main.cpp no_audio guards and scripts/10-cpu-pmu/03-target-case.sh. Build receipts and host smoke are under evidence/. Target workload uses separate /storage/apps/falllinie-pmu-* deployments and does not replace production.

### Technical details
Five grouped-counter20s measurements, one199Hz20s profile and two no-counter brackets; each game runs32s with6s warmup before collection. Seed42,dual384x288,no-vsync,60FPS target,autoplay,paired cadence60. Raw perf.data remains device-local; bounded decoded reports and SHA256 are retained locally. Thread inventories and CPU governor/frequency snapshots provide context without tuning them.

## Step 2: Complete the hardware campaign and qualify CPU/cache evidence

All eight bounded cases completed successfully. Five pairs produced20intervals each with100%reported PMU running coverage, and the requested199Hz cycles profile produced1873samples with zero reported loss. This is now actual CPU hardware-counter evidence, not a proposed capability. The no-audio workload retains the main loop and graphics while eliminating the audio device and DSP worker.

The result is a driver-heavy, low-IPC baseline:0.14537instructions/cycle and71.81%of the separate profile's main-thread cycle weight inlibmali. Cache, TLB and branch event ratios are now available, but they do not establish a causal cache bottleneck or identify which GL operation is expensive.

### Prompt Context
**User prompt:** SeeStep1.
**Assistant interpretation:** Finish qualification and preserve actionable, correctly scoped findings without mixing an optimization change into the experiment.
**Inferred user intent:** Use measured low-level CPU evidence to guide the next render-loop optimization.
**Commit (implementation):**323b095 — Add audio-disabled render diagnostic and main-thread Cortex-A55 PMU campaign.

### What I did
- Ran04-campaign.sh once: two32sno-counter brackets, five32scounter cases, one32sprofile case;20scollections after6swarmup. Separate deployment `/storage/apps/falllinie-pmu-20261014T144623Z`.
- Preserved PMU aliases/type/event encodings, thread inventories, governor snapshots, perf interval CSV, application receipts, paired records and sample reports.
- Decoded an additional DSO-only report from the existing raw recording without repeating collection. Added that report command to the reusable target script for future runs.
- Added05-analyze.py, explicit coverage/count/identity checks, summaries/provenance and analysis/02. Its small positive/negative parser fixtures reject unsupported or multiplexed input rather than turning it into a zero.
- Verified every captured src hash against323b095 and matched local/deployed binarySHA256c2b0fc135c880766a2f1dde2ed819a5aa922654ddcbae460c9a2e36cc141c9c1.

### Why
Paired events measured simultaneously permit valid within-pair ratios without multiplexing loss. Separate groups avoid counter capacity pressure but require separate-run interpretation. A full-period DSO report is more useful than summing only symbol rows above the0.5%display cutoff. The outer monotonic envelope supports honest bounded scope selection without claiming exact perf enable timestamps.

### What worked
- Cycles14,300,334,687;retiredinstructions2,078,852,033;IPC0.1453709.
- L1Drefill/access4.5688%;L2Drefill/access24.3991%;retiredbranchmispredict/retiredbranch9.6470%;L1DTLBrefill/access4.7473%.
- Eachgroup20intervals and100%runningcoverage. Cyclesgroup runningtime10.0136s over20.0062selapsed: coverage is not CPU utilization.
- Cyclesprofile1873samples/0reportedlost; libmali71.81%,application10.65%,libc6.99%,kernel4.79%. Private driver offset0x42d6dc has16.89%weight in the symbol report.
- No-counter postwarmup frame means18.267517/18.275653ms;countermeans18.199694–18.303829ms. The profilecase17.991269ms is not an optimization result.
- All8GL_error0;allreceiptsaudio_disabled=true;allpairedfinalcountsreconcilewithzerodrops/invalid/active. Inventories show only one main thread and named Mali workers, no audio threads.

### What didn't work
No target-case, build, sampling or analysis failure occurred. Genericcyclesalias expansion was already recorded inStep1. One bookkeeping issue was identified before final commit: the repository's generic build-directory ignore rule excluded textual buildreceipts. Explicitly staged only that bounded evidence/build directory, not binaries.

### What I learned
A count group's100%coverage means full PMU scheduling while enabled for the thread, not one full CPU consumed across elapsed wall time. This main thread executes approximately half the20sinterval in the cycles case. A low IPC combined with driver-heavy cycle samples justifies more specific attribution; it does not by itself distinguish memory latency, synchronization or instruction dependencies.

### What was tricky to build
The build happened before the implementationcommit, so its original source-revision receipt names the parentHEAD. Rather than rewrite history, qualification.json verifies every captured source hash against the subsequent implementationcommit. Frame statistics cover frame>=60 across the run; counters cover only the20scollection. Scopes are selected within the recorded outer envelope, and those distinct durations are not silently mixed.

### What warrants a second pair of eyes
These are kernel-exposed named architectural events, not precise memory addresses or latency samples. Separate L1/L2/TLB runs cannot be subtracted into a cache hierarchy. Cycle-profile percentages are period-weighted, not simple sample counts or the earlier cpu-clock metric. No individual driver function name is inferred from the unsymbolized offset.

### What should be done in the future
Use this audio-disabled baseline for a single candidate or a narrower refill/branch-location profile. Interpret the prominent driver offset before assuming uniform lookup is responsible. Re-enable audio for eventual integrated acceptance; no new optimization is implemented here.

### Code review instructions
Start with analysis/02 and scripts/10-cpu-pmu/evidence/summary.json; inspectrawCSV for100%coverage and commands for--no-inherit/-t. Run05-analyze.py to reproduce the aggregates. Design-doc/02 explains scope and non-goals. FifteenCPUtests and hostautoplay no-audio smoke passed at the feature boundary; interactive start is protected by the same explicit guard but was not key-injection-tested.

### Technical details
Counter and record workload is immutable across cases except event selection. GPUelapsedqueriesoff;nogovernor/affinitypolicychange. PMUtype9,explicitarmv8_cortex_a55aliases. No system installs or production launcher replacement. Rawperf.data stays in the private device deployment, with committed SHA256 and decodedreports. All campaign subprocesses completed; no background capture is left running.

## Step 3: Prioritize the CPU PMU report before further attribution

Started the requested continuation with a read-only inspection of the prominent driver offset: copied the target libmali into ignored local storage, verified its known SHA256, and generated a bounded disassembly excerpt. The user then explicitly prioritized a detailed vault report. Paused attribution before any new counter/profile run and wrote the completed PMU investigation as a living technical article.

The report contains3815prose words, three data-driven SVGs, two Mermaid diagrams, the actual worked interval, summary/provenance and build qualification. It was committed/pushed as e649de9b0c6d9ae878944ddd6a08576d4f1e5089, with independently verified remote/local equality. The optimization MOC now includes the ninth Falllinie report. No cache-refill or branch-event location profile is claimed to have occurred.

### Prompt Context
**User prompt (verbatim):** "Continue, commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)"
**User prompt (verbatim, JSON-escaped):** "Actually, first  write a detailed project report for the obsidian vault as a deep dive technical analysis blog post using a textbook writing style (no analogies, see skill).      \n Commit and push the bsidian vault when done (go-go-parc vault).    and we'll update it as we go along."
**Assistant interpretation:** Prioritize publication of the completed baseline, retain updateable structure, then resume further attribution only as subsequent investigation work.
**Inferred user intent:** Establish a detailed shared technical account before more experiments accumulate.
**Commit (measured baseline):**f8f6632 — Qualify audio-disabled CPU PMU baseline and preserve target counter evidence.

### What I did
- Read-only target identity confirmed `/usr/lib/libmali.so.1.10.0`, SHA256605e3a5caf1be62bb48d97b1168434257355d56db5e3be9b515cfa742c173118. No existing game/perf workload matched the process check. Copied the library to scripts/11-cpu-attribution/.work and generated objdump excerpt for requested virtual-address range0x42d650–0x42d780. Identity and excerpt are retained; semantic/address interpretation remains pending.
- Authored `Projects/2026/10/14/PROJECT REPORT - Falllinie CPU PMU - Cache Events, Driver Cycles and an Audio-Disabled Baseline.md` in go-go-parc.
- Explained PMU counting versus sampling/register dumps, no-audio ownership, explicit event-source selection, group scheduling, event running time, real ratios, weighted driver attribution and the pending investigation.
- Added scripts/12-cpu-vault-report/prepare.py and validate.py. Copied actual evidence and generated three figures from summary.json; verified the worked interval against rawCSV.
- Updated the general optimization MOC's catalogue and current-evidence route without revising older measurement claims. Scoped publication to10files: new note,eightassets and MOC update.

### Why
The user requested a report first, so no further hardware measurement was needed. A living report should clearly separate completed results from pending event-location profiles, and should not describe lowIPC or cache ratios as proof of a specific bottleneck.

### What worked
Relative links and wikilinks resolve; SVG/XML and JSON parse; asset hashes match; every published frame mean and counter total matches its summary; the worked interval matches originalCSV. The report uses precise explanations and real data, without analogies. Scoped gitdiff-check passed, commit/push succeeded, remoteHEAD matched e649de9b0c6d9ae878944ddd6a08576d4f1e5089, and vaultstatus was clean.

### What didn't work
The initial publication validator failed at validate.py line28 with `AssertionError`: it required the exact prose string `not a continuous dump`, while the article correctly said `Neither is a continuous dump`. Removed this brittle phrasing assertion rather than changing correct prose to satisfy it. Retained structural/numeric validation and manually reviewed interpretation boundaries. The subsequent validation passed. No experiment or report data changed as a result.

### What I learned
Documentation validation should check actual invariants—links, source numbers and provenance—rather than pretend exact sentence matching proves an interpretation. The distinction between reported100%PMUcoverage and roughly half-wall-time thread execution merits a worked numerical example.

### What was tricky to build
Keep the same-run ratio groups separate from cross-run comparisons; keep whole-run frame statistics separate from20sPMUcounts. The new report explains the precommit build receipt and subsequent source-hash verification instead of silently replacing the older recorded revision. The MOC no longer claims its original eight-report cross-link set contains every later report directly; it provides the updated catalogue and route.

### What warrants a second pair of eyes
No driver function name or cache-causality claim is inferred from0x42d6dc. The disassembly excerpt has not yet been semantically interpreted or independently correlated with ELF/perf address mapping. The report's completed measurement checkpoint remains f8f6632, and current publication does not claim new hardware results.

### What should be done in the future
Continue with bounded event-location attribution and driver-offset interpretation, then update this living report with distinct evidence checkpoints. New results must not retroactively change the baseline's scope. Audio-enabled integrated acceptance remains later work.

### Code review instructions
Read the new vault report and scripts/12-cpu-vault-report/validation.json. NoteSHA2565a44f6bc2ec4f907cff66c0f628a1c701e8f78918d5213fbdc54476449159da1; exact committed file list and publication logs are alongside the validator. Read scripts/11-cpu-attribution/evidence only as preliminary read-only artifacts, not a completed attribution result.

### Technical details
Publication:3815prosewords,3SVGs,2Mermaid diagrams,8relativeassetlinks,8assetfiles,10scopedfiles including MOC. Target captures were not repeated. Native tooling/evidence remains unpushed; vault main was pushed and independently verified. No background collection is running.

## Step 4: Resume event-location attribution and inspect the driver scan

Resumed after publication. The saved driver excerpt shows vector unsigned minima/maxima around the hot offset: `ld4` reads sixteen32-bit elements, pointer advances64bytes, `umin`/`umax` reduce lanes, and a counted loop repeats. This suggests a range scan, not a uniform-name lookup; its inputs and callers still require evidence. The disassembler's nearest exported label `glViewport+...` is not the private function's identity.

Prepared a five-case unchanged-runtime campaign: cycles, L1Drefill, retiredbranchmisprediction, L1DTLBrefill, cycles. Only event selection changes. Extended the existing runner to preserve process mappings and explicit sample configuration; source shaders and game runtime remain untouched.

### Prompt Context
**User prompt (verbatim):** "resyme"
**Assistant interpretation:** Resume the paused targeted attribution, maintain the diary/commit cadence and update the living report when there is new evidence.
**Inferred user intent:** Progress from aggregate PMU data to a concrete optimization hypothesis.

### What I did
Read the saved disassembly, inspected renderer indexed draws and the driver's ELF load segments. Added scripts/11-cpu-attribution/01-campaign.sh and sample-event/frequency configuration to the bounded runner.

### Why
A large instruction-location weight is more actionable when its surrounding algorithm and associated event profiles are known. No precise load attribution is implied by overflow sampling.

### What worked
Executable LOAD segment has file offset0 and virtual address0, permitting matching module-relative offsets to this code segment; process mapping evidence will be retained during each run. Renderer uses32-bit indexed draws, making index-range scanning a hypothesis worth testing, not a proven call path.

### What didn't work
No new failure observed at this preparation checkpoint.

### What I learned
The hot address itself is an ADD immediately after an interleaved vector load. Reading only the sampled opcode would miss the surrounding reduction and could misattribute the cost.

### What was tricky to build
Retain the identical qualified game binary and explicit mainTID/no-inherit while changing the hardware event. Nearest exported-symbol annotations on stripped code must not become invented function names.

### What warrants a second pair of eyes
Frame-pointer driver unwinding may be incomplete; sampling skid and distinct event populations remain. A min/max algorithm alone does not identify its caller or prove a cache-latency cause.

### What should be done in the future
Execute the bounded event campaign, examine address/call-path evidence, then decide whether an index-submission experiment is justified. No candidate optimization is enabled at this checkpoint.

### Code review instructions
Inspect the new campaign helper and existing runner diff; all private binaries stay ignored. Target evidence and analysis will follow in Step5.

### Technical details
Five32s games,20s main-thread199Hz requested-event profiles after6s warmup; no-audio, dual384x288, seed42 and unchanged323b095runtime. Requested events are named Cortex-A55 PMU aliases already counted successfully.

## Step 5: Complete attribution and implement an opt-in indexed-range experiment

All five targeted profiles completed, totaling9895samples with zero reported loss. Driver cycle weight remained72.09%/71.20% in opening/closing cycles cases; the0x42d6dc hotspot remained16.84%/15.56%. Driver attribution was58.52%forL1Drefill,70.51%forretiredbranchmisprediction and72.10%forL1DTLBrefill. The cycle hotspot was not displayed at the0.5%symbol cutoff in those other event reports; this does not mean its actual event weight is zero.

Verified the executable mapping base0x7f95d40000 and ELF executable LOAD offset/VA0. The sampled runtimePC maps to0x42d6dc. Its partial frame-pointer caller maps to0x4103ef; the preceding BL at0x4103ec calls0x42d6c4, the min/max helper entry. This establishes the immediate call relation, not a fully symbolized GL call path. Prepared an opt-in range-draw candidate to test whether supplying known vertex bounds avoids scanning, without altering shader formulas or index data.

### Prompt Context
**User prompt:** SeeStep4; intervening tooling-summary question answered without new measurements.
**User prompt (verbatim):** "continue"
**Assistant interpretation:** Finish the concrete indexed-draw experiment and its evidence, preserving source fidelity and detailed checkpoints.
**Commit (attribution tooling):**d7b3935 — Add targeted CPU event attribution with unchanged no-audio workload.

### What I did
Added11-cpu-attribution/02-analyze.py to reconcile identities, paired completeness, sample/loss counts and the address mapping. Preserved event reports, processmaps, ELFsegments, callgraph and bounded disassembly. Added --draw-ranges, typed GLES3glDrawRangeElements loading and a renderer draw_indexed wrapper. The wrapper supplies start0/end(vertex_count-1) for nonempty meshes; rider subgroups use the full rider vertex domain, a legal broad bound. Empty draws retain the original API behavior. Default remains ordinary glDrawElements.

### Why
The instruction sequence reduces unsigned32-bit values, which is consistent with index-range scanning. The renderer's indices are already generated against known vertex arrays. Supplying bounds is a stronger evidence-led experiment than assuming uniform lookup overhead; the driver may still ignore hints or perform validation, so success must be measured.

### What worked
All attribution case statuses0, GL_error0, no-audio receipts and paired counts valid. Host/ARM candidate builds and15CPUtests passed. No shader, index storage/order, triangle grouping, camera logic or production launcher changed. The same candidate binary will run with flag off/on in ABBAorder.

### What didn't work
Two exact-text edit attempts were rejected before modifying renderer.cpp: first `Found 2 occurrences of edits[1]`, then `Could not find edits[6]` for an incorrectly assumed resize parameter spelling. Read the actual render block/signature and applied unique replacements. These were editing-tool failures, not compilation or target failures.

### What I learned
The sampled ADD follows LD4 in a reduction loop; labeling it as expensive pointer arithmetic would be misleading. The nearest exported `glViewport` label is not the helper's name. Event profiles locate different activity distributions and cannot be interpreted as exclusive time fractions.

### What was tricky to build
Provide legal bounds per boundVAO, not global cumulative counts for streamed cells. Staticcombinedterrain uses its totalvertices; cells/objects/dynamicmeshes use permeshcounts; ridergroups use the containing riderbuffer. Keep null/offset index pointers and GL_UNSIGNED_INT unchanged.

### What warrants a second pair of eyes
A GL range hint is not a promise the driver bypasses the scan. The full callpath remains partially unsymbolized, and overflow samples have skid. Bounded fixed-image equality will not establish universal visual parity.

### What should be done in the future
Run the same-device fixed seven-cell capture pair and four32s cycles-profile ABBAcases. Compare scan weight, paired renderingCPU/wall, frame distributions and GL health. Leave candidate opt-in unless evidence supports a later adoption decision.

### Code review instructions
Inspect renderer::draw_indexed, all six indexed call sites, main.cpp flag/receipt and gl_api.hpp typed entry. Reproduce with13-index-ranges/01-build.sh and02-campaign.sh. Attribution summaries remain separate in11-cpu-attribution/evidence.

### Technical details
Candidate source checkpoint precedes target qualification. Build uses existing offlineARMtoolchain,O2,debug/framepointers; full target receipts will follow. No renderer work is delegated to another thread and no audio is reenabled.

## Step 6: Qualify the range-draw result without claiming a frame-rate win

The four-case range-draw ABBAcampaign completed successfully. The same binary with flagoff/on preserves all three fixed-fixture buffers byte-for-byte. TopsceneCPU falls from2.44–2.91ms to1.42–1.59ms, and unfiltered candidate profiles contain no sampledPC in the identified min/max helper, while both baselines restore its roughly17%cycleweight. This is concrete CPU-side progress.

End-to-end frame performance did not clearly improve. The closing baseline is faster than the opening baseline and the pooled candidate mean is slightly higher. Later output scopes absorb more wall time in these selected records. Keep the candidate opt-in and document the distinction between removing CPU work and meeting the frame target; do not reinterpret wall residual as proven GPUwait.

### Prompt Context
**User prompt:** SeeStep5.
**Assistant interpretation:** Complete the controlled candidate experiment, retain both its benefit and limits, and update the living report as requested.
**Commit (candidate):**487492c — Attribute driver scan hotspot and add opt-in indexed draw ranges.

### What I did
Ran13-index-ranges/02-campaign.sh once: same-device seven-cellcapture pair, then baseline/ranges/ranges/baseline32s games with20scyclesprofiles. Added03-analyze.py for receipt/identity/pairedcount/loss validation, scopedCPUmeans, frame distributions and full-symbol helper-range inspection. Decoded unfiltered symbol reports from the existing recordings without recapturing. Verified buildsource hashes against487492c and local/deployed candidateSHA256b2d88ab072e888be4315173ab75af4a17325f2c03581c9d00e625f5d6113837c. Added analysis/03 with detailed interpretation.

### Why
The earlier0.5%display cutoff could hide small residual scan activity. Full symbol reports establish no observed candidate samples in[0x42d6c4,0x42d758), not a proven zero callcount. Sampled scopeCPU supplies an independent time-domain observation alongside relative profile weights.

### What worked
FourcasesGL_error0;receiptflags match;pairedcountscomplete;7336cyclesamples/0reportedlost. Scene/depth/print BMPs identical acrossstaticfixture. Baselinehelper roundedweights17.12%/16.76%, candidate no sampledhelperoffsets. Top/lower summedviewCPU averages approximately7.012msbaseline versus5.889mscandidate across equally sized selected groups. No shaders or indexdata changed.

### What didn't work
The proposed CPU reduction did not translate into a demonstrated end-to-end improvement: pooled framemeans18.071014msbaseline versus18.224560mscandidate, with meaningful baseline drift. No runtime or collection failure occurred. CandidatepeakRSS122804/119788KiB exceeded baseline113732/112340KiB; retain this observation without assigning an unmeasured cause.

### What I learned
A strongly sampled CPUhotspot can be removed from observed execution while frame tails remain. The outputscope's walltime can grow after earlier CPU submissionwork decreases. Broad legal riderbounds could be worth tightening, but neither increasedGPUwork nor that explanation forRSS has been measured.

### What was tricky to build
Keep one exactbinary for both flags, maintain source/binary receipts despite precommit builds, and avoid summing nested CPUscopes. Report selected20scopegroups perview separately from whole-run frame>=60statistics. No transition to an audio-enabled workload occurred.

### What warrants a second pair of eyes
The main benefit is CPU-side in a finite reduced workload. Staticfixtureidentity is bounded, frequency and trajectories are uncontrolled, and sampleabsence is not exhaustive execution tracing. GLDrawRangeElements bounds are broadlegal domains, not exact rider-subgroup extrema.

### What should be done in the future
Leave --draw-ranges opt-in. Consider tighter subgroupbounds and output/GPUdependency evidence before further changes; eventually restore audio for integrated acceptance. Publish this checkpoint to the living report, not as a claim of stable60FPS.

### Code review instructions
Read analysis/03,13-index-ranges/evidence/summary.json and the full-symbol reports. The analyzer can reproduce all displayed aggregates without another target run. Build tests were already run atStep5; no redundant rebuild/testmatrix was run solely for this commit.

### Technical details
Target/storage/apps/falllinie-ranges-20261014T154415Z;6808postwarmupframes,3417baseline/3391candidate;7336samples. No backgroundcapture remains. Source candidate and production deployment are distinct; productionlauncher/default unchanged.
