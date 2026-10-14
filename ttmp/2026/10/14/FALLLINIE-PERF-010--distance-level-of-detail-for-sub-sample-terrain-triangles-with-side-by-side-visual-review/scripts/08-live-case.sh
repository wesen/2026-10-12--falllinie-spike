#!/bin/sh
# Live autoplay case (not replay): launcher-like settings, muted full audio DSP, 30 s,
# with a whole-device counter capture. Usage: ROOT OUT [game flags...]
set -eu
ROOT=$1 OUT=$2; shift 2
mkdir "$OUT"
export XDG_RUNTIME_DIR=/run/0-runtime-dir WAYLAND_DISPLAY=wayland-1 SWAYSOCK=/run/0-runtime-dir/sway-ipc.0.sock SDL_VIDEODRIVER=wayland
sampler=
cleanup(){ if [ -n "$sampler" ]; then kill "$sampler" 2>/dev/null || true; wait "$sampler" 2>/dev/null || true; fi; }
trap cleanup EXIT INT TERM
conditions(){ for f in /sys/class/thermal/thermal_zone*/temp /sys/devices/system/cpu/cpufreq/policy0/scaling_cur_freq /sys/devices/system/cpu/cpufreq/policy0/scaling_governor /sys/class/devfreq/fde60000.gpu/cur_freq; do echo "$f"; cat "$f"; done; }
conditions > "$OUT/conditions-before.txt"
timeout --kill-after=3s 50s "$ROOT/mali-counters" 45 base > "$OUT/counters.csv" 2> "$OUT/counters.log" & sampler=$!
set -- "$ROOT/falllinie-perf010" --assets "$ROOT/assets/shaders" --place --autoplay --dual-screen --render-size 384x288 --no-vsync --fps 60 --audio-buffer 1024 --no-vinyl --muted --seed 42 --seconds 30 --profile "$OUT/frames.csv" --paired-clocks --profile-cadence 60 --receipt "$OUT/receipt.json" "$@"
printf '%s\n' "$@" > "$OUT/argv.txt"
status=0; timeout --kill-after=5s 45s "$@" > "$OUT/game.log" 2>&1 || status=$?
probe=0;wait "$sampler" || probe=$?;sampler=
printf 'game_status=%s\nprobe_status=%s\n' "$status" "$probe" > "$OUT/status.txt"
conditions > "$OUT/conditions-after.txt"
[ "$status" = 0 ] && [ "$probe" = 0 ]
