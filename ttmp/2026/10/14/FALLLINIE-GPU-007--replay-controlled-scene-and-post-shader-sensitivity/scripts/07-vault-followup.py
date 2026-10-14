#!/usr/bin/env python3
"""Append the measured GPU007 checkpoint to the living GPU report."""
import hashlib,json,re,shutil,subprocess
import xml.etree.ElementTree as ET
from pathlib import Path
from PIL import Image
P=Path(__file__).resolve().parent;D=P/'evidence/target-20261014T191037Z'
V=Path('/home/manuel/code/wesen/go-go-golems/go-go-parc')
N=V/'Projects/2026/10/13/PROJECT REPORT - Falllinie GPU Profiling - Hardware Counters, Shader Compilation and a Rejected Optimization.md'
A=N.parent/'_assets/falllinie-gpu-shader-investigation/shader-sensitivity';A.mkdir(parents=True,exist_ok=True)
s=json.loads((D/'summary.json').read_text());assert len(s['cases'])==8
files=['summary.json','analysis-provenance.json','sensitivity.svg','endpoint-comparison.png','figure-provenance.json','asset-manifest.json']
for name in files:shutil.copyfile(D/name,A/name)
rows=['| Shader mode | Mean ms | p95 ms | p99 ms | Fragment Mcycles/s |','|---|---:|---:|---:|---:|']
for mode in ['baseline','post-copy','scene-flat','both']:
 g=s['groups'][mode];rows.append(f"| {mode} | {g['mean_ms']:.6f} | {g['p95_ms']:.6f} | {g['p99_ms']:.6f} | {g['rates_per_second']['MaliFragActiveCy']/1e6:.2f} |")
