#!/bin/sh
# Bounded main-thread PMU case; no process-wide inherited counters.
set -eu
ROOT=$1
NAME=$2
PAIR=$3
OUT=$ROOT/evidence/$NAME
mkdir -p "$OUT"
export XDG_RUNTIME_DIR=/run/0-runtime-dir WAYLAND_DISPLAY=wayland-1 SDL_VIDEODRIVER=wayland
export SWAYSOCK=/run/0-runtime-dir/sway-ipc.0.sock
app=
cleanup(){ if [ -n "$app" ] && kill -0 "$app" 2>/dev/null; then kill -TERM "$app"; wait "$app" || true; fi; }
trap cleanup EXIT INT TERM
"$ROOT/falllinie-pmu" --assets /storage/apps/falllinie-spike/assets/shaders --seconds 32 --no-vsync --fps 60 --seed 42 --cells 3 --render-size 384x288 --dual-screen --place --autoplay --no-audio --dev-overlay --profile "$OUT/frames.csv" --paired-clocks --profile-cadence 60 --receipt "$OUT/receipt.json" > "$OUT/game.log" 2>&1 &
app=$!
echo "$app" > "$OUT/main-tid.txt"
sleep 6
kill -0 "$app"
for f in /proc/$app/task/*/comm; do echo "$f"; cat "$f"; done > "$OUT/threads.txt"
cat /proc/$app/status > "$OUT/process-status.txt"
cat /sys/devices/system/cpu/cpufreq/policy0/scaling_governor /sys/devices/system/cpu/cpufreq/policy0/scaling_cur_freq > "$OUT/cpu-before.txt"
status=0
"$ROOT/pmu-clock" > "$OUT/collection-start-ns.txt"
if [ "$PAIR" = none ]; then
 sleep 20
elif [ "$PAIR" = sample ]; then
 perf record --no-inherit -t "$app" --no-buildid-cache -e armv8_cortex_a55/cpu_cycles/ -F 199 --call-graph fp -m 128 -o "$OUT/perf.data" -- sleep 20 > "$OUT/perf-record.log" 2>&1 || status=$?
else
 EVENTS=$(printf '%s' "$PAIR" | awk -F, '{printf "{armv8_cortex_a55/%s/,armv8_cortex_a55/%s/}",$1,$2}')
 printf 'perf stat --no-inherit -t %s -I 1000 -x , -e %s -- sleep 20\n' "$app" "$EVENTS" > "$OUT/command.txt"
 perf stat --no-inherit -t "$app" -I 1000 -x , -o "$OUT/perf-stat.csv" -e "$EVENTS" -- sleep 20 > "$OUT/perf-stat.log" 2>&1 || status=$?
fi
"$ROOT/pmu-clock" > "$OUT/collection-end-ns.txt"
echo "$status" > "$OUT/perf-status.txt"
cat /sys/devices/system/cpu/cpufreq/policy0/scaling_cur_freq > "$OUT/cpu-after.txt"
wait "$app"; app=
if [ "$PAIR" = sample ] && [ "$status" = 0 ]; then
 perf report -i "$OUT/perf.data" --stdio --header-only > "$OUT/perf-header.txt" 2>&1
 perf report -i "$OUT/perf.data" --stdio --no-inline -g none --no-children --sort pid,comm,dso,symbol --percent-limit 0.5 > "$OUT/perf-report.txt" 2>&1
 perf report -i "$OUT/perf.data" --stdio --no-inline -g none --stats > "$OUT/perf-statistics.txt" 2>&1
 perf report -i "$OUT/perf.data" --stdio --no-inline -g none --no-children --sort pid,dso --percent-limit 0 > "$OUT/perf-dso.txt" 2>&1
 # Bounded raw data stays on device; preserve reports and byte hash locally.
 sha256sum "$OUT/perf.data" > "$OUT/perf-data.sha256"
fi
exit "$status"
