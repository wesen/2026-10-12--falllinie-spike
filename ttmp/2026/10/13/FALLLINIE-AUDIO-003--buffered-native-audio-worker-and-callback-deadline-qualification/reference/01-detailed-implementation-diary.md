---
Title: Detailed implementation diary
Ticket: FALLLINIE-AUDIO-003
Status: active
Topics: [audio, performance]
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Parent investigation, delivery, integration and device qualification history."
LastUpdated: 2026-10-13T17:00:00Z
WhatFor: "Preserve actual requests, decisions, failures, commands and qualification evidence."
WhenToUse: "Resume from the latest relevant checkpoint; expand earlier evidence only as needed."
---

# Implementation diary

## Step 1: Separate authored vinyl noise from callback risk and establish the worker contract

The user requested a new ticket, an intern-facing technical guide, reMarkable delivery and actual implementation with meaningful commits and a detailed diary. The parent created FALLLINIE-AUDIO-003 and grounded the design in the existing native engine rather than assuming a second process would make synthesis cheaper. The initial milestone is documentation/design and a reproducible listening experiment, not an implemented worker.

The preceding A/B test is relevant because it distinguishes a source sound component from timing failures. Both excerpts were precomputed; only the vinyl mix coefficient was zeroed in the second. The user's tentative no-crackle response supports a preference change, while earlier callback budget misses still motivate independent transport qualification.

### Prompt Context
**User prompt (verbatim):**

> ok, Create a new docmgr ticket for that, and Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable.
>
>
> Then implement, commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)

**Assistant interpretation:** Document and deliver the buffered PCM/DSP-worker design, then implement and qualify it with source-preserving options and a copy-only callback.
**Inferred user intent:** Understand and improve the audio architecture, not merely hide computation with a larger buffer or remove source effects silently.

Earlier authored listening feedback (first recorded here): “still crackles though, btw.” and “the second play worked without crackles I think...”. Their uncertainty is retained. The automatically supplied prompt counters/session metadata are not additional user requirements.

### What I did
- Inspected existing audio.hpp, DSP/FFT/convolution code, runtime feedback and recorded target measurements at f5ab7b1.
- Confirmed on the handheld that main and SDLAudioP2 could use cores0–3; audio already had its own thread. No affinity/scheduling policy was changed.
- Built matched offline source/no-vinyl probes under `scripts/2026/10/13/audio-crackle-isolation/`. The source variant is unchanged; the second variant replaces only vinyl_filter's0.22 mix coefficient with0.0 while retaining noise generation, filter processing and RNG history.
- Created a31-second WAV:15seconds reference,1second silence,15seconds matched no-vinyl; played through paplay on the existing speaker sink. The paused native game was preserved. Generated WAVs/binaries stay ignored, while manifests/render logs/tooling are retained.
- Created the ticket and detailed design guide with definitions, state/ownership diagrams, API sketches, decisions, pseudocode, implementation commits and failure-oriented tests.

### Why
A dedicated worker can generate PCM ahead and leave SDL a bounded copy path. This does not reduce required arithmetic or prove sufficient average throughput; queue starvation, latency, ownership and lifecycle must become explicit. The guide therefore treats target delivery as an acceptance gate, not an implementation inference.

### What worked
Matched probe generation and paplay completed successfully; the user tentatively reported clean second playback. Ticket/document creation succeeded. Existing evidence identifies baseline521/1053 over-budget callbacks, optimized512-buffer19/2786, and1024-buffer3/2574; those are callback wall-budget misses, not driver XRUNs. The new design preserves one synthesizer and one live DSP owner.

### What didn't work
Before ticket creation, `pw-top -b -n 3` and a pactl chain exceeded their10-second parent command bounds. Explicit runtime/Pulse socket environment with remote `timeout 3 pactl ...` recovered the Pulse queries; no global daemon configuration changed. The first grim capture reported `failed to create display` and scp reported `/tmp/falllinie-crackle-current.png: No such file or directory`; exporting WAYLAND_DISPLAY=wayland-1 recovered capture. These were diagnostic environment failures, not subagent infrastructure failures or DSP outcomes.

### What I learned
The user-observed crackle cannot all be attributed to callback timing. The source deliberately injects vinyl transients. Pre-rendered matched playback isolates that signal contribution without invoking live synthesis; timing qualification remains separate.

### What was tricky to build
A worker introduces two frontiers: produced musical samples and samples delivered to SDL. Pausing must preserve queued unheard PCM and acknowledge stopped generation, rather than discard data while leaving the musical clock advanced. Ring retirement must account for partial blocks so a producer cannot overwrite data still being copied.