extra='''## 17. Measured follow-up: separate scene and post sensitivity

The next experiment has now been performed, rather than left as the proposal in Section 15. It uses the same qualified replay executable, 600-frame seed-42 sequence and ordinary indexed draws, at the demanding **512×384 dual-view** configuration with audio and application pacing disabled. The sequence is baseline, cheap post, flat scene, both, both, flat scene, cheap post, baseline. Each mode has two measured cases; the eight runs are a bounded symmetric experiment, not randomized repetitions across many seeds or sessions.

### 17.1 Change work explicitly, not production appearance implicitly

The cheap-post package retains the print FBO and fullscreen draw. Center depth selects white sky or grayscale scene luminance; original print-style, neighborhood and sky processing is omitted. The flat-scene package retains geometry, vertex-position source, material encoding and circular snow discard, but returns constant luminance without original lighting, analytic shadows, contour/grid/glyph work and fog. The combined package does both. Canonical shader assets and runtime source are unchanged; existing `--assets` selection chooses private packages without weakening replay's post-bypass restriction.

These are **non-parity diagnostic substitutions**, not production optimizations. Flat scene can change linked varyings, active uniforms and subsequent post work as well as fragment calculations. Changed images can affect compression, caching and compositor work. Holding the executable, draw structure and FBOs constant does not isolate GPU shader milliseconds.

The first host attempt used a color-only post copy. It linked, but the renderer rejected the removed active depth sampler with `fatal: missing print uniform uD`. A meaningful depth-aware selection fixed the contract without a dummy dependency or relaxed guard. Four corrected short host cases then passed before device collection; failed sources and logs remain in the diary evidence. Both original HUD/glyph atlases were packaged at their required parent path. Eighteen asset files were checksum-verified on the target before and after the campaign, and the exact executable hash was checked independently.

### 17.2 Actual repeated results

{TABLE}

Frame statistics pool 1,080 postwarmup intervals per mode; p95/p99 are computed from the pooled values, not averaged percentiles. Fragment rates aggregate the two cases using their contained host-window durations. The counter windows remain narrower than the complete frame-summary windows, and counters remain whole-device and compositor-inclusive.

Cheap post lowers the mean by **18.11%**, flat scene by **29.02%**, and both by **29.50%**, relative to this changed-workload experiment's baseline. The baseline bracket moves from 23.5443 to 23.6059 ms, approximately 0.26%; repeated nonbaseline means are close. These are much larger changes than that bracket drift, but still finite sensitivity observations rather than a general statistical guarantee.

![Measured shader sensitivity](./_assets/falllinie-gpu-shader-investigation/shader-sensitivity/sensitivity.svg)

The effects are not additive. Cheap post changes the mean by 4.2703 ms and flat scene by 6.8425 ms, but together they change it by 6.9544 ms—not the 11.1127 ms sum. Flat scene already reaches a mean near the apparent presentation floor. Adding cheap post reduces fragment activity from roughly 793–794 to 528 Mcycles/s with only another 0.1120 ms mean-time reduction. That is consistent with reduced GPU demand being hidden by another limit; it is not calibrated utilization or proof of the exact presentation mechanism.

The tails also resist a simple ranking. The combined p95 is **20.2223 ms**, worse than flat scene's **18.2899 ms**, while combined p99 is lower. The distributions cross. No scheduler or fence trace explains that behavior here, and it must not be hidden behind the better mean.

CPU work also changes. Sums of selected top/lower view CPU means are approximately 6.286 ms baseline, 6.214 ms cheap post, 5.790 ms flat scene and 5.282 ms both. These are selected nonoverlapping view scopes, not whole-loop CPU time. Linked shader changes can affect driver submission work; the experiment does not assign a precise cause to those CPU differences.

### 17.3 Confirm repeatability without claiming parity

All eight runs retain selected-state signature **6081555844518115723**. Repeated same-mode scene/depth/print captures match. Depth diagnostic images match across all modes; post-only changes preserve encoded scene images, and every diagnostic print differs from baseline as intended.

![Actual internal print targets for the four modes](./_assets/falllinie-gpu-shader-investigation/shader-sensitivity/endpoint-comparison.png)

*These are unscaled internal print-target captures after the uninstrumented endpoint redraw, not final HUD/compositor screenshots. Their visible differences are the intervention, not a rendering bug being accepted into production. Matching depth diagnostics do not prove every 24-bit depth value in every measured frame is identical.*

All game/probe statuses are zero and all receipts report `GL_error=0` with successful placement. The campaign contains 4,800 frames, 4,320 postwarmup intervals, 1,600 complete paired records and 46,800 counter rows. Invalid/dropped/active paired counts, collection errors and nonfinite values are zero. Rates use 728 contained adjacent host intervals.

Backend-minus-host offsets now span approximately 1.724–1.735 seconds. GPU snapshots remain 800 MHz, temperatures span 51.25–53.75°C, and CPU snapshots vary. After snapshots occur after the probe finishes, not at game exit. No fixed-clock, calibrated-busy, app-only counter or exact timestamp-join claim follows.

### 17.4 Decision and next narrower question

This experiment makes **scene-related work/content the stronger sensitivity** at the demanding configuration, while confirming that a cheaper post path also matters. It does not establish analytic shadows, contour calculations, glyph sampling or any single instruction as the cause. The next useful diagnostic is narrower: omit only the original scene shader's three analytic rider-shadow segment evaluations, then compare against the same replay. That shadow-only experiment remains proposed, not measured. Source-faithful production acceptance would require a separate candidate and appearance/performance evidence; none of these substitutions is promoted.

The complete evidence is ticket `FALLLINIE-GPU-007`, campaign `target-20261014T191037Z`, implementation/tooling commit `074588b` and measured checkpoint **`7a2db77`**. Runtime remains `32b8bfd` with the executable identity documented in Section 16. The native diary preserves the required-uniform failure, correction and measurement decisions. The vault retains the [actual summary](./_assets/falllinie-gpu-shader-investigation/shader-sensitivity/summary.json), [input provenance](./_assets/falllinie-gpu-shader-investigation/shader-sensitivity/analysis-provenance.json), [asset manifest](./_assets/falllinie-gpu-shader-investigation/shader-sensitivity/asset-manifest.json) and [publication hashes](./_assets/falllinie-gpu-shader-investigation/shader-sensitivity/publication-provenance.json). No new governor, affinity, driver, output-policy or production-launcher change was made.

'''.replace('{TABLE}','\n'.join(rows))
old=N.read_text();assert '## 17. Measured follow-up:' not in old
marker='\n\n## Falllinie optimization report network';assert old.count(marker)==1
new=old.replace(marker,'\n\n'+extra+marker)
new=new.replace('latest_native_evidence_commit: 6c5b231','latest_native_evidence_commit: 7a2db77')
new=new.replace('> [!summary] Latest checkpoint — deterministic replay and GPU scaling\n> The eight-case live replay now controls simulation input and terrain work per frame. Dual-view mean intervals are 16.62 / 17.40 / 23.57 ms at 256×192 / 384×288 / 512×384. Single-view loop rates remain near 60/s while GPU activity rises, suggesting a presentation constraint rather than unchanged GPU demand. Replay signatures and same-resolution endpoint images match; no production shader, resolution or draw-policy change is promoted.', '> [!summary] Latest checkpoint — repeated scene/post sensitivity\n> At dual 512×384, pooled means are 23.575 ms baseline, 19.305 ms cheap post, 16.733 ms flat scene and 16.621 ms both. These deliberate non-parity substitutions preserve tracked replay state, not the final image. Combined shading reductions lower hardware activity without much further mean-time gain; no production optimization is promoted. Section 17 contains the measured follow-up.')
new=new.replace('The original runtime/evidence metadata remains historical; `latest_*` identifies this follow-up.','Section 17 records the subsequently completed scene/post experiment; earlier future-work statements remain historical. The original runtime/evidence metadata remains historical; `latest_*` identifies the current follow-up.')
start='## What this investigation established\n';assert old.split(start,1)[1].split(marker)[0]==new.split(start,1)[1].split('\n\n## 17. Measured follow-up:')[0]
N.write_text(new)
M=V/'Projects/2026/10/13/MOC - Optimization and Performance Engineering.md';m=M.read_text();needle='That audio-disabled benchmark preserves the real rendering paths but deliberately fixes simulation input and streaming work per frame.';assert needle in m
M.write_text(m.replace(needle,needle+' Section 17 adds the repeated non-parity scene/post experiment: scene simplification has the larger timing effect, while combined reductions expose the apparent presentation floor. These are changed-image sensitivity results, not accepted production optimizations.'))
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
provenance={'native_evidence_commit':'7a2db77','note_sha256':sha(N),'historical_sections_1_through_16_preserved':True,'assets':{str(p.relative_to(V)):sha(p) for p in A.iterdir() if p.name!='publication-provenance.json'},'generator_sha256':sha(Path(__file__))}
(A/'publication-provenance.json').write_text(json.dumps(provenance,indent=2,sort_keys=True)+'\n')
for name in files:
 assert sha(A/name)==sha(D/name)
for target in re.findall(r'!?\[[^\]]*\]\(([^)]+)\)',new):
 if not target.startswith(('http://','https://')):assert (N.parent/target).exists(),target
ET.parse(A/'sensitivity.svg')
with Image.open(A/'endpoint-comparison.png') as image:image.verify()
(P/'evidence/publication-validation.json').write_text(json.dumps({'historical_sections_preserved':True,'new_assets':7,'cases':8,'note_sha256':sha(N),'links_assets_and_images_valid':True},indent=2)+'\n')
print(N)
