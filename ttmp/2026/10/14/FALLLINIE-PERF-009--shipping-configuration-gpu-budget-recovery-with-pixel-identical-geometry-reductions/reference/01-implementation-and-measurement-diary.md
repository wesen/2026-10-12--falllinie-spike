---
Title: Implementation and measurement diary
Ticket: FALLLINIE-PERF-009
Status: active
Topics:
    - performance
    - graphics
    - handheld
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Detailed diary for PERF-009: shipping-configuration shadow-bounds measurement, geometry counters, pixel-identical geometry reductions and presentation-acquire scope."
LastUpdated: 2026-10-14T18:06:41.484562526-04:00
WhatFor: ""
WhenToUse: ""
---

## Step 1: Design the shipping-configuration budget recovery

This step turns the vault performance review's plan into a ticket and a design document before any code changes. The design fixes the workload (GPU-006 replay at the shipping 384×288 dual-view size), the metric (long-frame share and missed refreshes per frame, not only the mean), the promotion rule (byte-identical images at five checkpoints, unchanged replay signature, no timing regression), and four phases: shadow bounds at the shipping size with and without audio, a geometry counter set, three pixel-identical geometry reductions, and a presentation-acquire scope.

Reading the source to write the design corrected one assumption from the review: the receipt's `vertices`/`indices` count only terrain cell meshes (`Renderer::update_player` sums `cell_meshes_` over the seven-cell patch), so object geometry is additional. It also confirmed the projection path: `vW` is absolute world position while `gl_Position` uses `w.xyz-uRenderOrigin`, which matters for both fog distance and frustum tests.

### Prompt Context
**User prompt (verbatim):** "Ok, create a new design doc for this and then implement, commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)"
**Assistant interpretation:** Create a docmgr ticket with a design document covering the plan I proposed (shadow bounds at the shipping size, geometry counters, pixel-identical geometry reductions, presentation instrumentation), then implement and measure it on the handheld, committing in focused steps and keeping an investigation-format diary.
**Inferred user intent:** Turn the review's recommendations into measured, default-ready improvements toward 60 FPS without fidelity loss, with the same evidence standard as earlier tickets but less overhead.

### What I did
- Created ticket `FALLLINIE-PERF-009` with `docmgr ticket create-ticket`, a design doc and this diary with `docmgr doc add`, and nine tasks.
- Read `src/renderer.cpp` (`initialize`, `render`, `upload_cell`, `draw_indexed`, `capture`), `src/game_renderer.cpp` (`update_player` streaming window, `render_first_person`), `src/shadow_bounds.hpp`, `src/math.hpp` (`render_frame`), the replay/audio handling in `src/main.cpp`, the PERF-004 counter probe and the GPU-008 campaign scripts.
- Listed the pinned Mali-G52 counter names from `.work/libGPUCounters/docs/mali-g52_counter_reference.html` to choose the geometry set.
- Checked device reachability: `ssh root@192.168.0.202 uname -a` → `Linux RK3566 7.2.7 #1 SMP PREEMPT ... aarch64`; governor `ondemand`; no Falllinie process running.

### Why
- The review identified that the best existing candidate was never measured at the shipping size and that geometry was never examined; measuring first prevents implementing changes that are not needed.
- Opt-in flags with a written promotion rule avoid the earlier pattern where successful candidates stayed opt-in indefinitely.

### What worked
- The GPU-008 scripts (`01-build.sh`, `03-target-case.sh`, `04-capture-case.sh`, `05-campaign.py`, `capture_tools.py`) are directly reusable with a render-size parameter.
- The pinned libGPUCounters checkout exposes all geometry counters needed (`MaliGeomTotalPrim`, `MaliGeomVisiblePrim`, `MaliGeomSampleCullPrim`, `MaliFragRastQd`, `MaliTilerVarShadThread`, ...).

### What didn't work
- No failures in this step.

### What I learned
- The kernel identifies the SoC as RK3566 (`uname -a`), while earlier reports say RK3568. Both have a single-core Mali-G52 and Cortex-A55 cores; the GPU identity used in the analysis (G52 r1p0, one core) comes from the counter library, so no conclusion changes.
- Replay forces `no_audio=true` (`src/main.cpp`), so an audio-on comparison needs a new flag rather than an existing option.
- The receipt's vertex/index counts cover terrain cells only.

### What was tricky to build
- Defining the fog-constant condition required following which eye is uploaded as `uCam`: `render()` uploads `eye=camera_.eye+camera_shake_` in absolute world coordinates, and the fragment shader computes `dist=length(vW-uCam)` in absolute coordinates. The CPU test must use exactly that point, not the render-origin-relative `frame.eye`.

