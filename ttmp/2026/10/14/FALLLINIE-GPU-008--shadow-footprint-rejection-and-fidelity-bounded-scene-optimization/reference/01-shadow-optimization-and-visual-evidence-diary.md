---
Title: Shadow optimization and visual evidence diary
Ticket: FALLLINIE-GPU-008
Status: active
Topics: [performance]
DocType: reference
Intent: long-term
Summary: Detailed implementation, failures, measurements and screenshot evidence.
---

## Step 1: Design narrower scene experiments before implementation

The broad flat-scene intervention removed too many features to identify a useful optimization. Designed all five proposed alternatives, then selected shadow-only attribution and conservative light-space footprint rejection as the first implementation. The bound works for sloped receivers; it does not pretend terrain is flat.

The design separates exact-arithmetic reasoning from finite-precision qualification and keeps an original-loop fallback for unsupported coordinates. Full-window screenshots and internal diagnostics will be captured outside timing loops, at several replay endpoints.

### Prompt Context
**User prompt (verbatim):** "Ok, Add design doc for all this, then Work on it, take screenshots for the diary and writeup report. commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)"
**Assistant interpretation:** Design the alternatives, implement a bounded first candidate, measure it on hardware and publish visual evidence.
**Inferred user intent:** Obtain practical image-preserving optimization rather than another flattened rendering demonstration.

### What I did
Read the original scene shader, renderer uniform uploads, final capture flow and replay controls. Created GPU008 design and this diary.

### Why
The source runs three analytic segment-distance evaluations per terrain/trail fragment. Their projected support can be bounded independently of receiver height.

### What worked
Identified a conservative exact-arithmetic rectangle using F(P)=(P.x-L.x/L.y*P.y,P.z-L.z/L.y*P.y). Original segment tc is clamped, so projected endpoints enclose every possible Q.

### What didn't work
No implementation or target test has run at this checkpoint.

### What I learned
A horizontal ground-plane radius around the rider would be unsafe on slopes. Light-invariant coordinates avoid that assumption; float bounds still need slack and validation.

### What was tricky to build
Do not substitute approximate shadows while calling the result conservative. Retain original loop math and fall back when inputs leave the qualified domain.

### What warrants a second pair of eyes
The floating-point margin is an engineering qualification, not a formal proof for arbitrary worlds. Compiler output changes can also affect images.

### What should be done in the future
Implement the selected modes and screenshot path, run boundary tests and real device comparisons, then decide whether precomputation or a different scene component is justified.

### Code review instructions
Read design-doc/01, assets/shaders/scene_fs.glsl and renderer.cpp's shadow uploads. Review capture ordering and render_first_person camera restoration.

### Technical details
Default original; off is non-parity/replay-only; bounds is opt-in. Proposed finite domain +/-8192, slope components <=2, outward bounds plus0.25 projected slack. No kernel/governor/production change.

## Step 2: Implement the guarded original loop and dual-view screenshots

Implemented original/off/bounds modes and a CPU-computed light-invariant rectangle using actual uploaded float segments. The bounds shader retains the original shadow loop verbatim inside a conditional; unsupported inputs retain original evaluation. Default rendering and canonical shader assets remain unchanged.

Host and ARM builds succeeded, all16 CPU tests passed, and a600,000-point float-reference stress test found zero rejected shadow-contributing samples. The short host replay has identical tracked state in all modes; original/bounds match every pixel across eight capture targets, while shadow-off changes1,402 pixels across those targets. This is host qualification, not target fidelity or speed evidence.

### Prompt Context
**User prompt:** See Step1.
**Assistant interpretation:** Implement a measurable candidate, retain a narrower non-parity control and produce complete screenshot evidence.
**Commit (design):**88f1f32.

### What I did
Added shadow_bounds.hpp and test_shadow_bounds.cpp; wired CLI/receipt policy and renderer uploads. Refactored framebuffer saving to share readback code and added top/lower full-window screenshots plus both cameras' internal targets. Added build, host qualification, bounded device/capture runners and lossless PNG conversion with original BMP hashes.

### Why
Compute the box from uniforms once per view, not independently per fragment. Keep screenshot readback after profiler detachment and retain camera restoration. Avoid relying solely on tiny internal print images when checking the HUD and enlarged presentation.

### What worked
01-build.sh: host/ARM build success,16/16 CPU tests. Float test:600000 samples,300280 rejected,207121 contributing samples retained.02-host-check.py: signature13752613737572218072, zero candidate pixel differences across scene/depth/print/output for top and lower views. Shadow-only omission visibly differs. No compile warnings observed in build receipts.

