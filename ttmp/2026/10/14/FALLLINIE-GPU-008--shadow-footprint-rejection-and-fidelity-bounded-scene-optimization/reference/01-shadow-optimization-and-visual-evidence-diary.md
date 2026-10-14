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
