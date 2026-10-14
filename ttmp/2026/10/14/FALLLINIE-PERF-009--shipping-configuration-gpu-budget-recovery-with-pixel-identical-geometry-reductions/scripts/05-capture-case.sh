#!/bin/sh
# Untimed visual checkpoint: replay to FRAMES, then capture full windows and internal targets.
# Usage: ROOT OUT SIZE FRAMES SEED PALETTE [game flags...]
set -eu
ROOT=$1 OUT=$2 SIZE=$3 FRAMES=$4 SEED=$5 PALETTE=$6; shift 6
mkdir "$OUT"
export XDG_RUNTIME_DIR=/run/0-runtime-dir WAYLAND_DISPLAY=wayland-1 SWAYSOCK=/run/0-runtime-dir/sway-ipc.0.sock SDL_VIDEODRIVER=wayland
set -- "$ROOT/falllinie-perf009" --assets "$ROOT/assets/shaders" --benchmark-replay --frames "$FRAMES" --seed "$SEED" --palette "$PALETTE" --no-vsync --fps 0 --render-size "$SIZE" --dual-screen --place --receipt "$OUT/receipt.json" --capture-dir "$OUT/capture" "$@"
printf '%s\n' "$@" > "$OUT/argv.txt"
status=0; timeout --kill-after=5s 75s "$@" > "$OUT/game.log" 2>&1 || status=$?
printf 'game_status=%s\n' "$status" > "$OUT/status.txt"
exit "$status"