### What didn't work
No build or host qualification failure. Device performance and fidelity remain unmeasured at this checkpoint.

### What I learned
Full-window captures can reuse the same checked RGBA readback/vertical flip as internal diagnostics; there is no need for compositor policy changes. Readbacks still do not establish physical scanout appearance.

### What was tricky to build
Use the same float input coordinates as GL, outward rounding and0.25 slack; do not derive the bound from independently rounded doubles. Conditional receivers outside +/-8192 or unsupported segments evaluate the source loop. Keep lower render/capture ordering from corrupting top endpoint artifacts.

### What warrants a second pair of eyes
The sampling test is not a proof for all floats or GPU instruction lowering. Target pixel equality remains a separate gate. Domain limits and generous slack are documented rather than hidden behind a claim of universal equivalence.

### What should be done in the future
Run multi-endpoint target screenshots and the six-case timing/counter matrix. Continue measurement if pixels differ, but block fidelity acceptance and retain differences.

### Code review instructions
Start with shadow_bounds.hpp, then its test, renderer upload/adaptation/capture changes and main.cpp's post-loop two-window capture. Inspect host/validation.json and lossless-conversion.json records.

### Technical details
New --shadow-mode defaults original; off requires replay. Screenshot dimensions follow actual drawables. Build precedes implementation commit; source-hashes.txt records the compiled source bytes for later commit qualification. Snapshot runs are separate from measured cases; all mode-specific assets remain canonical.

## Step 3: Measure the candidate and inspect actual handheld screenshots

All ten visual runs and six timed runs completed successfully. The bounds candidate reduced pooled mean interval from23.574617 to20.636466ms, about12.46%, with p95 moving from33.527307 to29.381448ms. Shadow omission reached20.024081ms but visibly removes shading; bounds keeps the source loop where it can contribute.

All40 multi-endpoint original/bounds image pairs match every pixel, including top HUD output and lower first-person output. The timed repetitions also match endpoint images, and the new baseline retains GPU007's original top scene/depth/print pixels. This is a useful fidelity-preserving candidate within tested evidence, not proof of every possible frame or integrated60FPS.

### Prompt Context
**User prompt:** See Step1.
**Assistant interpretation:** Run actual hardware measurements and use screenshots to verify what is retained, not just claim equivalent mathematics.
**Commit (implementation):**e970ec5 — Add opt-in conservative shadow bounds and dual-view capture qualification.

### What I did
Ran05-campaign.py into target-20261014T194947Z. Collected five original/bounds visual pairs (120/600/1200frames, another seed, x-ray palette), then original/off/bounds/bounds/off/original timing cases. Ran06-analyze.py and07-figures.py, checked baseline continuity and source-build qualification, and wrote analysis/01. Inspected actual full-window output, the later509m checkpoint and the enlarged shadow crop.

### Why
The narrow omission control identifies actual shadow-related sensitivity; the bounded candidate tries to preserve it. Multiple endpoints and both cameras provide stronger evidence than one internal print image. A symmetric matrix helps expose drift without pretending it is randomized statistical inference.

### What worked
All16 target executions succeeded. Timing:3600frames,3240postwarmup intervals,1200reconciled paired records,35100counterrows,614contained host intervals. All timed signatures6081555844518115723. Assets match before/after; all40 visual comparisons are pixel-equal. Original means23.5747/23.5746ms and bounds20.6217/20.6512ms. Build source hashes match e970ec5.

### What didn't work
No target/analysis failure. The optimization does not meet60FPS: mean is20.636ms and p99 remains32.626ms. CPU scopes do not show a qualified CPU improvement. The exact floating-point rejection guarantee outside the tested domain remains unproven; the original-loop fallback is intentional.

### What I learned
Bounds recovers about83% of the mean-interval difference between original and omission, but that is not a count of avoided shadow instructions. Fragment activity remains near801Mcycles/s as frames finish faster. Keeping full visual output preserves the source's local pink shadow region that the omission control removes.

### What was tricky to build
Readbacks happen after measurement detachment; full output must be saved before diagnostic depth rendering overwrites the internal print target. Compare all RGBA channels, not only an alpha-based difference bounding box. For the illustrative crop, convert to RGB before finding the shadow-off difference region; retain the entire difference rectangle plus margin.

### What warrants a second pair of eyes
Only five visual checkpoints, two timing runs per mode and audio disabled. Float test sampling is not formal proof; GPU compiler transformations still need broader scene/motion qualification. Opening temperature was about57.8°C after captures versus about55°C before the closing baseline, despite stable frame means. Counter clocks remain offset1.778–1.789s and device-wide.