### What warrants a second pair of eyes
Memory publication and cursor ownership; transactional startup/failure cleanup; pause/close lock ordering; offline/live exclusion; callback zero-fill/starvation counters; honest producer/consumer timing. An isolated worker and fresh independent reviewer will own implementation and code review before parent target testing.

### What should be done in the future
Upload the guide, implement transport/lifecycle/options in meaningful commits, inspect an exact candidate diff, then run parent-owned ARM/device comparisons and final listening qualification. Do not close the ticket on uploaded documentation alone.

### Code review instructions
Begin at the guide and audio.hpp/audio.cpp. Inspect `audio-crackle-isolation/manifest.json` and render-only logs for the matched experiment. The offline probe's final PASS line is not evidence that its main executed every CPU test: --render selects evidence rendering, finite/allocation checks, not the full default test branch.

### Technical details
The guide proposes one same-process worker, a bounded PCM SPSC, a small request-aware lead, silence on callback shortage, cumulative starvation counters and independent wall/thread CPU timing. Source-default vinyl and offline behavior remain available; the handheld clean preference is explicit. No production audio changes have been made at this checkpoint. Cloud upload and implementation evidence follow in later steps.

## Step 2: Publish the intern guide before implementation

The parent completed the design checkpoint5a3b745 and uploaded the guide once to the ticket-aware reMarkable cloud destination. Delivery succeeded without renderer/auth investigation. This closes documentation/delivery tasks only; implementation, review and target qualification remain open.

### Prompt Context
**User prompt (verbatim):** See Step 1.
**Assistant interpretation:** Deliver the requested guide before starting the implementation lane.
**Inferred user intent:** Make the detailed analysis and proposed implementation available for reading independently of code progress.
**Commit (code/docs):** 5a3b745 — Document buffered audio worker design and matched vinyl isolation.

### What I did
Ran `remarquee upload bundle <ticket>/design-doc/01-native-audio-timing-analysis-and-buffered-worker-implementation-guide.md --name 'FALLLINIE-AUDIO-003 Buffered Audio Worker Intern Guide' --remote-dir /ai/2026/10/13/FALLLINIE-AUDIO-003 --toc-depth 2 --non-interactive`. Preserved its bounded output in `scripts/initial-guide-upload.log` and prepared scoped implementation/review authority.

### Why
The user requested guide delivery followed by implementation. A new document name/destination avoids overwriting prior annotated reports; no --force was used.

### What worked
`OK: uploaded FALLLINIE-AUDIO-003 Buffered Audio Worker Intern Guide.pdf -> /ai/2026/10/13/FALLLINIE-AUDIO-003`.

### What didn't work
No upload failures observed. No extra local render/PDF inspection or independent cloud listing was requested or necessary under the delivery policy.

### What I learned
Cloud delivery proves that the document reached the cloud destination; it does not prove physical reMarkable synchronization.

### What was tricky to build
The document explicitly separates current f5ab7b1 behavior from planned worker APIs and records pending acceptance. An uploaded design is not an implemented audio feature.

### What warrants a second pair of eyes
The ensuing implementation's queue/lifetime ownership, not the successful routine upload receipt.

### What should be done in the future
Implement in isolated scope, then fresh review and parent target qualification. Keep uploaded guide historical; record finalized APIs/evidence in the repository without assuming another upload is required.

### Code review instructions
Read the guide and Step1 before starting audio changes. Compare the upload log with the destination above; inspect source and recorded test evidence independently.

### Technical details
Parent owns `reference/01-detailed-implementation-diary.md`, ticket task/changelog/index state, deployment and delivery. The implementation worker will own a separate reference diary and scoped source/tests/scripts in an isolated worktree. No device access, installs, push or subagents are delegated.

## Step 3: Queue the worker-authored vault report and retain the first starvation result

The user added a publication follow-up: the implementation worker should write a detailed textbook-style project report after the work is finished, then commit and push the vault. The parent routed that instruction to the existing worker without launching another writer or diverting implementation. Publication waits for the independent review and parent qualification evidence.

The worker subsequently reported two coherent audio commits and12/12 host CTests passing, including actual worker/callback bit identity and full/empty shutdown. A60-second llvmpipe/dummy integration run had negligible callback copy time but one PCM starvation. This is an intermediate worker report, not parent-verified target acceptance.

### Prompt Context
**User prompt (verbatim):**

