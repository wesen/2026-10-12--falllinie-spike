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
**User prompt (verbatim):** "Ok, create a new docmgr ticket, Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable.\n\n\nThen implement, don't overcautiously test, this is optimization work after all, not production software. \n\ncommit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)"
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