### What warrants a second pair of eyes
- The argument that `mix(lum,1.,1.)` stores 255 in an RGBA8 target regardless of implementation rounding; the visual checkpoints are the practical check.
- Whether a uniform early return in the scene shader changes Mali code generation for the non-flagged path in a way that costs time.

### What should be done in the future
- N/A for this step.

### Code review instructions
- Read `design-doc/01-shipping-configuration-gpu-budget-recovery-design.md`, Sections 3–8.

### Technical details
- Budget: 800 MHz × 16.667 ms = 13.33 M GPU cycles per loop frame.
- Terrain cell: 139 rows × 107 columns = 14,873 vertices; 29,256 triangles; resident window `cell-1 .. cell+5`.

## Step 2: Implement opt-in geometry policies, replay audio and the acquire scope

This step added every runtime switch the design needs, all defaulting to the existing behaviour: `--replay-audio`, `--fog-cells constant`, `--view-culling on`, `--short-indices on`, `--output-clear on` and `--window-depth 0`. The host xvfb replay at 384×288 dual view keeps the replay signature and produces byte-identical captures for every policy and for all of them combined with shadow bounds.

The host draw statistics changed the expected value of one policy before any device run. Per-cell frustum culling removes only 21 of 3,100 terrain draws and 350 of 3,100 object draws (about 1% of triangles), because every terrain cell is 600 m wide (lateral columns reach ±300 m) and so almost always intersects both frusta. Fog-constant shading applies to 748 terrain draws and 802 object draws, about a quarter. Objects are small: 195,000 triangles per view in total against about 189,000 terrain triangles.

### Prompt Context
**User prompt (verbatim):** see Step 1.
**Assistant interpretation:** Implement Phase A–D runtime switches behind flags, with tests and a host identity check.
**Inferred user intent:** Make every candidate measurable on the device with one binary.
**Commit (code):** a7626fd — "Add opt-in fog-constant shading, per-view culling, 16-bit indices, output-clear acquire scope and replay audio"

### What I did
- `src/geometry_bounds.hpp` (new): `Aabb`, `clip_rejects` (all eight corners outside one clip half-space with relative slack), `box_distance`, `fully_fogged` (distance > 400 + 2 m), `fits_short_indices`, and `adapt_fog_constant_shader`, which refuses to adapt unless the original fog tail is present verbatim and inserts `if(uFogConstant!=0){ o=vec4(1.,0.,uMat/255.,0.); return; }` at the start of `main`.
- `src/renderer.hpp/.cpp`: `CellMesh` stores `bounds` and `index_type`; `upload_cell(mesh, allow_short)` narrows indices for static meshes; dynamic meshes and the static fixture stay 32-bit; `draw_indexed` takes the index type; the scene pass draws terrain and objects through a `draw_static` lambda that applies culling and the fog flag and counts `DrawStats`; the uniform is reset before trail, rider and particles; `scene_fog_near/far` constants feed both the uniform upload and the CPU test; the output pass opens a `RenderPass::Acquire` scope around `glClear` when `--output-clear on`.
- `src/profiler.*`: `RenderPass::Acquire`, named `acquire` with parent `output`.
- `src/main.cpp`: flags, help text, `--replay-audio` (requires replay; forces muted audio), `SDL_GL_DEPTH_SIZE` from `--window-depth`, a `GEOMETRY ...` log line, and receipt fields including `geometry_draws`, snapshotted before the capture redraws.
- `tests/test_geometry_bounds.cpp` and its CMake registration.
- Ticket scripts: `01-build.sh` (from GPU-008, binary `falllinie-perf009`), `02-host-check.py`, `03-counters/` (probe copy with `base|geometry` sets), `04-target-case.sh`, `05-capture-case.sh`, `06-campaign.py` (plans `phase-a..d`, governor set/restore), `07-analyze.py`.

### Why
- One binary with explicit switches keeps every comparison an ABBA run of the same executable.
- The fog-constant adaptation is anchored on the exact original tail so that a future shader edit cannot silently invalidate the equivalence argument.

