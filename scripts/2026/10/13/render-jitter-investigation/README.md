# Long-run rendering precision investigation

Parent source checkpoints: `44c841d` (reported failure/recovery evidence), `7eac73f` (camera-relative fix). Original shaders remain unchanged. Detailed investigation: ticket `FALLLINIE-GAME-002`, parent diary Steps 5–8.

## Reproduce the arithmetic check

From repository root:

```sh
mkdir -p build/jitter
c++ -std=c++17 -O2 -ffp-contract=off -Isrc \
  scripts/2026/10/13/render-jitter-investigation/precision-probe.cpp \
  -o build/jitter/precision-probe
build/jitter/precision-probe
```

This emulates float GLSL matrix arithmetic, not Mali FMA or actual scanout. It includes production helper checks and an origin boundary with both chase and rolled first-person up vectors. Expected relative ray-error/jump threshold: `1e-4`. Pixel figures are angular estimates, not registered frame measurements.

## Selected on-device comparison

The initially captured interactive process had advanced to z≈58600 m. The selected diagnostic case uses seed 42, cell 536 (camera z=58960 m, y=-18288.21271 m), four cells and 384×288 rendering. Both runs used the same new executable; only the absolute-coordinate diagnostic flag differed.

On the device, from `/storage/apps/falllinie-spike`, with the usual SDL Wayland environment and no other running instance:

```sh
./falllinie-spike --place --cell 536 --cells 4 --render-size 384x288 \
  --frames 90 --absolute-render-coordinates \
  --capture-dir /tmp/falllinie-far-absolute \
  --receipt /tmp/falllinie-far-absolute/receipt.json
./falllinie-spike --place --cell 536 --cells 4 --render-size 384x288 \
  --frames 90 --capture-dir /tmp/falllinie-far-relative \
  --receipt /tmp/falllinie-far-relative/receipt.json
```

Copies of each run's `print.bmp`, `scene.bmp`, `depth.bmp`, receipt and log are stored alongside this README. The absolute print has dense false blue speckle across the terrain banks; the relative print removes that pattern while preserving the terrain, original halftone and source paint. Both receipts report GL_error=0. A static comparison does not prove dynamic physical horizon stability; the CPU translation probe specifically exercises that failure mechanism.

## Restored interactive instance

The old owned process PID 210855 was gracefully terminated with SIGTERM. The binary was replaced through `.new` then rename. The restored instance is PID 264668 at capture time, SHA256 `4c997f2f150e80fc3239d58835450e50620693aedfa2f4c580e5f16f575cb4ce`, running:

```sh
./falllinie-spike --dual-screen --place --render-size 384x288 \
  --no-vsync --fps 60 --profile /tmp/falllinie-jitter-fixed-live.csv
```

`fixed-live.csv` is the first 960 frames (60 warmup + 900 measured), copied without ending the interactive instance. `fixed-live-summary.json` was generated with:

```sh
node scripts/2026/10/13/falllinie-performance/analyze-profile.cjs \
  scripts/2026/10/13/render-jitter-investigation/fixed-live.csv
```

Production rate≈60 FPS, interval p95=16.736092 ms, zero intervals above 18.333 ms. These are application production intervals, not compositor scanout feedback. No audio/new gameplay lane is deployed yet, and this is not the final integrated human playtest.
