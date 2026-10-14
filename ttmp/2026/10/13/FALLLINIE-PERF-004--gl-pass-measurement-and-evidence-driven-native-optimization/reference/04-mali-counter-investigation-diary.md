---
Title: Mali hardware counter investigation diary
Ticket: FALLLINIE-PERF-004
Status: active
Topics: [performance]
DocType: reference
Intent: long-term
Summary: Direct investigation of G52 profiling interfaces, Arm primary documentation and a separate bounded counter probe.
---

## Step 1: Identify the actual driver and build a direct hardware-counter probe

Following the completed vault publication, investigated the GPU tooling directly, without delegation. The device exposes a commercial-driver Mali-G52 through /dev/mali0. Arm's libGPUCounters provides an application-facing counter API for this family, independent of the missing ftrace scheduler instrumentation. This justifies attempting actual counter collection rather than assuming the kernel's disabled FTRACE also prohibits GPU hardware counters.

Saved primary Arm documentation under this ticket's sources folder and built a separate ARM64 probe using the existing offline toolchain. No application/runtime source, installed driver, governor, policy, system package or production launcher was changed. Counter access remains unproven at this build checkpoint.

### Prompt Context
**User prompt (verbatim):** "go ahead. commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)."
**Assistant interpretation:** Proceed with the already identified next step: exact Mali hardware/driver identity, counter interface access and tool compatibility, followed by useful bounded measurements where supported.
**Inferred user intent:** Obtain more precise GPU evidence without another large orchestration or preliminary-test exercise.
**Prior direction:** Work directly unless parallelism genuinely warrants delegation; preserve useful documentation in sources/; publication was completed first as requested.

### What I did
- Ran ticket-local scripts/03-mali-counters/01-inventory.sh read-only on the target; saved evidence/device-inventory.txt.
- Used Kagi queries `Arm libGPUCounters Mali G52 Linux kbase gator Streamline hardware counters profiling` and `Arm Streamline Linux Mali G52 gatord hardware counters /dev/mali0 libGPUCounters requirements`. Primary result: ARM-software/libGPUCounters; supporting Arm gator README and G52 counter reference.
- scripts02-fetch.sh cloned a pinned shallow libGPUCounters checkout into ignored .work, copied README and G52 reference bytes, extracted Arm GitHub pages with Defuddle, recorded URLs/revision/fetchUTC/SHA256 in sources/. Third-party implementation and binaries remain ignored.
- Read API example, counter configuration/sampling implementation and derived counter formulas. Implemented probe.cpp using the public library API: explicit sampling start/stop, up to120s,10Hz, monotonic host request/completion timestamps and backend sample timestamp, named counters, unsupported/error/nonfinite reporting and final counts. Never reuse stale values after sample failure.
- Built through03-build.sh/CMake using existing offline falllinie-spike-toolchain, aarch64 GCC and no installs/network during build.

### Why
Streamline normally involves gatord and host tooling. libGPUCounters exposes public Mali hardware counters directly and is a narrower first path for our existing measured game. Its README explicitly limits support to Arm commercial drivers and targets Bifrost-era GPUs onward; this target fits that source-level compatibility statement. Actual successful ioctls/samples are still required before claiming device support.

### What worked
Inventory identifies Anbernic RG DS Plus/RK3568, Mali-G52 one core r1p0 GPU ID0x07040002, available shader mask0x1, mali_kbase r54p2-02eac0 (UK11.46), libmali.so.1.10.0 and /dev/mali0. renderdoccmd exists; gatord/malioc are not in PATH. Direct probe cross-build succeeded with build ID0704c24e352901986b05d0f38625aa3cd6d5df91; executable SHA is in evidence/probe.sha256.

### What didn't work
Two exploratory rg paths were guessed incorrectly: `backend/include/device/hwcnt` and `hwcpipe/include/hwcpipe/counter_metadata.hpp` returned `No such file or directory (os error 2)`. Correct paths are backend/device/include/device/hwcnt and hwcpipe/include/hwcpipe/types.hpp. No build failure, device counter test or driver setting change occurred in this checkpoint.

### What I learned
The kernel module and userspace library have distinct version identities; a filename containing g29p1 does not establish the running module version. RenderDoc being present proves an executable exists, not that GLES/Mali capture works. Counter formulas named Util generally normalize against GPU-active cycles, not elapsed wall time; they must not be described as whole-interval GPU utilization.

### What was tricky to build
The sampler can leave its previous buffer unchanged on failed collection, so the probe skips retrieval entirely on error. Derived ratios can become nonfinite on idle zero denominators; those remain explicit invalid ratio rows, not zeros. GPU-wide counters include compositor/other clients, unlike PID-scoped CPU perf from TRACE005. Clock-frequency snapshots cannot reconstruct time-varying frequency exactly.

### What warrants a second pair of eyes
Actual driver/interface support, counter interval/reset semantics, whole-device scope, derived metric denominators and matching the game's active interval to probe timestamps. Do not diagnose a shader from whole-device ratios without controlled workload changes.

### What should be done in the future
Run one bounded idle/live/bypass campaign with no-counter baselines once this tooling is ready. Preserve raw counters and failures before interpreting ratios. Confirm supported counters from actual device enumeration. Consider Streamline/gatord integration or shader compiler analysis only if the direct data leaves a question they can answer.

### Code review instructions
Start at scripts/03-mali-counters/probe.cpp, source receipts and device-inventory.txt. The probe has no GL calls and cannot alter game simulation. Rebuild with03-build.sh; do not commit .work or downloaded implementation. The original hardware snapshot includes historical boot warnings and does not claim they were caused by profiling.

### Technical details
Core clock source is CLOCK_MONOTONIC; backend timestamps are retained without asserting they equal the request boundary. Selected counters include GPU/core/fragment/nonfragment/tiler active cycles, external read/write bytes, activity/unit ratios and bus stalls. Success/error/unsupported states remain separate. Sampling is whole-device, not a per-frame pass measurement. No BPF, FTRACE or KPROBE attachment is involved.
