---
Title: Scene and post shader sensitivity contract
Ticket: FALLLINIE-GPU-007
Status: active
Topics: [performance]
DocType: design-doc
Intent: long-term
Summary: A balanced non-parity two-factor shader experiment using the qualified fixed replay and unchanged binary.
---

# Question and experimental boundary

GPU006 found a mean interval of23.57ms at512×384 dual-view, compared with approximately17.40ms at384×288. CPU reductions alone had not established a frame-time improvement. The next question is whether substantial scene-fragment simplification or a cheap post-processing pass changes that demanding workload.

This is explicitly **non-parity diagnostic work**, not an attempt to ship either shader. Keep the exact GPU006 executable, fixed600-frame seed42 replay,32terrain work units/frame, dual512×384, no application pacing, no audio, ordinary indexed draws and query-off paired clocks. Do not change the canonical assets, production launcher, governor or compositor policy.

## Four asset packages, one executable

1. **baseline:** byte-for-byte copies of the canonical shader assets.
2. **post-copy:** retain the post FBO and fullscreen draw, but use center depth to select white sky or grayscale scene color instead of the print shader. This meaningfully retains the required active depth sampler; the first single-color-read attempt linked but failed the renderer's uD validation.
3. **scene-flat:** retain scene geometry, vertex positions, material channel and snow-circle discard, but emit constant luminance with no original lighting/shadows/contours/glyph evaluation.
4. **both:** combine those two diagnostic substitutions.

Use existing `--assets` selection rather than weakening the replay parser's post-bypass prohibition. The existing runtime shader adapter expects precise declaration and sky-expression anchors. Insert an unconditional diagnostic result and return at the beginning of the original fragment main function; retain the unreachable original body so adaptation still recognizes those anchors. Exact submitted shader dumps in a bounded host smoke make this inspectable. This is deliberately removed work, not a conditional branch whose coverage is unknown.

Scene simplification may change linked vertex/varying work and the downstream post workload. Both substitutions change image content, potentially affecting compression, caching and composition. Keeping FBOs, draw calls and geometry constant does not isolate fragment GPU milliseconds. Do not subtract results and call the difference an independent pass duration.

## Controls and evidence

A balanced sequence runs baseline, post-copy, scene-flat, both, both, scene-flat, post-copy, baseline. Every case has600frames and a45second counter capture. The order balances simple temporal drift; it is not randomized or a statistical proof with many independent repetitions. Summarize each case and the two-run groups, preserving baseline drift.

Package both original HUD/glyph atlases at the parent path expected by the renderer. Record all asset hashes, verify them on target before and after the campaign, and record each case's explicit diagnostic/non-parity classification. Verify deployed executable identity against the qualified GPU006 binary. Receipt source metadata alone does not identify overridden shader files.

All selected-state replay signatures must match. Repeated endpoint images must match within each shader mode. Depth diagnostic images should match across modes; scene buffers should match when only post changes. Final images **must differ** for each deliberately non-parity mode versus baseline. These are post-loop internal-target captures, not full HUD/scanout captures or proof of identical24-bit depth values throughout the run.

The CPU and counter analyzer must validate600-frame sequences, all paired final counts, game/probe status, no GL errors, counter sequence/identity completeness, and contained host-poll intervals. Preserve whole-device scope, backend/host mismatch and frequency/temperature caveats. Do not assume a near60/s loop result means the diagnostic became arbitrarily fast.

## Feature-boundary work

Generate all four shader packages and run one short host replay per package to check actual linking, consistent state and expected image relationships. No runtime source change or broad rebuild/test matrix is needed: the binary and simulation are unchanged. Commit the diagnostic package/runner milestone, then run the actual device comparison and commit evidence plus interpretation. Keep a detailed chronological investigation diary and preserve failures.

## Decision rule

A large repeated distributional change identifies sensitivity to a substantial work/content substitution. It does not accept that substitution for production or explain which original instruction is causal. Depending on the result, investigate a narrower source-faithful change or the presentation limit. Leave canonical rendering unchanged in either outcome.
