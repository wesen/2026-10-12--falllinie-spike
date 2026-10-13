---
Title: Investigation diary
Ticket: FALLLINIE-PLAY-001
Status: active
Topics:
    - handheld
    - retro-gaming
    - architecture
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://evidence/tricks-hud-fixed-run.log
      Note: Short gameplay verification after real HUD bottleneck
    - Path: repo://src/game.hpp
      Note: Source-based ground and airborne mechanics
    - Path: repo://src/renderer.cpp
      Note: Original graphics and bounded HUD rendering
ExternalSources: []
Summary: ""
LastUpdated: 2026-10-12T20:36:24.222356045-04:00
WhatFor: ""
WhenToUse: ""
---

## Step 1: First source-based playable downhill run

The graphics qualification became a moving downhill game slice rather than another static test. Ground forces, steering, braking/tucking, charged pop, a following camera and terrain patch replacement now run on the handheld. The rider geometry is generated from the original source's primitives, basePose, two-bone IK and drawRider, not substituted with a generic model.

The first 20-second automated ride crossed a cell boundary, charged jumps, became airborne and landed. Its source-derived character was inspected in a framebuffer capture. This is automated execution, not human confirmation of controller feel.

### Prompt Context
**User prompt (verbatim):** "Ok, now continue. We care about getting it going and doing some real gameplay, no need to be overly cautious and test everything all the time, the important thing is playtesting!"
**Additional user prompt (verbatim):** "commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)\n\nUse the fallline.html to port. try to stick as close as possible."
**Subsequent user prompt (verbatim):** "play again so i can record"
**Assistant interpretation:** Build playable increments from the existing original falllinie.html, deploy them and prioritize hands-on playtesting over an expanding test matrix.
**Inferred user intent:** Preserve the original game while making the handheld version usable and recordable promptly.
**Commit (code):** `10a87f0` — first source-based playable downhill slice.

### What I did
- Added game.hpp with a 120 Hz simulation, source ground gravity/grip/drag/turn/pop values and source reset heading/speed.
- Added game_renderer.cpp with following-camera direction blending, damped springs, speed-dependent FOV and replacement of four-cell patches at cell transitions.
- Baked three original crouch poses using isolated original JavaScript definitions; retained 1,812 vertices, 7,842 indices and 23 source material draw groups per pose.
- Applied those local poses to the rider's world frame and interpolated crouch. This is a source-generated pose approximation, not a complete runtime IK/grab/uber port.
- Added physical joystick/dpad and keyboard controls, restart, autoplay and a --play launcher default.
- Ran one bounded 20-second autoplay session and then the requested two-minute recording run.

### Why
The verified renderer already supports the game's appearance. The next useful evidence is a moving rider and usable controls, not further static resolution permutations. Baking source poses provides the original silhouette and materials while deferring the much larger pose/state-machine migration.

### What worked
The first automated run completed 1,187 frames, crossed to patch 0..3, reached z=154.327 and recorded airborne/ground transitions. Frame p95 was 18.1288 ms with GL error 0. See evidence/playable-run.log and evidence/playable-frame.png. The recording replay was launched for 120 seconds after the user's request.

### What didn't work
No build/runtime failure occurred at this first milestone. Several source systems remain deliberately absent: runtime grabs/IK variation, spins/flips, scoring/combo logic, collisions with objects, reactive music and trails. Terrain generation is synchronous at cell transitions, so percentile timing does not prove the absence of isolated hitches.

### What I learned
The original rider can be retained without designing a new avatar: source pose and primitive functions can generate native-ready vertex data. Physical control feel still needs the user's playtest.

### What was tricky to build
The source board's local X is its nose direction. World transformation must preserve that convention; treating local Z as forward would rotate the original stance incorrectly. Each original primitive's material group is preserved rather than drawing the entire rider as one material.

### What warrants a second pair of eyes
Crouch interpolation between baked poses is an approximation. Landing forgiveness and immediate course recovery in this initial slice are not the original full state machine. The automated steering/jump input is not evidence of human controller usability.

### What should be done in the future
Add the source airborne rotation/landing rules and player-visible feedback, then get physical playtest feedback. Follow with original reactive audio, grabs/combos, objects and incremental streaming.

### Code review instructions
Start with game.hpp, game_renderer.cpp and scripts/generate-rider.cjs. Compare original groundStep, resetRider and updateCam in fixtures/falllinie.html; inspect playable-frame.png for the source-derived character.