### What worked
- Host build without warnings after two small fixes; `ctest`: 17/17 passed (`100% tests passed, 0 tests failed out of 17`).
- `test-geometry-bounds`: `boxes=24000 rejected=19460 rejected_points=7784000 fogged_boxes=12917 fog_points=826688 OK`. Every sampled point of a rejected box lies outside the same clip plane in float evaluation; every point of a fog-flagged box is at float distance ≥ 400, where GLSL `smoothstep` returns exactly 1 and both legal `mix` formulas quantize to 255.
- `02-host-check.py 240`: all five candidate configurations have 0 different pixels against `original` across output, print, scene and depth for both views; one signature `8964042839430826299` (240 frames).
- Replay audio on host with `SDL_AUDIODRIVER=dummy`: same signature with and without `--replay-audio`; `AUDIO ... producer_blocks=667 ... starved_callbacks=0`.
- ARM build: binary `b3e8fc8302d6761d70cc7abd6fd58a9eda75f33b12c752ec636ed6ffc89ad5c8`; probe `508e806007d7d84c16ba57164a37b673d0f6707f8d500f8b22af2792bfabe9f1`.

### What didn't work
- First build produced `warning: this ‘if’ clause does not guard... [-Wmisleading-indentation]` in `geometry_bounds.hpp:50` and the test, and `-Wrange-loop-construct` in the test; fixed by splitting statements and binding by reference.
- Testing the new analyzer against GPU-008 evidence overwrote `GPU-008/.../target-20261014T194947Z/summary.json`; restored with `git checkout` and added an explicit output-path argument. The analyzer reproduced the published GPU-008 pooled values (23.575 / 20.636 / 20.024 ms; 18.94 / 16.59 / 16.09 M GPU-active cycles per frame), which validates it.

### What I learned
- Per-cell culling is too coarse for this terrain. Cells span 600 m laterally, so a useful culling policy needs smaller chunks.
- Objects contribute about 3% of scene triangles; terrain dominates geometry.

### What was tricky to build
- Draw statistics must not include the post-loop capture redraws, so `main` snapshots `draw_stats_json()` before the capture block.
- The fog uniform must be reset before dynamic meshes; otherwise a flagged last object cell would make the rider or particles constant. The lambda tracks the current value and the reset runs after the object loop.

### What warrants a second pair of eyes
- `clip_rejects` slack (`1e-4` relative) versus the GPU's float transform; the CPU test checks float evaluation of the same expression, not Mali's exact instruction sequence.
- The early return precedes `discard` for particles; particles are never flagged, which the reset guarantees.

### What should be done in the future
- If geometry turns out to matter, chunk cells into smaller row/column blocks so that culling and fog flags apply at finer granularity.

### Code review instructions
- Start with `src/geometry_bounds.hpp`, then `Renderer::render` (the `draw_static` lambda and the acquire scope) and `Renderer::upload_cell`.
- Validate: `cmake --build build/integrated-source && ctest --test-dir build/integrated-source`; `python3 scripts/02-host-check.py 240` from the ticket.

### Technical details
- Host draw statistics over 240 frames (480 views), `combined`: terrain drawn 3,079 / culled 21 / fog-constant 748; objects drawn 2,750 / culled 350 / fog-constant 802; 92,673,024 triangles submitted, 933,576 culled.

## Step 3: Phase A — shadow bounds at the shipping configuration

This was the first device measurement of the ticket and the one the review singled out as most valuable. At 384×288 dual view, shadow bounds removed every long frame in the audio-off replay (long-frame share 0.039 → 0.000) and almost all of them with the muted audio producer running (0.065 → 0.005). The pooled mean fell from 17.273 to 16.620 ms without audio and from 17.697 to 16.688 ms with audio. The replay signature was identical in all eight cases.

The counters also corrected a weakness in the review's cycle model. Bounds runs still show about 13.3 M GPU-active cycles per loop frame, i.e. about 800 M cycles per second: the GPU remains busy essentially all the time even at 60 loop frames per second. When the GPU is saturated, "GPU-active cycles per frame" is just 800 M divided by the frame rate, so it cannot measure how much work a change removed. The prediction was right in outcome (under budget) but the headroom is close to zero.

### Prompt Context
**User prompt (verbatim):** see Step 1.
**Assistant interpretation:** Run the Phase A campaign and decide whether shadow bounds reach the 60 Hz budget at the shipping size.
**Inferred user intent:** Obtain the decision-relevant measurement before building more.
**Commit (code):** a7626fd (measured binary `b3e8fc83…ad5c8`)

### What I did
- `python3 06-campaign.py phase-a` → `evidence/phase-a-20261014T221635Z`, eight timed cases at 384×288 dual view in order original, bounds, bounds, original, original+audio, bounds+audio, bounds+audio, original+audio. Governor recorded `ondemand`, set `performance` (all cases started at 1,992,000 kHz), restored to `ondemand` in the `finally` path.
- `python3 -I 07-analyze.py evidence/phase-a-20261014T221635Z`.

