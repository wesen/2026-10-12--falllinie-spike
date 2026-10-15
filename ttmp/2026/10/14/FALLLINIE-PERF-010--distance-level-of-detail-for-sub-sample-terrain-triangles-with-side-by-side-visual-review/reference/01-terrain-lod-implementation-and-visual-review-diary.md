---
Title: Terrain LOD implementation and visual review diary
Ticket: FALLLINIE-PERF-010
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
Summary: "Diary for PERF-010: row-decimated terrain LOD, device measurement and side-by-side visual review."
LastUpdated: 2026-10-14T19:12:28.905816396-04:00
WhatFor: ""
WhenToUse: ""
---

## Step 1: Design and implement crack-free row-decimated terrain LOD

PERF-009 showed that 59% of submitted primitives cover no sample and that geometry is about half of GPU-active time. This ticket implements the change that removes most of them, distant terrain level of detail, as an opt-in, pixel-changing option for an owner decision. Each 16-strip terrain chunk gets decimated index ranges that keep every column and every 2^L-th row, always including the chunk's two boundary rows. Neighbouring chunks therefore share identical full-resolution boundary rows at any level and cannot crack.

On the host replay at 384×288 dual view (600 frames), `--terrain-lod 120` halves submitted triangles (212.4 M → 107.4 M over 1,200 views) and changes 0.34% of internal print pixels at the endpoint; `--terrain-lod 60` removes 64% (→ 77.3 M) and changes 0.68%. In a host side-by-side at 60 m, the differences are thin lines along distant ridgelines and edges, with no visible cracks.

### Prompt Context
**User prompt (verbatim):** "How can we make things faster, can you run the subpixel triangle culling and show me side by side"
**Assistant interpretation:** Implement the sub-pixel-triangle reduction identified in PERF-009 (distance level of detail), measure its speed on the device, and present original and reduced renders side by side so the owner can judge the visual cost.
**Inferred user intent:** Get more GPU headroom and decide with real images whether the visual change is acceptable.
**Commit (code):** b5c7eb3 — "Add opt-in row-decimated terrain level of detail (--terrain-lod NEAR_M), pixel-changing, default off"

