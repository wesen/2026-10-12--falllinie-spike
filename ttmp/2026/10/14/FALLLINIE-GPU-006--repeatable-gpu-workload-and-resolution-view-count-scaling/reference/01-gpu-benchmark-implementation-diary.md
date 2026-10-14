---
Title: GPU benchmark implementation diary
Ticket: FALLLINIE-GPU-006
Status: active
Topics: [performance]
DocType: reference
Intent: long-term
Summary: Intern guide delivery, fixed-step live replay and actual resolution/view-count scaling.
---

## Step 1: Establish the ticket, explain the system and deliver the guide

Created a new ticket rather than extending the CPU attribution ticket indefinitely. The guide explains the existing simulation, renderer ownership, streaming, clocks and counter limits before specifying a frame-indexed live fixture. A fixed simulationdt alone is insufficient: terrain-job progress currently depends on a3.5mswall budget, so the design also fixes streaming work units.

Uploaded the guide before implementation as requested. The retained result is `OK: uploaded FALLLINIE GPU 006 Intern Guide.pdf -> /ai/2026/10/14/FALLLINIE-GPU-006`. This proves cloud delivery, not physicaldevice synchronization, and is not evidence that the designed feature is already implemented.

### Prompt Context
**User request (summary from compacted history; original wording unavailable):** Create a new docmgr ticket with a detailed, technically clear intern-facing analysis/design/implementation guide using prose, bullets, pseudocode, diagrams and API/file references. Upload it to reMarkable, then implement with proportionate testing, meaningful commits and the detailed diary format. Attribution correction at Step3: the earlier checkpoint incorrectly labeled a reconstruction as verbatim; no verbatim claim is supported.
**Assistant interpretation:** Deliver the technical design first, then implement and run a focused replay/scaling experiment with meaningful feature-boundary validation.
**Inferred user intent:** Give an intern enough architecture and concrete contracts to reproduce and extend GPU measurements, while continuing practical optimization work.

### What I did
Read the research/delivery skills and current loop, runtime, renderer and terrain-job code. Created ticket, guide, diary and three tasks through docmgr. Wrote prose, two textual diagrams, pseudocode, file/API references, decision records and implementation/measurement phases. Uploaded via one remarquee bundle command; no redundant render/listing cycle.

### Why
Live wall-dt autoplay differs between configurations. Replay must control simulation updates, camera evolution, terrain residency and environmental inputs while retaining real game rendering. Existing frozen terrain is not representative enough.

### What worked
Guide delivery succeeded on the first attempt. Current APIs support fixed inputdt and can share the same terrain row algorithm with a deterministic work-unit budget. Audio-disabled mode and counterprobe already exist.

### What didn't work
No failure observed in this design/delivery step.

### What I learned
Persistent bestscore, physical focus/input and time-budgeted streaming are sources of variation beyond simulationdt. They need explicit diagnostic policies, not hidden assumptions.

### What was tricky to build
Explain replay as a generated-input trajectory rather than a recording of human inputs or proof of universal determinism. Keep wall measurement clocks independent of simulation time; preserve productiondefaults.

### What warrants a second pair of eyes
Signature coverage is necessarily partial; same-resolution image checks supplement it. Whole-device counters and unmatched backend timestamps retain their earlier limitations. No hardwarebusy percentage or isolatedpass duration is promised.

### What should be done in the future
Implement the guide's first slice, validate at the feature boundary, then run actual bounded matrix cases and interpret observed scaling.

### Code review instructions
Start with design-doc/01, main.cpp loop, game_renderer.cpp streaming and TerrainJob::step. Upload receipt is scripts/evidence/remarkable-upload.txt. Guide status is design-delivered, implementationpending at this checkpoint.

### Technical details
Proposed600frame trajectory,1/60simulation step,32row-work units/frame;256×192/384×288/512×384 internal targets, one/twoviews, realcamera/HUD/particles, audiooff. Countercapture remains bounded and does not alter governors or productiondeployment.

## Step 2: Implement frame-indexed replay and validate the feature boundary

Implemented --benchmark-replay with1/60simulationdt, seededautoplay, no-audio, disabled persistent score access and ignored gameplay/focus mutations. The real rendering path remains active. TerrainJob shares its original numerical algorithm but can stop after32row-workunits, making residency progression independent of wall time. Production defaults retain wall-dt and3.5msstreaming.

Host validation compares120frames of single-view uncapped rendering against dual-view60FPS pacing. Both produce replay signature13752613737572218072 and byte-identical finalprint images. All15CPUtests pass, including added comparisons of fixed1/32unit terrain generation against the synchronous reference. ARM candidate built successfully; target scaling remains the next gate.

### Prompt Context
**User prompt:** SeeStep1.
**Assistant interpretation:** Implement practical repeatability before measuring scaling, without an unrelated validationmatrix.
**Commit (guide/delivery):**c7ca46a.

### What I did
Changed main.cpp configuration/loop/receipt, renderer replaypolicy and terrain rowbudget. Added scripts/01-build.sh,02-target-case.sh and03-campaign.sh. The targetcampaign uses three internalresolutions, one/twoviews, repeated384×288dual reference and one pacedreference; all have600frames and45scountercaptures.

### Why
Fixing onlydt would leave geometry arrival time dependent on CPU speed. Selected-state rollingfingerprint excludes timing and resolution but includes gameposition/velocity/time/score, cameraeye/look and terrainvertex/index counts. Same-size captures supplement the partial fingerprint.