### Why
- The shipping configuration is the acceptance configuration; the earlier 512×384 measurement could not answer whether bounds reach 60 Hz.
- Running audio-off and audio-on in one session with one binary isolates the producer's effect.

### What worked

| Case | Mean ms | p95 ms | p99 ms | Long share | Missed refreshes/frame |
|---|---:|---:|---:|---:|---:|
| original (pooled 01, 04) | 17.273 | 20.399 | 29.875 | 0.039 | 0.036 |
| bounds (pooled 02, 03) | 16.620 | 17.611 | 18.251 | 0.000 | −0.003 |
| original + audio (05, 08) | 17.697 | 28.424 | 29.903 | 0.065 | 0.062 |
| bounds + audio (06, 07) | 16.688 | 17.901 | 20.399 | 0.005 | 0.001 |

- All cases: `GL_error=0`, `game_status=0 probe_status=0`, counter probe `final rows=5850 errors=0 nonfinite=0 stop=ok`, signature `6081555844518115723`.
- Audio cases: `starved_callbacks=0 missing_frames=0`; producer CPU mean 9.66–10.06 ms per block; `producer_over_budget` 0–2 per case.
- The opening/closing original pairs agree closely (17.243 / 17.304 ms; 17.656 / 17.738 ms), so drift is small relative to the effect.

### What didn't work
- No failures. The negative "−0.003 missed refreshes" values are measurement noise around exactly 60 loop frames per second (mean 16.620 ms is slightly below 16.667 ms because the replay loop is not locked to an exact 60.000 Hz).

### What I learned
- The audio producer is a real cost: it raises the original's long-frame share from 0.039 to 0.065 in the same session. With bounds, the remaining 0.5% long frames appear only when audio runs.
- GPU-active cycles stay at ≈800 M/s in every case, including bounds at 60 FPS. The GPU has almost no idle time left, so any additional scene content or a slower trajectory segment could push frames over again. Further GPU savings are still needed for margin, not just for the mean.
- Where the wait appears flips between runs: in `02-bounds` the top output scope waits 9.73 ms while the lower output waits 0.46 ms; in `03-bounds` it is 0.99 and 9.43 ms. This is consistent with the loop waiting for whichever window's buffer is released next, with a session-dependent phase between the two panels.

### What was tricky to build
- The campaign must restore the governor even if a case fails; `06-campaign.py` does it in a `finally` block and records before/during/after values in `campaign.json`.

### What warrants a second pair of eyes
- Whether the replay audio load represents the live game's audio load: the producer runs at the same rate and settings (1,024-frame buffer, muted, vinyl on) but follows the replay's fixed-step trajectory.

### What should be done in the future
- Promote shadow bounds after the five-checkpoint visual qualification at 384×288 (Phase C plan includes `bounds` in its visual set).
- Find more GPU headroom; Phase B decides whether geometry is the place.

### Code review instructions
- `scripts/06-campaign.py` (`PLANS['phase-a']`, governor handling) and `scripts/07-analyze.py`.
- Reproduce: `python3 -I scripts/07-analyze.py scripts/evidence/phase-a-20261014T221635Z`.

### Technical details
- GPU-active per loop frame (estimate): original 13.87–13.91 M; bounds 13.28–13.29 M; original+audio 14.18–14.24 M; bounds+audio 13.31–13.32 M. All are ≈ 800 M/s ÷ loop FPS.

## Step 4: Phase B — geometry counters, and row-block chunks

Phase B ran the shipping replay with a geometry counter set: original, cell-level candidate, candidate, original. It answered the question the review left open: geometry is a large cost, and most of it is triangles that never produce a fragment. Per loop frame (two views) the GPU receives about 425,000 primitives; only 66,000 are visible. 252,000 (59%) are culled because they cover no sample point, 85,000 (20%) fall outside the near/far planes, and 22,000 are culled by facing or the XY frustum. Position shading runs 447,000 threads per frame while varying shading runs only 77,000. Non-fragment activity is about 7.2 M cycles per frame, half of GPU-active time.

The cell-level candidate from Step 2 (per-cell culling, fog-constant cells, 16-bit indices) cut external reads by 16% per frame (25.76 → 21.63 MB) and non-fragment activity by 6% (7.21 → 6.75 M cycles), but frame timing barely moved (long share 0.044 → 0.041). Because cells are 600 m wide, per-cell culling only removed 6% of primitives. While Phase B ran I implemented row-block chunks, which on the host raise culled triangles from 1% to 9.6% and fog-constant chunks to 27% of chunk draws, still byte-identical.

