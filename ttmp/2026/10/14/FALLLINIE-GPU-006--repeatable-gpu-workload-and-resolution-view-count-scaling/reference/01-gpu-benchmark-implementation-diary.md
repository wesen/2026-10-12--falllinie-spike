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

## Step 4: Install the authorized key, complete the device matrix and interpret scaling

The credential blocker was avoidable: prior successful commands supplied the device's default credential per invocation rather than fetching it from a credential manager. After the user explicitly requested key installation, I added the standard local Ed25519 public key with ssh-copy-id, preserving existing authorized keys, and verified login with password/keyboard-interactive authentication and askpass disabled. No password value or private key is retained in this ticket. The runner now uses batch-mode key authentication, so it no longer depends on a transient password environment.

The eight-case device campaign completed successfully. All cases rendered the same600frame selected-state trajectory; all same-resolution endpoint scene/depth/print images match. Dual-view mean frame time increases from16.62ms at256×192 to about17.40ms at384×288 and23.57ms at512×384. Single-view cases stay near60completed frames/s despite rising GPUactivity. That is useful evidence of pixel-work sensitivity and a possible presentation constraint, not calibratedGPUutilization or a proven bandwidth bottleneck.

### Prompt Context
**User prompt (verbatim):** "copy our ssh key over t oavoid this issue in the future"
**Current user prompt (verbatim):** "continue"
**Assistant interpretation:** Remove the recurring authentication dependency, resume the authorized benchmark, preserve real results and document their limits.
**Inferred user intent:** Make forward progress on GPU measurement without repeated credential interruptions.
**Commit (runtime):**32b8bfd640f44c465d3f8b6311400f35eac07e81.
**Previous checkpoint:**ca52f0b — prepared analysis and recorded the authentication blocker.

### What I did
- Verified key-only login to root@192.168.0.202; the host reports RK3566. Installed only the public Ed25519 key, not a private key; existing authorized keys remain.
- Replaced askpass requirements in scripts/03-campaign.sh with explicit identity, BatchMode, strict known-host checking and connection timeout. SSH_IDENTITY_FILE can override the standard key path.
- Ran the eight-case campaign under target-20261014T180649Z; separate candidate deployment, no productionlauncher changes.
- Ran scripts/04-analyze.py successfully against the actualdevice data. Added analysis/01, a reproducible scalingSVG, input/source identity evidence and updated the guide's local measuredcheckpoint.
- Kept the originally uploaded reMarkable guide as the design edition; no unnecessary second upload or vaultpublication.

### Why
The missing environment variable did not mean the credential source was unavailable. Key-based access fixes that workflow problem directly. A fixed trajectory permits meaningful resolution/view comparisons; counter changes expose demand that flat presentation-limited frame times can hide.

### What worked
All8game/probe statuses zero,4800frames total and4320postwarmup frame intervals. All1330paired records reconcile, with no dropped/active/invalid records. All receipts reportGL_error0 and successfulplacement.46800counterrows, zeroerrors/nonfinite values,674selected adjacent hostintervals. The replay signature is6081555844518115723 in every case; same-resolution endpoint images match. Deployed executable hash equals the local qualified build.

### What didn't work
No campaign or analyzerfailure in this retry. Existing driver logs contain the nonfatal diagnostic `Could not open module param file '/sys/module/bifrost_kbase/parameters/large_page_conf'`; workload and countercapture still complete. Earlier authenticationfailure is retained rather than overwritten. No newCPUbuild was needed for runner/document-only changes.

Ticket hygiene then reported two `invalid_frontmatter` errors from `docmgr doctor --ticket FALLLINIE-GPU-006 --stale-after 30`: `frontmatter delimiters '---' not found` in scripts/README.md and scripts/evidence/target-20261014T174658Z/BLOCKED.md. The command returned zero despite those reported errors, so exitstatus alone was insufficient. Added frontmatter while preserving the historicalfailure body, then reran the narrow doctor check; no hardware rerun was necessary. Two nonblocking `missing_numeric_prefix` warnings remain for the conventional README.md and historical BLOCKED.md names. A local validation assertion expecting the exact success banner then raised `AssertionError`; corrected that check to reject actual errors while retaining these explicit warnings. The files were not renamed merely to suppress cosmetic warnings.

### What I learned
Unpaced single-view runs remain close to60loopframes/s while fragmentactivity roughly doubles between smallest and largest internalresolution. This supports a presentation-path limit in lighter cases and demonstrates why unchangedframe time is not evidence of unchangedGPUdemand. At dual512×384, mean23.5738ms andp95 33.4894ms show clear scaling pressure. The opening/closing384×288dual mean differs by only0.110%, but tails stillvary.

### What was tricky to build
The counterselection window is narrower than the fullpostwarmup framewindow because pairedscopes sample every60frames. Preserve that difference instead of dividing mismatchedrates by all-frame throughput and claiming exact events/frame. Single-view and dual-view outputscopes move apparent waiting around; scopewalltime cannot identify an isolatedGPUpass or schedulerstate.

### What warrants a second pair of eyes
Only one run per nonreference case, one seed and ten seconds of simulation. Audiooff and fixed streamingwork differ from production. GPU800MHz snapshots and50.6–53.1°C before/after temperatures do not prove constantclocks or absence of transientthermal effects. CPUfrequencysnapshots vary and after-samples occur after theprobe finishes. Whole-device counters include compositor work; no calibratedbusy percentage, bandwidthlimit or fullsourceparity claim.

### What should be done in the future
Use the replay for a separate, explicitly non-parity post/scene sensitivity test, or investigate the apparent60Hzpresentation constraint. Do not promote lowerresolution or shader changes from this matrix. Integratedaudio acceptance remains separate.

### Code review instructions
Read analysis/01 and scripts/README.md, then inspect scripts/04-analyze.py and the captured summary/provenance. Re-run analysis on target-20261014T180649Z to verify results. Sourcequalification against32b8bfd is in evidence/build/qualification.json. Inspect 03-campaign.sh for key-only access and absence of credentialmaterial.

### Technical details
Executable SHA25690f124a7745a37f76175130bd79ef748b741852bccfe58305548f7946c0d82ed. Counterprobe SHA2561f346e0801c99672e1b22edf26cca5a840f97c5cafd6f041de23d052697c65e1. All sixresolution/view combinations plus repeated384×288dual and its pacedreference,600frames/case,45secondcountercaptures. Framewarmup60; counterhostwindow from sampled postwarmup pairedextent. Production resolution/shaders/drawpolicy unchanged; nativecommits remainunpushed.
