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