> have the worker  write a detailed project report for the obsidian vault as a deep dive technical analysis blog post using a textbook writing style (no analogies, see skill).
> Commit and push the bsidian vault when done (go-go-parc vault).    once they're finished

**Assistant interpretation:** Resume the same implementation worker for an append-only vault report after the final native result is known; grant scoped vault commit/push authority then.
**Inferred user intent:** Preserve a rigorous technical account of the finished work, including actual remaining failures, without publishing a design as completed implementation.

### What I did
- Confirmed the workflow and worker entries point to the same child session, not competing writers.
- Sent a bounded steering instruction to live child7c532c54-8195-46ce-a832-0eea5d97d995 with steeringRecovery=false. Delivery was acknowledged under request2ce8653c-25c6-4ac8-a97a-b1e93ac7bf0b.
- Added taskmdph for worker-written report/assets and intended-file-only go-go-parc commit/push after native completion. No vault edits or push happened in this step.
- Preserved the worker's interim failure report without authorizing a larger lead or changing acceptance thresholds.

### Why
The report needs exact final commits, source/API relationships, concurrency/lifecycle reasoning, numerical/test evidence and actual target observations. The worker is already collecting implementation reasoning; reuse that context later rather than create a concurrent vault writer now.

### What worked
Steering was delivered. Worker acknowledged the deferred follow-up. Reported host run: GL_error0,2815callbacks,mean wall0.004962ms,max0.550192ms,zero callback budget misses. Reported12/12 tests include transported bit identity and deterministic full/empty shutdown.

### What didn't work
The same host run reportedly had producer maximum48.956ms, two nominal block-budget exceedances, and one starved callback/1024 missing frames with2048-frame lead. This is not zero-starvation acceptance. Host llvmpipe/dummy results are not Mali/PulseAudio handheld results; full evidence review and sanitizers remain pending.

### What I learned
Moving DSP out of the callback changes which component can miss its supply obligation. A fast copy-only callback can still output fallback silence if the producer does not maintain enough ready PCM.

### What was tricky to build
The asynchronous workflow's buffered-worker and single worker display refer to one actual implementation owner. New publication authority is conditional and separate from the child's current no-device/no-native-push boundary.

### What warrants a second pair of eyes
Inspect exact queue/lifecycle code and the cause/measurement scope of starvation; distinguish producer CPU time from wall time. Review must not convert successful callback counters into total playback acceptance.

### What should be done in the future
Complete implementation/sanitizers/review, integrate and qualify on device, then resume the same worker with final evidence and vault-specific authority. Read Obsidian writing and textbook-authoring skills, preserve older reports/unrelated state, include self-contained assets and commit/push only report files.

### Code review instructions
Use the forthcoming worker handoff/commits/logs rather than treating this interim progress message as executed parent validation. Keep taskmdph open until the vault report is actually committed and remote push succeeds.

### Technical details
Report destination is /home/manuel/code/wesen/go-go-golems/go-go-parc under a new dated Projects note with colocated _assets. Discuss PCM/SPSC publication, DSP/FFT/sample clocks, produced-versus-delivered frontiers, pause/stop ownership, API/file references, tests and limitations in prose, pseudocode and diagrams without analogies.

## Step 4: Integrate reviewed code and measure direct/buffered audio on the handheld

The independent reviewer found no source transport/lifecycle defect and accepted integration with qualification notes. The parent verified the clean candidate, cherry-picked four source milestones and reran12/12 host tests. Two120-second real-backend runs then distinguished synchronous callback DSP from buffered consumption. Buffered mode had zero callback budget misses and zero application PCM starvation with the same2048-frame target that had starved once on the host.

This is acceptance evidence for the finite tested configuration, not universal scheduling, driver XRUN, physical latency,60FPS or final human listening proof. The playable buffered/no-vinyl title was restored.

### Prompt Context
**User prompt (verbatim):** See Steps1 and3.
**Assistant interpretation:** Integrate reviewed implementation and obtain target evidence before the worker-authored vault report.
**Inferred user intent:** Establish working transport with explicit limits, not a callback benchmark that ignores producer starvation.
**Commit (code):** Parent f1781ed,9780e3f,a8894cc,36879ae map to worker a71bf65,ef69e0a,32fe75c,c6e4624.

