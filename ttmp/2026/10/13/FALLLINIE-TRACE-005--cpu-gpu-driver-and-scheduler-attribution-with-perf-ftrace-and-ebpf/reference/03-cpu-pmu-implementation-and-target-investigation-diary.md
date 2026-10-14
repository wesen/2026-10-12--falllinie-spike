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
