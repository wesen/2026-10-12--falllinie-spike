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