### Prompt Context
**User prompt (verbatim):** see Step 1.
**Assistant interpretation:** Measure whether geometry matters before investing in geometry changes; refine the candidate if it does.
**Inferred user intent:** Spend effort where the counters show cost.
**Commit (code):** pending in this step — "Draw static meshes as row-block chunks for culling and fog-constant decisions"

### What I did
- `python3 06-campaign.py phase-b` → `evidence/phase-b-20261014T222320Z` (probe set `geometry`; binary from `a7626fd`). Note: the previous commit `6b6c3c2` captured this directory's early `campaign.json` (`complete: false`) while the campaign was still running; this step commits the completed directory.
- `07-analyze.py` on the campaign.
- `src/geometry_bounds.hpp`: `IndexChunk` and `index_chunks(vertices, indices, chunk_indices)`, consecutive index ranges with the AABB of referenced vertices.
- `src/renderer.cpp`: when culling or fog-constant is enabled, static meshes get chunks of 16 terrain row strips (636 indices per strip, ≈12.8 m of slope). `draw_static` culls and fog-tests each chunk and merges consecutive surviving chunks with the same fog state into one `glDrawElements` with an index-buffer offset, preserving draw order. New stats `chunks_drawn/culled/fog_constant`.
- `tests/test_geometry_bounds.cpp`: chunks partition the index buffer in order and bound every referenced vertex.

### Why
- Terrain indices are emitted row by row (`TerrainJob::step`, `mesh_.indices.insert(...{a,c,b,b,c,d})` per column within each row), so a row block is a contiguous index range and can be skipped without reordering triangles.

### What worked

Per loop frame, pooled (estimates: whole-device rate ÷ loop FPS):

| Counter | Original | Cell-level candidate |
|---|---:|---:|
| `MaliGeomTotalPrim` | 424,955 | 399,862 |
| `MaliGeomVisiblePrim` | 66,328 | 66,251 |
| `MaliGeomSampleCullPrim` | 251,571 | 251,274 |
| `MaliGeomZPlaneCullPrim` | 84,813 | 60,128 |
| `MaliGeomFaceXYPlaneCullPrim` | 22,243 | 22,208 |
| `MaliTilerPosShadThread` | 447,477 | 419,713 |
| `MaliTilerVarShadThread` | 76,585 | 76,488 |
| `MaliFragRastQd` | 682,886 | 680,900 |
| `MaliFragThread` | 2,543,976 | 2,541,631 |
| `MaliNonFragActiveCy` | 7.21 M | 6.75 M |
| `MaliGPUActiveCy` | 13.95 M | 13.90 M |
| `MaliExtBusRdBy` | 25.76 MB | 21.63 MB |
| `MaliExtBusWrBy` | 13.37 MB | 13.35 MB |

- Frame timing: original mean 17.358 ms, long share 0.044; candidate 17.301 ms, 0.041. All four cases `GL_error=0`, probe `errors=0`, signature `6081555844518115723`.
- Host check with chunks (240 frames, 480 views): `cull` and `combined` culled 2,363 chunks and 8,948,872 of 93,606,600 triangles; `fog` flagged 8,502 of 31,000 chunk draws; all configurations 0 different pixels.

### What didn't work
- First attempt to apply the chunk edit failed before touching files: `SyntaxError: unterminated string literal (detected at line 58)` in my inline Python editing script (an unescaped quote in a replacement string). Rewrote the script with raw strings; all assertions on anchor text passed.

### What I learned
- 59% of submitted primitives are sub-sample triangles. They are discarded before fragment shading but still cost index fetch, position shading and tiler work. No pixel-identical CPU-side change can remove them cheaply: identifying them needs per-frame projection, and a coarser far mesh changes depth and the sky/terrain silhouette that the post pass reads. Distance level of detail is therefore an owner decision.
- Fragment threads per frame (2.54 M) are dominated by the two 1024×768 output copies (1.57 M threads); the scene and post passes at 384×288 together are under 1 M threads.
- 16-bit indices and culling reduce memory reads clearly (−16%) but not GPU-active time; with the GPU saturated, bytes saved do not translate one-for-one into time.

### What was tricky to build
- Merging runs must break on both a fog-state change and a gap left by a culled chunk; otherwise a draw would span culled indices or use the wrong uniform. The loop flushes the current run when the next surviving chunk is not adjacent or has a different fog flag.