### Technical details
Dpad indices 13/14/15/16 are up/down/left/right; jump is joystick button 0; START 9 restarts; SELECT 8 plus START exits. Keyboard arrows/WASD drive, SPACE charges/releases jump and R restarts. --autoplay is explicitly synthetic input. No audio is implemented in this slice.

## Step 2: Source airborne tricks, landing checks and visible HUD

The next playable increment implements the source's spin/flip velocity control, automatic rotation completion, quaternion orientation and landing alignment thresholds. Speed, distance, points and contextual controls are now visible in a native HUD using the original embedded IBM Plex Mono font.

The first run exposed a rendering bottleneck in that HUD. Replacing full-screen per-character draws with actual glyph rectangles restored normal frame pacing; this was a concrete gameplay problem, not a reason to run a broad regression matrix.

### Prompt Context
**User prompt (verbatim):** "Continue"
**Assistant interpretation:** Continue the source-based playable port after the requested recording run.
**Inferred user intent:** Add meaningful gameplay and feedback while keeping implementation momentum.
**Commit (code):** Recorded in the subsequent repository checkpoint; prior playable revision is 10a87f0.

### What I did
- Added source-convention quaternions and airborne spin/flip control with snap-to-180/snap-to-360 completion, air drag, surface-normal blending and takeoff-tilt leveling.
- Added original upside-down/sideways/sketchy landing thresholds and the spin/flip/perfect-landing point formula subset.
- Added a simplified 1.4-second bail/recovery state. Full bail physics, combo banking, repetition penalties, grabs and uber scoring are not ported yet.
- Exported an ASCII font atlas from the original embedded IBM Plex Mono font, then rendered status and controls with a native GLES overlay.
- Changed deployment to atomic executable rename so an existing recording process can keep running its original executable. Deployment no longer automatically runs the CPU suite.
- Ran one 18-second gameplay smoke and one 12-second fix verification; no static qualification matrix or full CPU suite was repeated.

### Why
Rotation control and landing consequences make the rider playable rather than a moving demonstration. Visible status allows the human player to evaluate those mechanics without reading SSH logs. The source shader, typography and ground parameters remain the reference.

### What worked
The initial new run landed 180s and accumulated 775 raw trick points, with GL error 0. After the HUD fix, a 12-second run landed multiple 180s, scored 605 points and completed 712 frames: p50=16.6237 ms, p95=18.33 ms, p99=19.0929 ms, GL error 0. Evidence is in tricks-run.log and tricks-hud-fixed-run.log. These are automated observations, not a human playtest verdict.

### What didn't work
`./falllinie-spike --place --autoplay --seconds 18` initially completed only 185 frames with p95=105.445 ms. The HUD reused a full-screen triangle for each character and discarded fragments outside that character's bounds, creating excessive repeated fragment work. The HUD now uses a six-vertex rectangle sized to each glyph or background panel; the otherwise comparable short run returned to approximately 60 frames/second. Raw pre-fix evidence is retained.

### What I learned
A small overlay can dominate a mobile GPU frame if its primitive coverage is much larger than its visible content. Readable feedback needs bounded rasterized geometry, not only correct shader output.

### What was tricky to build
Source rotations compose world-up spin, takeoff-axis flip and leveled takeoff orientation in a specific order. Rendering now uses the resulting quaternion rather than yaw alone. JavaScript rounding at negative half-integers was represented with floor(k+0.5) in the auto-completion logic.

### What warrants a second pair of eyes
Score currently adds raw spin/flip landing points directly; it does not claim source combo/repetition/banking parity. Baked neutral poses do not animate grabs or steering lean. Physical flip and landing feel require human playtesting; the automated path exercises spins, not a complete flip-control matrix.

### What should be done in the future
Playtest steering, jump release, airborne rotation completion and landing recovery. Then improve the source pose/state coverage, reactive soundtrack and incremental terrain updates according to observed playtest friction.

### Code review instructions
Compare game.hpp with source airStep, land and scoreAir; inspect quaternion.hpp and the bounded HUD vertex shader in renderer.cpp. Review the slowdown and corrected logs side by side rather than treating successful GL calls as a performance guarantee.

### Technical details
Air left/right controls spin; up/down controls flips after 0.08 seconds. Release input to allow the original completion controller to converge. The HUD is drawn at drawable resolution after presentation; print-target captures alone do not include it. Font export provenance is the claw-stuff investigation's 04-export-hud-font.js.