### What I did
- Confirmed worker clean at c6e4624; recomputed139691-byte patch SHA25669e69d4518871fa0220bcd6a9e701c2c939813bd95a92432e39727df3227a2d1. Read full reviewer report and finalized API03. Saved scripts/independent-review.txt as an unmodified raw reviewer record.
- Cherry-picked four commits without overwriting parent diary/tasks. Rebuilt Release and reran12/12 CTests in5.99seconds, saved scripts/parent-ctest.log. ARM64 -std=c++17 -O2 -Wall -Wextra -pthread compile/link passed without warnings using the existing offline SDL toolchain.
- Added parent-only scripts/05-device-compare.sh, announced replacement of the paused instance, transferred executable/launcher and ran direct first then buffered:120seconds each, autoplay, dual384x288,no-vsync,fps60,muted computation,overlay,actual1024-frame buffer,source vinyl enabled.
- Saved scripts/target/direct and buffered logs/CSV. Restored interactive buffered --no-vinyl with panel, PID419350. No device/system scheduling configuration changed.

### Why
Identical configuration is more informative than earlier differing durations/buffers, but live trajectories/events differ because timing affects physics interactions. These are matched configuration runs, not identical input traces; separate exact offline/worker fixtures establish PCM identity.

### What worked
- Direct:5574callbacks,mean wall12.907375ms,max24.9405ms,18over-budget;mean CPU11.360292ms,max20.557331ms. GL_error0;6006frames/5946measured,p5019.8539,p9521.7346,p9923.0284ms,RSS115988KiB.
- Buffered:5626callbacks,mean wall0.022966ms,max1.61ms,0over-budget;mean CPU0.013177ms,max0.066500ms. Producer5628blocks,mean wall12.765847ms,max27.0392ms,16nominal block-budget exceedances;mean CPU11.293611ms,max21.651957ms. CPU clocks were available for all samples.
- Buffered produced5763072 = delivered5761024 + queued2048 after acknowledged pause. Target2048/capacity8192,sampled low0/high2048,starved callbacks0,missing frames0. Queue high3,drops/steals/nonfinite0.
- Buffered graphics:6061frames/6001measured,p5019.7586,p9521.6821,p9922.8927ms,RSS115692KiB,GL_error0. Both modes remain approximately50FPS.

### What didn't work
Initial shutdown helper used `flock -w 8`; BusyBox reported `flock: invalid option -- 'w'`. The remote compound shell lacked set-e and continued deployment; both actual120-second logs contain normal complete AUDIO/frame results. Corrected helper uses set-e and `timeout 8 flock ... true`, app-run failure guards and explicit-file retrieval to avoid nested rerun directories. Syntax validation is required, not an unnecessary repeat of the completed tests.

Worker TSan0/3 failed before execution with unsupported unexpected-memory-mapping errors. Ticket doctor initially reported `frontmatter delimiters '---' not found` for the raw copied scripts/independent-review.md. Renamed the raw artifact to .txt without modifying its bytes, avoiding a false docmgr-document classification; reran doctor. Host llvmpipe/dummy one-starvation result remains a failed configuration, not erased by target success. Final live listening and physical event-to-speaker latency are still unmeasured. An initial parent diary edit did not match its exact tail text; no file changed, and the corrected narrow replacement appended this entry.

### What I learned
Producer computation remains roughly11.3ms CPU per1024-frame block; the callback no longer computes it. Producer over-budget blocks are not callback failures when ready PCM covers them. Sampled low0 is not starvation if refill precedes the next request. Max CPU and max wall aggregates need not describe the same block.

### What was tricky to build
Parent metadata had diverged from worker base, so bounded cherry-picks preserved both. Paused frontier equality is exact only after acknowledgement; live atomic snapshots are advisory. Budget misses, producer exceedances, missing PCM and device XRUNs require separate labels.

### What warrants a second pair of eyes
Original review covered publication/lifetimes. Remaining questions are rare sustained scheduling gaps, real control latency and live clean-mix listening; do not infer these from zero missing PCM in one finite run.

### What should be done in the future
Ask user to try live buffered/no-vinyl. Resume the same worker for the authorized detailed vault report using final code/evidence while explicitly keeping listening acceptance open. Do not enlarge lead silently or claim60FPS.

### Code review instructions
Read independent-review.txt, API03, parent-ctest.log and both target AUDIO/frame records. Check BusyBox-compatible shutdown/failure guards. Source/library vinyl defaults and explicit --source-vinyl override remain available.

### Technical details
Nominal callback/block duration1024/48000=21.3333ms;full2048-frame application lead42.6667ms is separate from1024-sample wet delay/backend buffering. No physical end-to-end latency or driver XRUN measurement. No affinity/FIFO change. Finite target results permit further use/reporting, not universal or complete-game acceptance.
