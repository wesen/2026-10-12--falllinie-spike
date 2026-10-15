#!/bin/sh
# Heat soak of the deployed production binary: live autoplay with launcher settings
# (512x384, dual screen, muted full audio DSP), stock governor, for SECONDS, while a
# 5 s sampler records every thermal zone, CPU and GPU frequency and GPU load.
# Usage: OUT SECONDS
set -eu
OUT=$1 SECS=$2
mkdir -p "$OUT"
export XDG_RUNTIME_DIR=/run/0-runtime-dir WAYLAND_DISPLAY=wayland-1 SWAYSOCK=/run/0-runtime-dir/sway-ipc.0.sock SDL_VIDEODRIVER=wayland
cd /storage/apps/falllinie-spike
(
 echo "time_s,zones_mC,cpu_khz,cpu_governor,gpu_hz,gpu_load"
 start=$(date +%s)
 while :; do
  zones=$(cat /sys/class/thermal/thermal_zone*/temp | tr '\n' ' ')
  load=$(cat /sys/class/devfreq/fde60000.gpu/load 2>/dev/null || echo na)
  echo "$(( $(date +%s) - start )),$zones,$(cat /sys/devices/system/cpu/cpufreq/policy0/scaling_cur_freq),$(cat /sys/devices/system/cpu/cpufreq/policy0/scaling_governor),$(cat /sys/class/devfreq/fde60000.gpu/cur_freq),$load"
  sleep 5
 done
) > "$OUT/thermal.csv" 2>&1 & sampler=$!
trap 'kill $sampler 2>/dev/null || true' EXIT INT TERM
set -- ./falllinie-spike --place --autoplay --dual-screen --render-size 512x384 --no-vsync --fps 60 --audio-buffer 1024 --no-vinyl --muted --seed 42 --seconds "$SECS" --profile "$OUT/frames.csv" --receipt "$OUT/receipt.json"
printf '%s\n' "$@" > "$OUT/argv.txt"
status=0; timeout --kill-after=10s $((SECS+30))s "$@" > "$OUT/game.log" 2>&1 || status=$?
echo "game_status=$status" > "$OUT/status.txt"