### What warrants a second pair of eyes
- The counter normalisation by loop FPS uses a counter window narrower than the frame window (as in earlier tickets).
- Whether `MaliGeomSampleCullPrim` on this driver counts only triangles with zero sample coverage; the pinned G52 reference describes it as primitives culled by the sample-coverage test.

### What should be done in the future
- Measure the chunked candidate in Phase C, including its CPU cost (more draws per view).
- Prepare a distance-LOD image comparison for the owner, since sub-sample triangles are the largest geometry cost.

### Code review instructions
- `src/geometry_bounds.hpp` (`index_chunks`), `Renderer::render` (`draw_static` chunk loop), `Renderer::upload_cell`.
- `python3 -I scripts/07-analyze.py scripts/evidence/phase-b-20261014T222320Z`.

### Technical details
- Terrain cell: 107 columns per row (`columns_` in `TerrainJob`), 636 indices per row strip, 139 rows; chunk = 16 strips = 10,176 indices.

## Step 5: Phase C — Mali visual qualification, timing, and default promotion

Phase C ran the chunked binary on the device. Every candidate and their combination produced byte-identical images against the original at all five checkpoints at 384×288: 25 comparisons of eight images each (full top and lower windows plus both cameras' scene, print and depth targets). Timing at 512×384 dual view, with shadow bounds as the baseline, split the candidates clearly: per-view chunk culling helps (−1.6% mean, long-frame share 0.238 → 0.219), 16-bit indices help a little (−0.7%), and the fog-constant early-out hurts (+0.7%) in both of its repetitions.

Following the design's promotion rule, shadow bounds, view culling and 16-bit indices are now the defaults. Fog-constant shading stays opt-in and is recorded as rejected: the uniform branch appears to cost every scene fragment more than it saves on the few fully fogged ones.

### Prompt Context
**User prompt (verbatim):** see Step 1.
**Assistant interpretation:** Qualify candidates on Mali, measure them, and promote the ones that pass the written rule.
**Inferred user intent:** Ship improvements instead of leaving them opt-in.
**Commit (code):** pending in this step — "Promote shadow bounds, view culling and 16-bit indices to defaults"

### What I did
- Built `aab2163` for ARM (`00e2cc09…e400a2`) and ran `06-campaign.py phase-c` → `evidence/phase-c-20261014T222848Z`. Visual set: `fog`, `cull`, `short`, `bounds`, `all` (= bounds + fog + cull + short). Timed: bounds, fog, short, cull, geometry, geometry, cull, short, fog, bounds, all with bounds.
- Deleted PNGs from all visual directories except `f0600-s42-p0-original` and `f0600-s42-p0-all` to keep evidence bounded (35 MB → 10 MB); `lossless-conversion.json` keeps every image's BMP/PNG/RGBA hash and `visual-comparisons.json` keeps every comparison.
- `src/main.cpp`: default `shadow_mode=Bounds`, `view_culling=true`, `short_indices=true`; help text updated. `--shadow-mode original --view-culling off --short-indices off` restores the old behaviour.
- Scripts: `ORIGINAL`/`NOGEOM`/`DEFAULTS` flag sets in `06-campaign.py` and `02-host-check.py`, because an empty flag list now means the new defaults; `08-live-case.sh` and an `acceptance` plan (replay with audio, then live autoplay with audio).

### Why
- Every promoted change has three pieces of evidence: an argument that it cannot change pixels, host identity, and Mali identity at five checkpoints; and each is faster in an ABBA campaign.
- Fog-constant fails the timing condition of the rule.

### What worked
- `visual-comparisons.json`: 0 different pixels for every candidate at every checkpoint.

| Mode (512×384 dual, bounds baseline) | Mean ms | p95 ms | Long share | Non-frag Mcy/frame | Reads MB/frame |
|---|---:|---:|---:|---:|---:|
| bounds (01, 10) | 20.571 | 29.460 | 0.238 | 7.84 / 7.81 | 27.3 / 27.4 |
| + fog constant (02, 09) | 20.712 | 29.566 | 0.249 | 7.86 / 7.90 | 26.7 |
| + short indices (03, 08) | 20.425 | 29.563 | 0.229 | 7.65 / 7.64 | 24.8 |
| + chunk culling (04, 07) | 20.236 | 29.526 | 0.219 | 7.08 / 7.07 | 25.2 |
| + all three (05, 06) | 20.297 | 29.512 | 0.220 | 6.95 / 6.91 | 22.2 |

- Repeats agree: bounds 20.559/20.583, cull 20.252/20.221, short 20.435/20.415, fog 20.712/20.712. Signature `6081555844518115723` everywhere.
- Culling removes 27,450 triangles per view on average (`triangles_culled / views`).

### What didn't work
- Fog-constant shading is slower. CPU scope times do not explain it (top scene CPU ≈2.1–2.4 ms in both), so the cost is on the GPU. The most likely cause is that the uniform branch changes register allocation or scheduling for the whole scene shader. This was not confirmed with the offline compiler.

### What I learned
- Culling helps through fewer position-shaded vertices and tiler work (non-fragment cycles −0.77 M per frame), which matches Phase B's finding that geometry is half of GPU-active time.
- Bytes saved by 16-bit indices (−2.5 MB/frame) give a small but repeatable time gain.

### What was tricky to build
- Changing defaults silently changes the meaning of every existing "no flags" baseline in the scripts. The plans now name the original behaviour explicitly, and the campaign docstring notes which binaries earlier phases ran on.

### What warrants a second pair of eyes
- The combined `geometry` result (20.297) is slightly slower than culling alone (20.236) because it includes fog-constant. Culling plus short indices without fog was not measured as a pair; the acceptance campaign measures the promoted combination.

### What should be done in the future
- Run `malioc` on the fog-constant scene shader to confirm the code-generation hypothesis, or remove the option.
- Acceptance with audio and live autoplay (next step).

### Code review instructions
- `src/main.cpp` defaults; `scripts/06-campaign.py` plan definitions.
- `python3 -I scripts/07-analyze.py scripts/evidence/phase-c-20261014T222848Z`; `scripts/evidence/phase-c-20261014T222848Z/visual-comparisons.json`.

### Technical details
- Visual checkpoints: frames 120/600/1200 seed 42 palette 0; frame 600 seed 7; frame 600 palette 5; 384×288 dual view; captures taken after the measured loop.

## Step 6: Phase D — presentation acquisition measured directly; acceptance with audio and live gameplay

Phase D answered the presentation question that the review could only infer. With `--output-clear on`, the first operation on each window's default framebuffer is an explicit clear inside its own `acquire` scope. At the shipping size, sampling every frame, one window's `acquire` scope takes 9.2–9.5 ms of wall time with only 0.28–0.40 ms of thread CPU time, and the rest of that window's output scope is under 0.6 ms. The loop waits for the window system to hand back a buffer, at the first default-framebuffer operation, not in `SDL_GL_SwapWindow`. Which window waits flips between sessions (top in `01-clear-ship`, lower in `02-clear-ship`), consistent with a session-dependent phase between the two panels' refresh cycles.

The acceptance campaign compared the old behaviour with the new defaults in two workloads with the muted audio producer running: the shipping replay and live autoplay with launcher-like settings (`--autoplay --dual-screen --render-size 384x288 --no-vsync --fps 60 --audio-buffer 1024 --no-vinyl`). In live gameplay the long-frame share fell from 7.6% to 0.1% and p95 from 27.8 ms to 17.2 ms. Window depth 0 had no measurable effect and is not promoted.

### Prompt Context
**User prompt (verbatim):** see Step 1.
**Assistant interpretation:** Instrument the presentation wait, test window depth, and run an acceptance comparison of the promoted defaults including audio and live play.
**Inferred user intent:** Confirm the defaults improve the real game, not only the replay benchmark.
**Commit (code):** f1c44a7 (device binary `97d5af04…40733`, identical to the pre-commit build of the same sources)

### What I did
- `06-campaign.py phase-d` → `evidence/phase-d-20261014T224833Z`: visual identity for `defaults`, `clear`, `nodepth` against explicit original flags at five checkpoints; timed `clear` twice at 384×288 with `--profile-cadence 1`; defaults vs `--window-depth 0` at 512×384, ABBA.
- `06-campaign.py acceptance` → `evidence/acceptance-20261014T225930Z`: replay+audio original/defaults/defaults/original, then live original/defaults/defaults/original.
- Compressed the two cadence-1 paired logs with `gzip -9` (4 MB each); `07-analyze.py` reads `.gz` transparently and regenerates a byte-identical `summary.json`.
- Kept PNGs only for `visual-f0600-s42-p0-original` and `-defaults`.
- Ran Mali Offline Compiler 2026.5 on the bounds and bounds+fog-constant scene fragment shaders dumped from the host binary (`evidence/malioc/`).

### Why
- The clear does not change the image (the copy overwrites every pixel) and moves the acquisition wait into a scope of its own, which makes the presentation hypothesis directly testable.
- Live autoplay is closer to the shipped game than the replay: wall-time streaming, real timing-dependent trajectory, launcher settings.

### What worked
- Visual: 0 different pixels for `defaults`, `clear` and `nodepth` at all five checkpoints (15 comparisons × 8 images).
- Acquire scope (n = 540 per view per run):

| Run | Top acquire wall / CPU ms | Top output wall ms | Lower acquire wall / CPU ms | Lower output wall ms |
|---|---|---:|---|---:|
| 01-clear-ship | 9.528 / 0.400 | 10.025 | 0.226 / 0.183 | 0.629 |
| 02-clear-ship | 0.404 / 0.339 | 1.064 | 9.231 / 0.276 | 9.587 |

- Window depth (512×384 dual, defaults): 20.132 ms (defaults) vs 20.147 ms (depth 0); no change.
- Acceptance:

| Workload | Mode | Mean ms | p95 ms | p99 ms | Long share |
|---|---|---:|---:|---:|---:|
| Replay + audio | original (01, 04) | 17.753 | 27.928 | 29.813 | 0.069 |
| Replay + audio | defaults (02, 03) | 16.625 | 17.510 | 18.419 | 0.000 |
| Live + audio | original (05, 08) | 17.883 | 27.796 | 29.436 | 0.076 |
| Live + audio | defaults (06, 07) | 16.670 | 17.235 | 18.616 | 0.001 |

- Health: every case `GL_error=0`, `starved_callbacks=0 missing_frames=0`, `producer_over_budget` 0–3, probe `errors=0`. Peak RSS 95–107 MiB; defaults are not higher than original.
- `malioc` confirms the Step 5 hypothesis for fog-constant: longest arithmetic path 11.03 → 11.20 cycles and total 11.25 → 11.53, work registers 31 → 32. Every non-fogged fragment pays the extra cycles.

### What didn't work
- The wrapper command for the two campaigns ended with `tail -3 file1 file2`, which zsh's `tail` rejected (`tail: option used in invalid context -- 3`), so the background task reported exit code 1. Both campaigns had already completed (`phase-d exit=0`, `acceptance exit=0`, `complete: true`, governor restored to `ondemand`).

### What I learned
- Presentation pacing is now measured, not inferred: frame time is the time until the next window buffer is free, so the loop is refresh-locked even with swap interval 0.
- With the defaults, the GPU still reports ≈13.0–13.1 M active cycles per loop frame at 60 FPS, i.e. close to fully busy. The improvement is real in the player-visible metric, but headroom remains thin.

### What was tricky to build
- Live runs have no replay signature, so the analyzer's signature sets must tolerate an empty value (`str()` around the receipt field).

### What warrants a second pair of eyes
- Live cases follow timing-dependent trajectories, so original and defaults do not draw identical frames; the ABBA ordering and the size of the change carry the comparison.
- The production launcher at `/storage/apps/falllinie-spike` was not replaced; it still runs the older binary.

### What should be done in the future
- Deploy the new binary to the launcher when the owner approves.
- Owner decision with side-by-side images: distance level of detail for sub-sample terrain triangles (59% of primitives), the largest remaining geometry cost and the main source of additional headroom.

### Code review instructions
- `python3 -I scripts/07-analyze.py scripts/evidence/acceptance-20261014T225930Z` and `.../phase-d-20261014T224833Z`.
- `scripts/08-live-case.sh` for the live configuration.

### Technical details
- Defaults now: `--shadow-mode bounds --view-culling on --short-indices on`; opt-in diagnostics: `--fog-cells constant` (rejected), `--output-clear on`, `--window-depth 0`, `--replay-audio`.

## Step 7: Publish to the vault (milestone)

Published the ticket's results as a new vault project report and linked it from the review and the optimization MOC. The vault keeps copies of every campaign summary, the Phase C comparison results and the live-acceptance figure produced by `scripts/09-figures.py`.

### Prompt Context
**User prompt (verbatim):** see Step 1.
**Commit (code):** 3620684 (evidence); vault commit 97d182d in go-go-parc.

### What I did
- `python3 -I scripts/09-figures.py scripts/evidence/acceptance-20261014T225930Z .../_assets/falllinie-perf009/01-live-acceptance.svg`.
- Vault: `Projects/2026/10/14/PROJECT REPORT - Falllinie GPU Budget Recovery - Shadow Bounds at the Shipping Size, Geometry Counters and Presentation Acquisition.md`; follow-up section appended to the performance review; reading-route sentence and catalogue row added to the MOC. Committed and pushed (`d6d8226..97d182d`).

### What didn't work
- No failures.

### What should be done in the future
- Owner decisions: deploy the new binary to the launcher; prepare side-by-side images for distance level of detail.