### What I did
- Created ticket `FALLLINIE-PERF-010`, design doc and this diary.
- `src/geometry_bounds.hpp`: `TerrainLod`, `build_terrain_lod(base, vertices, columns=107, strips_per_chunk=16, out)` (validates the exact `{A,C,B,B,C,D}` strip pattern for every quad, then emits levels 1–4 per chunk when the stride divides the chunk's strip count), and `terrain_lod_level(distance, near, ranges)`.
- `src/renderer.*`: `upload_terrain()` for terrain cells (three call sites in `renderer.cpp`/`game_renderer.cpp`); the element buffer stores original indices followed by levels 1–4; per-chunk ranges in `CellMesh::lod`; the chunk loop picks the level by box distance from the shading eye and merges adjacent same-level ranges; stats `lod_triangles[5]`.
- `src/main.cpp`: `--terrain-lod off|NEAR_M` (10–1000 m), log line and receipt field.
- `tests/test_geometry_bounds.cpp`: on a real cell (`build_terrain(Terrain(42),3)`), level 0 equals the chunk range, every level uses only rows at multiples of the stride from the chunk start, includes both boundary rows and emits `(strips/stride)·106·6` indices; a swapped index makes validation fail. The test now links `terrain`.
- PERF-010 scripts copied from PERF-009 with the baseline set to the PERF-009 defaults; plan `lod`.

### Why
- Decimating rows only keeps every lateral vertex on chunk boundaries, which removes the need for crack stitching or skirts. Rows are also the direction that collapses fastest with distance at grazing view angles.

### What worked
- `ctest`: 17/17 passed with the extended geometry test.
- Host draw statistics (1,200 views): off 212,422,480 triangles; 120 m 107,359,308 (levels 0–4: 55.5 M / 23.7 M / 17.0 M / 4.3 M / 0); 60 m 77,341,804 (34.6 M / 16.5 M / 8.8 M / 8.5 M / 2.2 M).
- Host endpoint differences vs off: 120 m 381 of 110,592 print pixels (top), 2,642 of 786,432 output pixels; 60 m 754 print and 5,279 output pixels.

### What didn't work
- My first edit script failed to parse (`SyntaxError: unterminated string literal (detected at line 50)`) because two multi-line replacements used single-quoted raw strings; switched them to triple quotes before any file changed.

### What I learned
- Differences concentrate on distant silhouettes and terrain edges, where the coarse surface cuts corners, and on contour lines whose interpolated height shifts slightly.

### What was tricky to build
- Merging draw runs must compare positions in the combined buffer: level-L ranges are contiguous only between consecutive chunks that both support level L, so a run breaks whenever the next chunk's range does not start where the previous one ended.

### What warrants a second pair of eyes
- Vertex normals stay those of the full-resolution mesh; lighting across a coarse triangle interpolates normals of vertices that are further apart.

### What should be done in the future
- Device timing and images (next step), then the owner decision.

### Code review instructions
- `build_terrain_lod` and `terrain_lod_level` in `src/geometry_bounds.hpp`; the chunk loop in `Renderer::render`.

### Technical details
- Level selection: `L = min(4, floor(log2(d/N)) + 1)` for `d >= N`, reduced until supported; the last chunk of a cell has 10 strips, so it supports levels 1 only.

## Step 2: Device measurement and side-by-side review

The device campaign measured both LOD settings against the PERF-009 defaults and captured all five checkpoints. The 60 m setting changes the GPU's situation: at the shipping size with the audio producer running, GPU-active cycles per loop frame fall from 13.14 M (≈99% of the 800 MHz clock) to 9.22 M (≈69%), so the GPU has idle time for the first time in this series. At 512×384 dual view, where the defaults still miss refreshes (mean 20.116 ms, long frames 21.0%), 60 m holds 16.617 ms with no long frames. Primitives per frame fall from 368,600 to 134,000 and sub-sample culls from 252,000 to 54,500; memory reads fall from 21.2 to 9.2 MB per frame.

The visual cost is 0.29–0.70% of window pixels at 120 m and 0.60–1.38% at 60 m per 1024×768 window, concentrated on distant ridge lines, terrain edges and halftone dots near them. The side-by-side page with swipe and difference views is published at https://claude.ai/artifact/RLbNEsTb18oKnLUAw8qVvd for the owner decision.

### Prompt Context
**User prompt (verbatim):** see Step 1.
**Assistant interpretation:** Measure the LOD candidates on the handheld and present the images side by side.
**Inferred user intent:** Decide whether the visual change is acceptable for the measured speedup.
**Commit (code):** b5c7eb3 (device binary `4e1b588c…38fb`)

### What I did
- `python3 06-campaign.py lod` → `evidence/lod-20261014T231641Z`: 15 visual captures (baseline = defaults, lod120, lod60 at five checkpoints), 6 timed shipping cases with audio (defaults/120/60/60/120/defaults), 4 timed 512×384 cases (defaults/60/60/defaults), 2 geometry-counter cases (defaults, 60).
- `python3 -I 07-analyze.py` on the campaign.
- `python3 -I 08-review-assets.py evidence/lod-… <scratch>/lod-review`: 70 PNGs (full windows, red difference overlays, 2× crops of the 256×192 region with most changes) and `review.json`.
- Built the review page from `scripts/review-template.html` with `review.json` inlined and published it as an artifact; kept `evidence/review/terrain-lod-review.html`, `review.json` and the URL. Kept PNGs in the repository only for checkpoint `f0600-s42-p0`; the published page carries all of them.

### Why
- The shipping size with audio is the acceptance configuration; 512×384 shows the size of the saving when the GPU is overloaded; the geometry counters show where the saving comes from.

### What worked

| Mode | Mean ms (384, audio) | p95 ms | GPU-active Mcy/frame | GPU busy | 512×384 mean | 512×384 long frames |
|---|---:|---:|---:|---:|---:|---:|
| defaults | 16.625 | 17.485 | 13.14 | ≈99% | 20.116 | 21.0% |
| LOD 120 m | 16.621 | 17.834 | 10.23 | ≈77% | not measured | not measured |
| LOD 60 m | 16.619 | 17.867 | 9.22 | ≈69% | 16.617 | 0.0% |

- Geometry counters per frame (defaults → 60 m): total primitives 368.6k → 134.0k; sample-culled 252.0k → 54.5k; visible 66.5k → 35.7k; position-shader threads 388.3k → 152.4k; non-fragment active 5.26 M → 2.80 M cycles; reads 21.2 → 9.2 MB; writes 13.4 → 10.2 MB.
- Signature `6081555844518115723` in every case; `GL_error=0` everywhere; probe `errors=0`.

### What didn't work
- Case `05-lod120-audio` recorded one audio starvation: `starved_callbacks=1 missing_frames=1024`, with `producer_over_budget=5`. All other audio cases had zero. The GPU change does not plausibly cause a CPU-side producer delay, but this run alone cannot rule it out; it is recorded rather than discarded.
- At the shipping size the mean and p95 do not improve. The defaults already reach 60 Hz there, and the 60 m p95 is slightly higher (17.87 vs 17.49 ms) in this campaign. The benefit at the shipping size is headroom (GPU busy ≈99% → ≈69%), not a faster frame.
- `visible` primitives also fell (66.5k → 35.7k): some distant decimated triangles still cover samples, but more of their area is now covered by fewer, larger triangles. This is where the pixel changes come from.

### What I learned
- Sub-sample terrain was the dominant GPU cost. Removing about 64% of submitted primitives frees about 30% of the GPU.

### What was tricky to build
- The page must work without the device: all images ship as files beside the page and the statistics are inlined, so it renders completely at rest.

### What warrants a second pair of eyes
- Whether the visible changes (distant ridge lines, halftone dots along them, slightly shifted contour lines) are acceptable for the game's look, especially at frame 120 where the near distance falls on more visible terrain.

### What should be done in the future
- Owner decision: off, 120 m or 60 m. If accepted, rerun the live acceptance campaign with the chosen setting as default and repeat the audio cases to check the single starvation.

### Code review instructions
- `scripts/08-review-assets.py`, `scripts/review-template.html`, `scripts/06-campaign.py` (plan `lod`).
- `python3 -I scripts/07-analyze.py scripts/evidence/lod-20261014T231641Z`.

### Technical details
- GPU busy = GPU-active cycles per frame × loop FPS ÷ 800 MHz.

## Step 3: Owner chose 60 m; acceptance with audio in replay and live autoplay

The owner chose the 60 m setting after reviewing the side-by-side page. It is now the default (`--terrain-lod off` restores full detail). The acceptance campaign compared previous defaults with the new default at the shipping size, with the muted audio producer running, in the replay (three runs each) and in live autoplay (two runs each). Both already met 60 Hz; the new default lowers the tail (live p95 17.40 → 16.78 ms, p99 18.84 → 17.69 ms) and cuts GPU-active cycles per frame from 12.99 M to 9.24 M, about 70% instead of 97% of the GPU.

The repeated audio runs answer the open question from Step 2. One audio starvation occurred with the new default (`05-lod60-audio`) and one with the previous default (`10-previous-live`). The rare underrun is therefore a property of the audio producer under this load, not of the terrain change.

### Prompt Context
**User prompt (verbatim):** "60m.\n\nexplain the 1024x768 output copies for?"
**Assistant interpretation:** Make 60 m the default, qualify it, and explain the role and cost of the full-resolution output pass.
**Inferred user intent:** Keep the speedup and understand the next cost centre.
**Commit (code):** aa7d5f5 — "Make 60 m row-decimated terrain LOD the default (owner decision); add 60 m acceptance plan" (device binary `e3e94bd7…afbc62f`)

### What I did
- `src/main.cpp`: `terrain_lod` default 60; help text.
- `06-campaign.py`: plan `accept60`, which compares explicit `--terrain-lod off` with the default; the live-case script is now part of the deployment. The earlier `lod` plan used an empty flag list for its baseline when the default was still off; its evidence stays valid for the binary it ran (`b5c7eb3`).
- `python3 06-campaign.py accept60` → `evidence/accept60-20261014T233951Z`; `07-analyze.py`.

### What worked

| Workload | Mode | Mean ms | p95 ms | p99 ms | Long | GPU-active Mcy/frame |
|---|---|---:|---:|---:|---:|---:|
| Replay + audio | previous (01, 04, 06) | 16.626 | 18.240 | 19.296 | 0.1% | 13.13 |
| Replay + audio | 60 m (02, 03, 05) | 16.619 | 17.647 | 18.682 | 0.0% | 9.24 |
| Live + audio | previous (07, 10) | 16.667 | 17.399 | 18.837 | 0.0% | 12.99 |
| Live + audio | 60 m (08, 09) | 16.663 | 16.777 | 17.687 | 0.0% | 9.24 |

- `GL_error=0` everywhere; governor restored to `ondemand`; replay signature unchanged.
- Audio producer CPU per block is slightly lower with 60 m (9.06–9.32 ms vs 9.61–10.06 ms), consistent with less CPU contention from driver work.

### What didn't work
- Two audio starvations of one block each: `05-lod60-audio` (`starved_callbacks=1 missing_frames=1024`, `producer_over_budget=4`) and `10-previous-live` (`starved_callbacks=1 missing_frames=1024`, `producer_over_budget=5`). Not caused by the terrain change; a separate audio-robustness follow-up.

### What I learned
- The output pass (two 1024×768 full-screen copies, ≈1.57 M fragment threads and ≈6.3 MB of writes per frame) is now the largest fragment cost; its only substantial saving would come from display-side scaling, which changes the look.

### What should be done in the future
- Audio: investigate the rare producer overrun (deeper target lead or a cheaper reverb tail).
- Resolution comparison 384×288 vs 512×384 with 60 m (next step).

### Code review instructions
- `python3 -I scripts/07-analyze.py scripts/evidence/accept60-*`.

## Step 4: Internal resolution 384×288 vs 512×384, side by side

The owner asked for a side-by-side capture of the two internal sizes after the explanation of the output pass. Both sizes run with the 60 m terrain reduction and hold 60 Hz with no long frames, in the replay and in live autoplay, with the muted audio producer running. 512×384 uses about 93% of the GPU against about 69% at 384×288. In return it stretches to the 1024×768 panels by exactly 2×: the 384×288 close-ups show a plaid moiré in the halftone and lumpier outlines, caused by the uneven 2- and 3-pixel stretch, while 512×384 shows an even dot pattern. The comparison page is published at https://claude.ai/artifact/Cvb2FU7yxw5QPcXBMHB6hh.

### Prompt Context
**User prompt (verbatim):** "yes, do a side by side capture"
**Assistant interpretation:** Capture both internal sizes on the device at the standard checkpoints, measure them, and present them side by side.
**Inferred user intent:** Decide whether to spend the new GPU headroom on a sharper image.
**Commit (code):** aa7d5f5 binary (`e3e94bd7…afbc62f`); scripts in this step's commit.

### What I did
- Plan `res512` in `06-campaign.py`: visual baseline (default 384×288) vs `--render-size 512x384` at five checkpoints; timed replay+audio r384/r512/r512/r384; live+audio r384/r512/r512/r384.
- `09-resolution-assets.py`: full windows for both sizes and 2× crops of three fixed regions per window; `resolution-template.html` page; published as an artifact.

### What didn't work
- First run `evidence/res512-20261014T234755Z` stopped after the first checkpoint: `capture_tools.compare` asserted equal image sizes (`assert ia.size==ib.size` → `AssertionError`), but the internal scene/print/depth targets differ by design. Its early files had been swept into commit `b637d1f`; this step removes them from the index and keeps the directory outside the repository. `compare` now records `size_mismatch` for differently sized images and the campaign's pixel count ignores those entries. Rerun `evidence/res512-20261014T234849Z` completed.

### What worked

| Size | Workload | GPU busy | Mean ms | p95 ms | p99 ms | Long |
|---|---|---:|---:|---:|---:|---:|
| 384×288 | replay + audio | 69.3% | 16.620 | 17.825 | 18.786 | 0.0% |
| 512×384 | replay + audio | 93.0% | 16.619 | 17.021 | 17.572 | 0.0% |
| 384×288 | live + audio | 69.7% | 16.661 | 16.937 | 18.051 | 0.0% |
| 512×384 | live + audio | 93.2% | 16.667 | 16.751 | 17.097 | 0.0% |

- All eight cases `GL_error=0`, `starved_callbacks=0 missing_frames=0`; receipts confirm `render_size` [384,288] / [512,384].

### What I learned
- At 512×384 the halftone pitch (`4.4·width/1024`) is exactly 2.2 texels and every texel maps to a 2×2 screen block, which removes the beat pattern visible at 384×288.
- 512×384 tails were slightly tighter than 384×288 in these runs; the difference is small and both are refresh-locked.

### What warrants a second pair of eyes
- 93% GPU busy leaves less margin for heavier scenes than 69%; a decision for 512×384 should be checked over a longer live session.

### What should be done in the future
- Owner decision on internal size; if 512×384, change the launcher argument (`--render-size`) rather than the binary default, since the launcher already passes 384×288.

### Code review instructions
- `scripts/09-resolution-assets.py`, `scripts/resolution-template.html`, `scripts/capture_tools.py` (`size_mismatch`).

## Step 5: Owner chose 512×384; long live sessions; report updated

The owner chose 512×384 ("crisp is much better"). The launcher now passes `--render-size 512x384`; the binary default was already 512×384. Three 110-second live autoplay sessions with launcher settings and muted full audio, seeds 42, 7 and 1234, produced 19,605 post-warmup frames with none at or above 25 ms, the longest 21.74 ms, p99 16.92–16.94 ms and the GPU 89.5–90.6% busy. The vault report gained a latest-checkpoint summary and Section 10, and the MOC row was updated.

### Prompt Context
**User prompt (verbatim):** "crisp is much better , 512x384 baby" — then "Update the report."
**Assistant interpretation:** Switch the launch configuration to 512×384, check the narrower margin over longer play, and record the decision and results in the vault report.
**Inferred user intent:** Ship the sharper look, provided it still holds 60 FPS.
**Commit (code):** 58e90a5 — "Launch at 512x384 internal resolution (owner decision); add long live-session plan"

### What I did
- `scripts/falllinie-spike.sh`: `--render-size 384x288` → `512x384`.
- `scripts/10-long-live-case.sh` and plan `long512` (110 s per session, counter capture over the whole run, ssh timeout 160 s).
- `python3 06-campaign.py long512` → `evidence/long512-20261015T000047Z`; `07-analyze.py`.
- Vault: report latest-checkpoint callout and Section 10; MOC catalogue row; pushed `35f1aa1..e1f147f`.

### What worked
- Per seed (mean / p95 / p99 / longest / GPU busy): 42: 16.667 / 16.735 / 16.939 / 21.45 ms / 90.6%; 7: 16.667 / 16.738 / 16.942 / 21.74 ms / 90.3%; 1234: 16.666 / 16.745 / 16.917 / 19.75 ms / 89.5%. Zero frames ≥ 25 ms; 9–13 frames per run above 18.33 ms.
- `GL_error=0`, `starved_callbacks=0`, peak RSS 99–102 MiB, thermal zone 0 at 68.1–71.7 °C after each run.

### What didn't work
- No failures.

### What I learned
- At 512×384 with 60 m terrain detail the GPU keeps about 10% idle time, stable across three seeds and 110-second sessions.

### What should be done in the future
- Deploy to the handheld's production install when the owner approves (`scripts/deploy-device.sh` copies the binary, assets and launcher).
- Audio: the rare one-block underrun seen in earlier 30-second runs (independent of rendering) did not recur in these sessions but remains open.

### Code review instructions
- `scripts/falllinie-spike.sh`; `python3 -I scripts/07-analyze.py scripts/evidence/long512-20261015T000047Z`.

## Step 6: Deploy, register in ROCKNIX Ports, publish the deep dive

The release was deployed to the handheld's production install and registered in EmulationStation's Ports menu with a name, description and screenshot. A heat soak was started and stopped at the owner's request. The full deep-dive report was published to the vault.

### Prompt Context
**User prompt (verbatim):** "yeah go ahead. do it. Then  write a detailed project report for the obsidian vault as a deep dive technical analysis blog post using a textbook writing style (no analogies, see skill).      \n Commit and push the bsidian vault when done (go-go-parc vault)." Follow-ups during the work: "the commit and push" and "no need for soak".
**Assistant interpretation:** Build and deploy the production binary, add a proper Ports entry and restart EmulationStation, then write and push a vault deep dive; commit and push both repositories; skip the heat soak.
**Inferred user intent:** Have the improved game installed and launchable from the handheld menu, with a durable write-up.
**Commit (code):** 0bf9969 — "Register Falllinie in the ROCKNIX Ports gamelist on deploy; keep previous binary for rollback; add heat soak"

### What I did
- `bash scripts/build-device.sh` (production flags) → `build-device/falllinie-spike` SHA-256 `7f86667cc49c1aa15e725ee636f8db8f58445f1c79cf7fc6988c980f16363398`.
- `scripts/ports/merge-gamelist.py` (replaces only the `./falllinie-spike.sh` entry, keeps others) and `scripts/ports/falllinie-spike.png` (512×384 device capture, top window, frame 600).
- `scripts/deploy-device.sh`: backs up the previous binary as `falllinie-spike.previous` and the gamelist as `gamelist.xml.bak`, merges the entry, copies the image, restarts `essway` when `RESTART_ES=1`, prints installed hashes.
- `RESTART_ES=1 bash scripts/deploy-device.sh`; verified binary hash, launcher `--render-size 512x384`, gamelist content, image, `essway` active, and a `grim -o DSI-2` capture of EmulationStation.
- Started `scripts/11-heat-soak.sh` (1,200 s); stopped it after about 2.5 minutes at the owner's request; partial data not used.
- Vault: `PROJECT REPORT - Falllinie at 60 FPS - Geometry, Presentation and a Crisp 512x384 Release on a Single-Core Mali-G52.md` with two figures, linked from the MOC; pushed `e1f147f..85fcee7`.

### What worked
- EmulationStation restarted cleanly; its log shows no "has no games" warning for Ports and one unrelated `HttpReq::onError` network error.
- Gamelist merge test on host: an input with a DS demo entry and a stale Falllinie entry produced two games, the stale one replaced.

### What didn't work
- `scripts/build-device.sh` is not executable in the repository: `permission denied: scripts/build-device.sh`; ran it with `bash`.
- Stopping the soak: my first remote kill loop matched its own `sh -c` command line (`pgrep -f "falllinie-spike --place --autoplay"`), killed the ssh session (exit 255) and left the game process running; killed it by PID 1246649 and confirmed nothing remained.

### What I learned
- ROCKNIX Ports is defined in `/storage/.config/emulationstation/es_systems.cfg` (path `/storage/roms/ports`, extensions `.sh .appimage`), and the gamelist lives beside the scripts as `gamelist.xml`.

### What should be done in the future
- Human playtest from the Ports menu; audio underrun fix; long-session thermal check if wanted.

### Code review instructions
- `scripts/deploy-device.sh`, `scripts/ports/merge-gamelist.py`.
- Rollback on the device: `cd /storage/apps/falllinie-spike && mv falllinie-spike.previous falllinie-spike` and restore `gamelist.xml.bak`.
