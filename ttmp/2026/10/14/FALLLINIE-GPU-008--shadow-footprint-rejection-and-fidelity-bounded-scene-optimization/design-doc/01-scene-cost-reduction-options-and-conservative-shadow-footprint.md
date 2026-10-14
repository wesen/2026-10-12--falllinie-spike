---
Title: Scene cost reduction options and conservative shadow footprint
Ticket: FALLLINIE-GPU-008
Status: active
Topics: [performance]
DocType: design-doc
Intent: long-term
Summary: Design all proposed scene reductions; implement and measure a shadow diagnostic and conservative footprint candidate first.
---

# Scene shading reduction without flattening the image

## Motivation and boundaries

GPU007 replaced nearly all scene shading with constant luminance: its 23.575 to 16.733 ms change does not identify a single expensive expression and is not acceptable visual fidelity. This ticket starts from the original scene/post shaders. Geometry, precision qualifiers, materials, RNG, ordering, post-processing and simulation remain unchanged. New modes are opt-in; production remains original.

The user requests a design covering the alternatives, implementation, screenshots for diary/report, meaningful commits and a detailed diary. The first implementation milestone is shadow attribution plus a conservative footprint candidate. Other alternatives are designed here, not silently promised as already measured.

## Options and order

1. **Shadow-only diagnostic:** omit the three analytic rider-shadow evaluations while retaining every other scene feature. Explicitly non-parity, replay-only. A symmetric original/off/bounds/bounds/off/original matrix separates broad scene findings from this narrower question.
2. **Conservative footprint rejection:** outside a bound that encloses every possible shadow contribution, use shadow factor one; inside, retain the exact original rayseg/smoothstep/min expressions. No intended appearance concession. This is the first candidate because most terrain is spatially remote from the small rider shadow.
3. **Uniform-term precomputation:** segment vectors, dot(L,v), dot(v,v) and reciprocal denominator candidates can be prepared once. Do not assume source expressions map directly to runtime instructions: inspect submitted source/compiler behavior first. Reassociation and CPU/GPU rounding may change output. Measure separately after bounding so the benefit is not double-counted; most calculations might already be avoided by the bound.
4. **Distance-dependent shadow LOD:** preserve three segments nearby; transition to one softer capsule farther away, eventually fade below visible footprint. This intentionally changes the image. Needs several distances, transitions, both cameras, motion review and user acceptance. Do not implement a silent default simplification.
5. **Distant marking/contour work:** bypass truly zero-contribution terms after their source distance fade completes. Keep derivative evaluation in defined/uniform control flow: moving fwidth or implicit texture derivatives into divergent branches can change results. A second, explicitly lossy option suppresses subpixel markings at distance. Glyphs, x-ray grid and contours need separate tests, particularly the 250 m distance numerals and palette 5. Deferred until shadows are measured.

## Footprint derivation

Let L be the uploaded light direction with nonzero L.y. Define

    F(P) = (P.x - (L.x/L.y)*P.y,
            P.z - (L.z/L.y)*P.y).

F(P+tL)=F(P). Every evaluated shadow point Q is on segment A..B because the original tc is clamped to [0,1]. If the original point-to-ray distance is below the upper smoothstep radius r=1.2*A.w, then

    |F(P).x - F(Q).x| <= r * sqrt(1+(L.x/L.y)^2)
    |F(P).y - F(Q).y| <= r * sqrt(1+(L.z/L.y)^2).

F is linear, so project each endpoint, expand its componentwise min/max by those radii, and union all three rectangles. A receiver outside that union cannot have a shadow factor below one in exact arithmetic. This does not assume flat terrain or a fixed receiver height. It deliberately includes the infinite light line, making it conservative for the original forward-ray clamp.

CPU bounds use the actual float endpoints/light uploaded to GL, not independently rounded double rig positions. Upload slope and rectangle once per view. Keep the original loop and lum multiplication intact inside the guard. Do not replace rayseg with a different distance function.

### Floating-point and fallback policy

The exact-arithmetic argument is not a universal floating-point proof. Qualify bounded coordinates and add generous projected slack. Initial domain: finite segment coordinates and receiver components within +/-8192 world units, |L.x/L.y| and |L.z/L.y| <=2, nondegenerate segments, valid positive radii and normalized light. Expand bounds by an additional 0.25 world units; compute bounds in double from uploaded floats and round outward to float. This is intentionally loose relative to the small rider footprint. Receivers outside the coordinate domain and invalid/unsupported uniform inputs execute the original loop, not an unshadowed fallback. Wider domain qualification can be future work.

CPU tests sample original float distance evaluations against the rejection decision, including near-boundary points, translations, tilted segments and fallback cases. Hardware image comparisons remain mandatory: compiler changes can cause differences even when the algebraic rejection is sound. Any difference must be measured and explained; do not relax equality merely to pass.

## Runtime and screenshot contract

Add --shadow-mode original|off|bounds, default original. Off requires deterministic replay and is labeled non-parity in receipts/logs. Bounds remains an experimental opt-in candidate. Canonical assets stay byte-identical; shader adaptations are explicit and exact submitted bytes can be dumped.

Extend final capture to include full-window top/lower framebuffer images (including applicable HUD), plus internal scene/print/depth diagnostics for both cameras. Readbacks occur only after measurements are detached, on the main thread. They are rendered-window screenshots, not compositor/physical scanout captures. Lower camera restoration must remain intact. Capture-only replay runs at multiple endpoints are separate from timing runs; readback must not contaminate measured loops.

## Qualification and device campaign

- CPU tests at implementation boundary, host GL linking and short replay/image checks, ARM build and exact source/binary provenance.
- Separate device directory; existing key-only SSH; no driver/governor/affinity/display-policy or launcher change.
- Same binary, canonical assets, seed42,600frames,dual512x384,32terrain rows/frame,no audio,fps0,ordinary indexed draws,queries off,paired cadence60. Six symmetric cases,45-second whole-device counter windows.
- Match signature across modes; original/off images may differ. Original/bounds must be compared pixelwise for both internal and full-window endpoints. Retain source captures/differences, not just a written claim.
- Additional original/bounds captures at early and later replay endpoints, another seed, and x-ray palette. Include a later checkpoint to exercise more than the original short trajectory; record whether distance numerals actually enter view rather than assume it.
- Capture scene/print/depth and full-window PNGs losslessly from BMP output with conversion provenance. Publish representative side-by-side images and magnified differences when nonzero.

Counter rates are whole-device, host-window estimates, not utilization or pass durations. CPU scopes and wall intervals remain separate. At a presentation floor, reduced activity without reduced frame time is meaningful but not integrated 60 FPS acceptance. Audio remains disabled in this investigation.

## Acceptance and reporting

A candidate is promising only if repeated actual target results improve relevant cost and image evidence supports intended fidelity. Limited endpoints do not prove all frames/seeds or moving-image equivalence. Keep the mode opt-in even after a successful narrow campaign. If the candidate fails equality, retain it as a rejected experiment or fix it before claiming acceptance. Document all failures promptly. Commit design, implementation and measured/publication milestones separately; native commits remain unpushed. Update the living GPU report without deleting prior findings, and link screenshots in the diary and report.
