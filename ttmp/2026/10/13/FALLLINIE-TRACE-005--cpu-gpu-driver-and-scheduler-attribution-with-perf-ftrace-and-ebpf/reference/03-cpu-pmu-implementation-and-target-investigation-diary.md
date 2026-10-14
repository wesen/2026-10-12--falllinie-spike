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
