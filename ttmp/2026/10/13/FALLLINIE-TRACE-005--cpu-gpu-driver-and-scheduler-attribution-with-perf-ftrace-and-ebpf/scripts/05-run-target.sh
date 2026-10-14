#!/bin/sh
# Complete live workload cases, invoked by the parent. No system-wide attachment.
# Usage: sh 05-run-target.sh BIN ASSETS NEW_OUT CASE SOURCE_REV ORDER
set -eu
[ "$#" = 6 ] || exit 2
BIN=$1 ASSETS=$2 OUT=$3 CASE=$4 REV=$5 ORDER=$6
case "$CASE" in baseline|clocks|view|passes|perf-stat|perf-record) ;; *) exit 2;; esac
SECONDS_PER_RUN=${SECONDS_PER_RUN:-30}
case "$SECONDS_PER_RUN" in 30|60|120) ;; *) exit 2;; esac
mkdir "$OUT"
printf 'source_rev=%s\ncase=%s\norder=%s\nseconds=%s\nwarmup_frames=60\n' "$REV" "$CASE" "$ORDER" "$SECONDS_PER_RUN" > "$OUT/metadata.txt"
sha256sum "$BIN" > "$OUT/binary.sha256"
snapshot() {
 date -u +%FT%TZ
 awk '/^MemAvailable:|^SwapTotal:/ {print}' /proc/meminfo
 for f in /sys/devices/system/cpu/cpufreq/policy*/scaling_cur_freq /sys/devices/system/cpu/cpufreq/policy*/scaling_governor /sys/class/thermal/thermal_zone*/temp; do
  [ ! -r "$f" ] || { printf '%s=' "$f"; cat "$f"; }
 done
}
cat /proc/sys/kernel/random/boot_id > "$OUT/boot-id.txt"
snapshot > "$OUT/before.txt"
set -- "$BIN" --assets "$ASSETS" --seconds "$SECONDS_PER_RUN" --no-vsync --fps 60 --seed 42 --cell 0 --cells 3 --render-size 384x288 --dual-screen --place --autoplay --muted --audio-mode buffered --audio-buffer 1024 --source-vinyl --dev-overlay --profile "$OUT/frames.csv" --profile-cadence 60 --receipt "$OUT/receipt.json"
case "$CASE" in baseline) ;; *) set -- "$@" --paired-clocks;; esac
case "$CASE" in view|passes) set -- "$@" --gpu-profile-mode "$CASE";; *) set -- "$@" --gpu-profile-mode off;; esac
case "$CASE" in
 perf-stat) set -- perf stat -x , -o "$OUT/perf-stat.csv" -e cpu-clock,context-switches,cpu-migrations,page-faults -- "$@";;
 perf-record) set -- perf record --clockid mono --no-buildid-cache -e cpu-clock -F 99 -m 128 --call-graph fp -o "$OUT/perf.data" -- "$@";;
esac
printf '%s\n' "$@" > "$OUT/argv-lines.txt"
status=0
timeout --kill-after=5s "$((SECONDS_PER_RUN+30))s" "$@" > "$OUT/stdout.log" 2>&1 || status=$?
printf 'exit_status=%s\n' "$status" >> "$OUT/metadata.txt"
snapshot > "$OUT/after.txt"
if [ "$CASE" = perf-record ] && [ "$status" = 0 ]; then
 # Only this workload's sampling; raw stacks stay local and are not committed.
 perf report -i "$OUT/perf.data" --stdio --header-only > "$OUT/perf-header.txt" 2>&1
 perf report -i "$OUT/perf.data" --stdio --no-inline -g none --no-children --sort pid,comm,dso,symbol --percent-limit 0.5 > "$OUT/perf-report.txt" 2>&1
 perf report -i "$OUT/perf.data" --stdio --no-inline -g none --stats > "$OUT/perf-statistics.txt" 2>&1
fi
exit "$status"
