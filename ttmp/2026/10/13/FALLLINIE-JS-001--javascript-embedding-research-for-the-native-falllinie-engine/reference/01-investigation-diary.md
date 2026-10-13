---
Title: Investigation diary
Ticket: FALLLINIE-JS-001
Status: active
Topics:
    - research
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-10-13T10:10:27.260849613-04:00
WhatFor: ""
WhenToUse: ""
---

# Investigation diary

## Step 1: Scope and native source boundary inspection

I created a separate research ticket in the managed worktree rooted at native candidate `24b50d8ab6007ba9f66499354331d3d1c93ba30f`. This is documentation and isolated host experimentation only. The existing native trees/trails implementation is merged in this revision but not deployed; the parent reports its last console preflight unreachable. I will not access hardware or modify the parent ticket.

### Prompt Context
**User prompt (verbatim):** "Ok, now what about trees, particle trail, and other features that are missing form the port (compared to original HTML). And also, research how to port JS / integrate JS into our game engine."
**Assistant interpretation:** Compare runtime choices against the frozen HTML and actual C++17 SDL2/GLES3 boundaries, then propose a measured, incremental design rather than restarting the native port.
**Inferred user intent:** Understand remaining browser/native gaps and whether retaining authored JavaScript would reduce ongoing translation effort.

### What I did
- Read AGENTS, full-blown-tech-research-design, docmgr, diary and investigation-entry skills.
- Inspected runtime, game, terrain, world, renderer, pose, particles, audio, feedback and profiler sources, existing oracle generation, CMake and ARM64 toolchain.
- Ran `docmgr ticket create-ticket --ticket FALLLINIE-JS-001 --title 'JavaScript embedding research for the native Falllinie engine' --topics research` and added design/diary documents.
- Confirmed HEAD, clean initial index, x86_64 host GCC 13.3.0 and existing Docker toolchain image `sha256:8068d5a56ed677c85ffd8e02f9ee4e88c9804d56d070aead0d598c44eb0f83dd`.

### Why
Native APIs and frame ownership determine the cost and safety of embedding. A browser HTML file is not a stand-alone ECMAScript module.

### What worked
Separate ticket creation and source inspection. Existing world definitions serve both collisions and rendered objects; particles/trails and native music already exist.

### What didn't work
Parent discovery attempt (reported, not rerun here): Kagi `surf.sock connection refused at /home/manuel/snap/chromium/common/surf-cli/surf.sock`. The parent supplied no full CLI stderr or timestamp; this is the exact failure text available, not evidence of successful search. I will directly fetch known official/upstream URLs and preserve timestamps/hashes instead of restarting browser infrastructure.
Initial HTML read hit the tool's 50KB cap at embedded font line 5; subsequent inspection skips font payload and reads the script body in bounded chunks.

### What I learned
The native frame clock already implements source-shaped fixed 120Hz ticks and single frame scale sampling. Any script adapter must preserve it, not add another requestAnimationFrame loop.

### What was tricky to build
Research source captures must have docmgr metadata; large working source must be ignored, while bounded references and receipts remain committed.

### What warrants a second pair of eyes
The parent must independently qualify trees/trails on-device; nothing in this ticket establishes deployed parity.

### What should be done in the future
Run pure source/native oracles, typed-array ownership/error/interrupt probes and cross-link checks before settling on a runtime.

### Code review instructions
Review the research-only changed path inventory. Start at the guide and experiment README; check no production modifications.

### Technical details
Scripts belong under `scripts/2026/10/13/falllinie-js-runtime/`; `.work/` is ignored. No source/vendor binaries or engine downloads will be committed.