### What should be done in the future
Keep bounds opt-in. Broaden motion/seed qualification and run a matched integrated/audio-enabled check before default adoption. Precomputation, lossy distant shadows and contour/glyph changes remain designed but unimplemented; do not conflate them with the measured candidate.

### Code review instructions
Read analysis/01, source helper/test and the exact submitted shaders. Inspect the following actual screenshots and the40-pair visual-comparisons.json. Run06-analyze.py to recompute comparisons from retained pixels, not merely trust that JSON.

![Original and bounds, both full-window views](../scripts/evidence/target-20261014T194947Z/figures/full-window-comparison.png)

![Original shadow retained, versus non-parity omission](../scripts/evidence/target-20261014T194947Z/figures/top-shadow-detail.png)

The comparison panel preserves native1024×768 outputs. The detail uses a3×nearest-neighbor crop; full source rectangle[516,426,566,480] and conversion hashes are retained. These are rendered-window captures, not physical scanout photographs.

### Technical details
Binary f9b7de52377501090392052039c0a13ad18cc258661aa6cb98614165ef9138c3. Implementation e970ec5, source-qualified after build. Default remains original (bounds disabled); all shader inputs are canonical. Raw compressed images and counter evidence total about31MB before final bookkeeping; no binaries/libraries/secrets staged.

## Step 4: Publish the design outcome, derivation and screenshot report

Published the measured candidate as Section18 of the living GPU report. It includes the light-invariant bound derivation, a flow diagram, measured timing/counter tables, both full-window views, enlarged original/bounds/off shadow details and later/x-ray screenshots. Historical Sections1–17 are preserved, with the latest-summary/navigation updated to distinguish this candidate from earlier rejected or non-parity experiments.

Vault commit3306f79b05d158ed45e37305f5835375101bb9d4 was pushed and independently matched to remote main. Native measured checkpointa9a6c9f and implementatione970ec5 remain unpushed. Bounds remains opt-in; precomputation, distance-based LOD and marking changes are design alternatives, not completed implementations.

### Prompt Context
**User prompt:** See Step1.
**Assistant interpretation:** Deliver a readable technical report with actual screenshots and preserve evidence/commit boundaries.
**Commit (measurements):**a9a6c9f — Qualify shadow footprint speedup with repeated device runs and pixel-equal screenshots.

### What I did
Added08-vault-followup.py, copied figures, comparison/qualification/provenance and selected raw screenshots, and appended Section18. Validated old section preservation, image readability, relative links and source-copy hashes. Scoped the vault commit to the report, MOC and new shadow-bounds assets; pushed and verified remote main.

### Why
The broad scene experiment and the narrower successful candidate need separate historical claims. A reader should see exactly what shadow pixels are preserved and what the shadow-off diagnostic would lose.

### What worked
Publication checks passed. The report and diary embed actual target screenshots. The native doctor reported all checks passed. Source/build qualification identifies the measured executable separately from later documentation work.

### What didn't work
Final review found the legacy general DIAGNOSTIC log's non_parity expression did not include shadow-off, although the dedicated SHADOW line and structured receipt already correctly labeled it. Corrected the single general-log boolean after the campaign. Host rebuild and a one-frame off-mode smoke print `non_parity=1`; no shader/math/measurement policy changed. The measured ARM binary remains the recorded e970ec5 artifact and was not silently replaced or relabeled.

### What I learned
The image-preserving candidate removes enough work for a repeatable12.46% mean reduction, but still leaves substantial scene/post cost and frame tails. It does not justify adopting intentionally lossy alternatives or declaring integrated60FPS.

### What was tricky to build
Preserve old report sections byte-for-byte while updating only current metadata/navigation and adding the new chapter. Keep native screenshot comparisons and copied publication assets tied by hashes rather than reconstructing images from descriptions.

### What warrants a second pair of eyes
Finite endpoint qualification, conservative engineering float slack, no moving-image review and no audio-enabled campaign. Report latest_runtime_commit identifies the actually measured implementation, not the later log-only correction.

### What should be done in the future
Broader seed/motion and integrated audio qualification before any default change. The other three designed reductions require their own controlled evidence; they remain explicitly deferred.

### Code review instructions
Read report Section18 and its shadow-bounds assets, publication-validation.json and remote-verification.txt. The screenshot-linked Step3 records the actual image acceptance. Review the final one-line main.cpp log-label fix separately from measured shader/runtime changes.

### Technical details
Measured binary f9b7de52377501090392052039c0a13ad18cc258661aa6cb98614165ef9138c3; measured evidencea9a6c9f. Vault3306f79b05d158ed45e37305f5835375101bb9d4. Production launcher, canonical assets, governor and compositor policy unchanged.
