---
Title: Row-decimated terrain level of detail design
Ticket: FALLLINIE-PERF-010
Status: active
Topics:
    - performance
    - graphics
    - handheld
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Crack-free row decimation of distant terrain row blocks to remove sub-sample triangles; a pixel-changing optimization measured on the device and presented side by side for an owner decision."
LastUpdated: 2026-10-14T19:12:28.60171964-04:00
WhatFor: ""
WhenToUse: ""
---

# Row-decimated terrain level of detail

## 1. Problem

PERF-009's geometry counters show about 425,000 primitives per frame at the shipping configuration, of which 252,000 (59%) cover no sample and are discarded by the GPU after index fetch, position shading and tiling. Non-fragment activity is about half of GPU-active time, and the GPU is still near saturation with the PERF-009 defaults. The sub-sample triangles are distant terrain: rows are 0.8 m apart down the slope, and at grazing view angles their projected spacing shrinks quickly with distance.

Removing them changes pixels, because a coarser surface has slightly different depth, silhouettes and interpolated attributes. This ticket therefore produces a measured candidate and a side-by-side visual comparison for an owner decision. It does not change any default.

## 2. Mesh structure used

`TerrainJob` emits a cell as 139 rows × 107 columns of vertices and 138 row strips of 106 quads. Strip `k` connects rows `k` and `k+1`; its six indices per column are `{A, C, B, B, C, D}` with `A = k·107 + i`, `B = A + 1`, `C = A + 107`, `D = C + 1`. PERF-009 already splits each cell into chunks of 16 strips (10,176 indices).

## 3. Decimation rule

For a chunk covering strips `s0 … s0+n−1` (rows `s0 … s0+n`), level `L` uses row stride `t = 2^L` when `t` divides `n`. It emits, for rows `a = s0, s0+t, …` and every column `i`, the quad `{A, C, B, B, C, D}` with `C = A + t·107`. All columns are kept.

**No cracks.** The chunk's first and last rows (`s0` and `s0+n`) are always included and keep every column. Adjacent chunks share those boundary rows with identical vertices, whatever level each uses, so no T-junctions appear between chunks. Within a chunk, the coarse surface is the full-resolution surface sampled on fewer rows. Vertex normals and attributes are the original per-vertex values.

**Level selection.** With `d` the distance from the shading eye to the nearest point of the chunk's bounding box and `N` the configured near distance, `L = 0` for `d < N`, otherwise `L = min(4, ⌊log₂(d/N)⌋ + 1)`, reduced until `2^L` divides the chunk's strip count. Level 0 is the original index range.

## 4. GPU layout

The element buffer stores the original indices first, then for each level 1–4 every chunk's decimated indices in chunk order. Each chunk records `(first, count)` per level. Consecutive surviving chunks with the same level, fog state and adjacent ranges are merged into one draw. This keeps 16-bit indices, culling and draw order intact.

## 5. Options

`--terrain-lod off|N`, where `N` is the near distance in metres (default `off`). Two candidates are measured: `N = 120` (conservative) and `N = 60` (aggressive).

## 6. Evaluation

- **Timing.** ABBA replay campaigns at 384×288 dual view (shipping, defaults baseline) with base and geometry counters, and at 512×384 dual view for sensitivity.
- **Images.** Full top and lower windows plus internal targets at the five standard checkpoints, compared with the PERF-009 defaults. Report differing pixels, maximum channel delta and the spatial distribution.
- **Side-by-side review.** A comparison page with full windows side by side, a difference overlay and enlarged crops of the largest difference regions.
- **Decision.** Owner choice between off, 120 m and 60 m. No default changes without that decision.