### What worked
Host/ARMbuilds,15tests and cross-view/pacing replay check passed. Autoplay starts without audio; no change to physics/shader formulas or productionlauncher.

### What didn't work
No implementation or validationfailure observed at this checkpoint.

### What I learned
A deterministic workbudget can reuse the same rowstate machine. Benchmark evidence should name its altered streamingpolicy rather than presenting it as identical production scheduling.

### What was tricky to build
Do not hash objectpadding, pointers or timing. Serialize selected scalar fields in explicit order/precision and fold bytes. The hash is same-binary/platform evidence, not universal floatingpoint determinism. Preserve feedback draining and ignore focuspause when comparing windowcounts.

### What warrants a second pair of eyes
The fingerprint does not encode everyparticle/mesh byte; images are only checked at a bounded endpoint. Counters remain whole-device and rates use hostwindows. Developer-overlay timing text and persistedbestscore are intentionally excluded.

### What should be done in the future
Execute and analyze the targetmatrix; reject same-trajectory claims if signatures differ. Document actual scaling and any collection failures before declaring this ticketimplemented.

### Code review instructions
Inspect main.cpp benchmarkguards, game_renderer.cpp TerrainJob call, TerrainJob::step and tests/test_terrain.cpp. Host receipts/logs/images are scripts/evidence/host; build receipts are evidence/build.

### Technical details
Simulation updatefixed1/60, original120Hzphysics internal;32sampling/emissionworkunits perframe. Frame/CPUclocks remainreal. Actualfingerprint processing is outside profiledrender scopes and its small fixed workload still contributes to loop timing. Private targetdeployment only.

## Step 3: Preserve the authentication blocker and prepare measurement analysis

The attempted actualdevice campaign stopped before deployment. SSH rejected authentication because the external helper's ROCKNIX_PW variable is absent from this agent environment. No GPU006target workload launched and no target scaling result exists. I did not guess credentials or search for secrets; the user needs to restore the external credential environment.

Completed the bounded analyzer and build-source qualification helper while preserving the failed attempt. The analyzer requires the complete eightcase matrix,600frames/case, matching tracked replay trajectories, same-resolution endpointimage equality, healthy pairedrecords and validcounter completion. It reports actual frame distributions, scopedCPU/wall means and whole-device counter rates using contained hostpoll intervals. Its syntax is checked; actualGPU006data validation is deferred because of authentication.

### Prompt Context
**User prompt:** SeeStep1.
**Implementation commit:**32b8bfd640f44c465d3f8b6311400f35eac07e81.
**Assistant interpretation:** Retain an honest blocked checkpoint rather than inventing benchmarkresults or silently weakening qualification.

### What I did
Ran `SSH_ASKPASS=/tmp/askpass.sh bash <ticket>/scripts/03-campaign.sh`, redirecting stdout/stderr to scripts/evidence/campaign-launch.txt. Checked only helper variable names and whether their environmentvalues were present. Added scripts/04-analyze.py,05-qualify-build.py and a resumableREADME. Added docmgr file relations and ran doctor successfully.

### Why
The prior captures cannot replace the new deterministic workload. Hostimage equality and CPUtests establish a feature boundary, not target GPU scaling. Counterselection must not mix unknown backendtimestamps with hostframe timestamps.

### What worked
Guide delivery, host/ARMbuilds and replaychecks remain valid. The source manifest can be verified against the implementationcommit even though compilation preceded that commit. docmgr doctor passed.

### What didn't work
Campaign exit255, before anytargetworkload:
```text
Permission denied, please try again.
Permission denied, please try again.
Received disconnect from 192.168.0.202 port 22:2: Too many authentication failures
Disconnected from 192.168.0.202 port 22
```
A first ad-hoc sourcequalification command also failed because manifest paths are absolute and were passed unchanged into `git show revision:path`:
```text
fatal: path '/home/manuel/code/wesen/2026-10-12--falllinie-spike/src/audio.cpp' exists on disk, but not in '32b8bfd640f44c465d3f8b6311400f35eac07e81'
```
Corrected the reusable helper to convert absolute manifest paths to repositoryrelative paths. This was a provenance-script failure, not a compile/runtime failure.

### What I learned
Credential environment does not necessarily survive session continuation. Check credentialpresence without exposing values; preserve failed preflight evidence separately from actualmeasurementdata.

### What was tricky to build
Do not normalize missingcounter values over a fullwindow as though they were zero; the analyzer returns null rates for incomplete countercoverage. Compare endpointimages only within one internalresolution. Reject incomplete campaign directories rather than emitting empty success summaries.

### What warrants a second pair of eyes
Actual targetcampaign, analyzer end-to-end validation and source-faithful integrated/audio acceptance remain unperformed. Eightcases are exploratory scaling, not repeated statistics for each configuration. Repeatedreferencebrackets detect some drift, not every confounder.

### What should be done in the future
Restore ROCKNIX_PW outside chat, rerun scripts/03-campaign.sh, analyze the resulting complete directory and interpret resolution/viewcount sensitivity. Leave the measurementtask unchecked until those results exist.

### Code review instructions
Check scripts/README.md and evidence/target-20261014T174658Z/BLOCKED.md first. Then review analyzerwindow selection, finalrecord checks and signature/image assertions. No productiondeployment, nativepush or vaultpublication was performed in this ticket.

### Technical details
Native changes are committed locally. The failedattempt had no remote run; targetcredential remains the only known blocker to the next measurement step. Sourcequalification is explicitly buildidentity only and does not claim deviceacceptance.
