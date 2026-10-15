#!/bin/sh
# Long live autoplay case with launcher settings at a chosen internal size, muted full
# audio DSP, and a whole-device counter capture covering the run.
# Usage: ROOT OUT SIZE SECONDS [game flags...]   (SECONDS <= 115)
set -eu
ROOT=$1 OUT=$2 SIZE=$3 SECS=$4; shift 4
mkdir "$OUT"
export XDG_RUNTIME_DIR=/run/0-runtime-dir WAYLAND_DISPLAY=wayland-1 SWAYSOCK=/run/0-runtime-dir/sway-ipc.0.sock SDL_VIDEODRIVER=wayland
sampler=
cleanup(){ if [ -n "$sampler" ]; then kill "$sampler" 2>/dev/null || true; wait "$sampler" 2>/dev/null || true; fi; }
trap cleanup EXIT INT TERM
conditions(){ for f in /sys/class/thermal/thermal_zone*/temp /sys/devices/system/cpu/cpufreq/policy0/scaling_cur_freq /sys/devices/system/cpu/cpufreq/policy0/scaling_governor /sys/class/devfreq/fde60000.gpu/cur_freq; do echo "$f"; cat "$f"; done; }
conditions > "$OUT/conditions-before.txt"
timeout --kill-after=3s $((SECS+5))s "$ROOT/mali-counters" $((SECS+3)) base > "$OUT/counters.csv" 2> "$OUT/counters.log" & sampler=$!
set -- "$ROOT/falllinie-perf010" --assets "$ROOT/assets/shaders" --place --autoplay --dual-screen --render-size "$SIZE" --no-vsync --fps 60 --audio-buffer 1024 --no-vinyl --muted --seed 42 --seconds "$SECS" --profile "$OUT/frames.csv" --paired-clocks --profile-cadence 60 --receipt "$OUT/receipt.json" "$@"
printf '%s\n' "$@" > "$OUT/argv.txt"
status=0; timeout --kill-after=5s $((SECS+15))s "$@" > "$OUT/game.log" 2>&1 || status=$?
probe=0;wait "$sampler" || probe=$?;sampler=
printf 'game_status=%s\nprobe_status=%s\n' "$status" "$probe" > "$OUT/status.txt"
conditions > "$OUT/conditions-after.txt"
[ "$status" = 0 ] && [ "$probe" = 0 ]
