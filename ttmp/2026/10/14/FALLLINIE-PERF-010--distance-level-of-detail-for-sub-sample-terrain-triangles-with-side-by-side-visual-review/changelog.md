# Changelog

## 2026-10-14

- Initial workspace created

## 2026-10-14

Row-decimated terrain LOD measured on device: 60 m frees ~30% of the GPU (busy 99% -> 69%) and fixes 512x384 dual view (21% -> 0% long frames); 0.6-1.4% of window pixels change. Side-by-side review published for owner decision.

### Related Files

- /home/manuel/code/wesen/2026-10-12--falllinie-spike/src/geometry_bounds.hpp — build_terrain_lod
