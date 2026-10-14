---
Title: Shader sensitivity investigation diary
Ticket: FALLLINIE-GPU-007
Status: active
Topics: [performance]
DocType: reference
Intent: long-term
Summary: Explicit non-parity scene/post experiments using the unchanged GPU006 runtime and replay.
---

## Step 1: Define the two-factor experiment and expose a real shader contract

Created a separate ticket for the next experiment rather than extending GPU006's completed resolution matrix. The design uses the same qualified binary and generated-input replay, with baseline, cheap-post, flat-scene and combined asset packages. The intended eight-case symmetric order repeats each mode and brackets the sequence with the baseline. Production assets, display policy and runtime source remain unchanged.

The first short host feature check linked the cheap-post program but then failed the renderer's required-uniform validation: `fatal: missing print uniform uD`. A one-color-sample replacement had correctly optimized away the depth sampler. This is an actual integration contract, not a shader syntax failure. Preserve the failed shader dump/log and adjust the diagnostic to a meaningful depth-aware grayscale copy instead of weakening the renderer or adding a dummy dependency.

### Prompt Context
**User prompt (verbatim):** "continue"
**User prompt (verbatim):** "commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)"
**Assistant interpretation:** Continue the proposed controlled post/scene sensitivity experiments, commit at coherent implementation/measurement milestones, and preserve full investigation reasoning and failures.
**Inferred user intent:** Obtain stronger causal direction for GPU optimization without pretending diagnostic image changes are production improvements.
**Prior measured checkpoint:**6c5b231; reporting checkpoint0a0d849.

### What I did
Read actual scene/post fragments, renderer adaptation and required uniforms. Created the ticket/design/tasks/diary. Added01-prepare.py for18hashed asset files and02-host-check.py for four short replay/linking checks, shader dumps and endpoint comparisons. The initial attempt completed baseline then failed post-copy at initialization.

### Why
At512×384 dual-view the replay is clearly slower than the lighter cases. Changing only one shader stage at a time can expose sensitivity, but cannot measure independent pass milliseconds. Retain FBOs, draw structure, geometry, material encoding and snow-circle discard; isolate assetselection using the existing --assets override instead of changing replay's prohibited post-bypass combination.

### What worked
All shader programs linked for the attempted post-copy case. The generator preserves canonical shaders and includes the original HUD/glyph atlases at the required parent path. Baseline replay ran successfully. Exact submitted sources were retained.

### What didn't work
`python3 <ticket>/scripts/02-host-check.py` stopped with subprocess exit1 for post-copy after the log reported `program original print linked`, followed by `fatal: missing print uniform uD`. Renderer::initialize requires active uC,uD,uRes. The initial single-read post substitute leaves uD unused and the driver removes it. Failure evidence is retained before retry.

### What I learned
Runtime shader adaptation checks are not the only compatibility requirement; the linked program must expose required active uniforms. Meaningfully retaining a center-depth read is a cleaner diagnostic than relaxing a production guard or disguising an inactive uniform as evidence of work.

### What was tricky to build
The adapter requires original sky/depth-source anchors. The diagnostic inserts an unconditional early result/return but retains the original unreachable main-body text, allowing the adapter to recognize source anchors while the compiler eliminates the original work. Scene simplification also changes varying/vertex linkage and downstream image processing; those are part of this intervention, not isolated fragment time.

### What warrants a second pair of eyes
All nonbaseline images intentionally change. Shader-content changes can affect compression/cache/compositor work. Depth BMP equality is a bounded depth-diagnostic comparison, not proof of bitwise24-bitdepth equivalence in every frame. A near60/s result may hit the known presentation ceiling.

### What should be done in the future
Use a depth-aware grayscale substitute, rerun the bounded functional gate, commit the implementation/tooling milestone, then collect the actual symmetric device matrix. Do not extrapolate host timings to Mali.

### Code review instructions
Start with design-doc/01 and scripts/01-prepare.py. Inspect actual submitted shader dumps and failed game.log rather than guessing from the external asset text alone. The target will use unchanged binarySHA90f124a7745a37f76175130bd79ef748b741852bccfe58305548f7946c0d82ed.

