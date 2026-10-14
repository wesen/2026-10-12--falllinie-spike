#!/bin/sh
set -eu
[ "$#" = 4 ] || exit 2
PROBE=$1 GAME=$2 OUT=$3 CASE=$4
case "$CASE" in idle|baseline|live|bypass) ;; *) exit 2;; esac
mkdir "$OUT"
date -u +%FT%TZ > "$OUT/start.txt"
cat /proc/sys/kernel/random/boot_id >> "$OUT/start.txt"
sha256sum "$PROBE" "$GAME" > "$OUT/binaries.sha256"
if [ "$CASE" = idle ]; then
 timeout --kill-after=3s 40s "$PROBE" 30 > "$OUT/counters.csv" 2> "$OUT/counters.log"
 printf 'probe_status=0\ngame_status=not-run\n' > "$OUT/status.txt"
 exit 0
fi
sampler=
cleanup(){ if [ -n "$sampler" ]; then kill "$sampler" 2>/dev/null || true; wait "$sampler" 2>/dev/null || true; fi; }
trap cleanup EXIT HUP INT TERM
if [ "$CASE" != baseline ]; then
 timeout --kill-after=3s 55s "$PROBE" 45 > "$OUT/counters.csv" 2> "$OUT/counters.log" & sampler=$!
fi
export XDG_RUNTIME_DIR=/run/0-runtime-dir WAYLAND_DISPLAY=wayland-1 SWAYSOCK=/run/0-runtime-dir/sway-ipc.0.sock SDL_VIDEODRIVER=wayland PULSE_SERVER=unix:/run/0-runtime-dir/pulse/native
set -- "$GAME" --assets /storage/apps/falllinie-spike/assets/shaders --seconds 30 --dual-screen --place --autoplay --muted --source-vinyl --audio-mode buffered --audio-buffer 1024 --dev-overlay --seed 42 --no-vsync --fps 60 --render-size 384x288 --profile "$OUT/frames.csv" --paired-clocks --profile-cadence 60 --gpu-profile-mode off --receipt "$OUT/receipt.json"
[ "$CASE" != bypass ] || set -- "$@" --post-bypass
printf '%s\n' "$@" > "$OUT/argv.txt"
game_status=0; timeout --kill-after=5s 55s "$@" > "$OUT/game.log" 2>&1 || game_status=$?
probe_status=not-run
if [ -n "$sampler" ]; then probe_status=0; wait "$sampler" || probe_status=$?; sampler=; fi
printf 'probe_status=%s\ngame_status=%s\n' "$probe_status" "$game_status" > "$OUT/status.txt"
date -u +%FT%TZ > "$OUT/end.txt"
[ "$game_status" = 0 ] && { [ "$probe_status" = 0 ] || [ "$probe_status" = not-run ]; }
