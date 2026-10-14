#!/bin/sh
set -eu
ROOT=$1 OUT=$2 MODE=$3
case "$MODE" in baseline) NONPARITY=false;;post-copy|scene-flat|both) NONPARITY=true;;*)exit 2;;esac
mkdir "$OUT"
printf '{"variant":"%s","non_parity":%s,"render_size":[512,384],"views":2,"frames":600,"fps_cap":0}\n' "$MODE" "$NONPARITY" > "$OUT/diagnostic.json"
export XDG_RUNTIME_DIR=/run/0-runtime-dir WAYLAND_DISPLAY=wayland-1 SWAYSOCK=/run/0-runtime-dir/sway-ipc.0.sock SDL_VIDEODRIVER=wayland
sampler=
cleanup(){ if [ -n "$sampler" ]; then kill "$sampler" 2>/dev/null || true; wait "$sampler" 2>/dev/null || true; fi; }
trap cleanup EXIT INT TERM
for f in /sys/class/thermal/thermal_zone*/temp /sys/devices/system/cpu/cpufreq/policy0/scaling_cur_freq; do echo "$f"; cat "$f"; done > "$OUT/conditions-before.txt"
timeout --kill-after=3s 50s "$ROOT/mali-counters" 45 > "$OUT/counters.csv" 2> "$OUT/counters.log" & sampler=$!
set -- "$ROOT/falllinie-gpu006" --assets "$ROOT/inputs/assets/$MODE" --benchmark-replay --frames 600 --seed 42 --no-vsync --fps 0 --render-size 512x384 --dual-screen --place --profile "$OUT/frames.csv" --paired-clocks --profile-cadence 60 --receipt "$OUT/receipt.json" --capture-dir "$OUT/capture"
printf '%s\n' "$@" > "$OUT/argv.txt"
status=0; timeout --kill-after=5s 40s "$@" > "$OUT/game.log" 2>&1 || status=$?
probe=0;wait "$sampler" || probe=$?;sampler=
printf 'game_status=%s\nprobe_status=%s\n' "$status" "$probe" > "$OUT/status.txt"
for f in /sys/class/thermal/thermal_zone*/temp /sys/devices/system/cpu/cpufreq/policy0/scaling_cur_freq; do echo "$f"; cat "$f"; done > "$OUT/conditions-after.txt"
[ "$status" = 0 ] && [ "$probe" = 0 ]