### Technical details
Planned seed42,600frames,32terrain row units,dual512×384,fps0,noaudio,ordinarydraws,queryoff,pairedcadence60 and45second hardware-counter captures. No new runtimebinary, canonical shader edit, processpolicy change or delegation.

## Step 2: Complete the diagnostic packages and the bounded functional gate

Changed cheap post to select white sky from center depth or grayscale scene color for geometry. This retains the required active uD sampler meaningfully while omitting original print/sky/neighborhood work. The four short host cases now link and complete with matching selected-state signature13644297744808369648. All depth diagnostics match, post-only changes preserve the encoded scene, and every nonbaseline final print differs as intended.

The implementation milestone includes all18asset hashes, explicit per-case non-parity metadata, before/after target asset verification, key-only deployment, the symmetric eight-case runner and a counter/CPU/image analyzer. The game binary remains the previously qualified GPU006 executable. Actual Mali measurements are the next gate.

### Prompt Context
**User prompts:** SeeStep1.
**Assistant interpretation:** Finish one coherent diagnostic feature and validate actual shader integration, then measure rather than expanding precautionary tests.

### What I did
Retained the failed first attempt under evidence/host-first-attempt, including submitted shaders. Regenerated packages and ran02-host-check.py: four60-frame dual-view256×192cases. Added03-target-case.sh,04-campaign.sh and05-analyze.py. The analyzer checks modes/argv, deployedbinary, assetverification,600-frame sequences, completepairedrecords, countersequences, signatures and expected image relationships.

### Why
The runtime's active-uniform guard should remain intact. The substitute should actually read depth rather than rely on an optimizer-resistant artificial use. Explicit external diagnosticmetadata and assethashes are necessary because the unchanged application's source receipt does not identify custom assetcontents.

### What worked
Hostfour-case gate passes with GL_error0 and audio_disabled. Original source and canonical shaders remain untouched. Depth/scene relationships and deliberate printdifferences all match the contract. The previous one-read failure is preserved, not silently replaced by a successful log.

### What didn't work
No further host failure after the depth-aware correction. No device performance claim yet. Initial staging stopped on raw game.log line5 (`trailing whitespace`) because the GL extension list ends with a space. Preserved the raw logs and used a command-local whitespace exception only for log files; source/docs retain normal checks. This stopped before commit/deployment and did not run a partial target campaign.

### What I learned
Minimal integration testing caught a required-uniform mismatch that source-level shader reasoning missed. The fix changes the diagnostic definition from a one-read copy to center-depth plus conditional scene-color sampling; this distinction is explicit in the design.

### What was tricky to build
Meaningful image equality is mode-specific: requiring everyprint to match would reject the purpose of this non-parity experiment, while allowing repeatedsame-mode mismatches would undermine reproducibility. Scene-only simplification preserves material IDs and snowdiscard but still changes downstream postcontent and potentially vertex/varying linkage.

### What warrants a second pair of eyes
The shader bodies retain unreachable original text solely for adaptation compatibility; submittedshader dumps show the actual result. GLSL early-return semantics remove those paths, but this does not measure how the installed driver schedules the remaining stages. Counterwindow and compositor limitations remain.

### What should be done in the future
Commit this milestone, then run the symmetric baseline/post/scene/both/both/scene/post/baseline campaign on the actual handheld. Interpret both individual runs and grouped distributions; do not promote any diagnosticshader.

### Code review instructions
Read01-prepare.py and inputs/manifest.json, compare actual host submittedsources with canonical files, then inspect the target runner and05-analyze.py. The unchanged executable is verified bySHA256 before collection; targetshader assets are verified before andafter.

### Technical details
Four hostcases ×60frames; all share one partialtrajectorysignature. Targetplan remains512×384dual,600frames/case,45scountercaptures. No runtime rebuild or broadCPUtest suite is repeated because runtimebytes are unchanged.
