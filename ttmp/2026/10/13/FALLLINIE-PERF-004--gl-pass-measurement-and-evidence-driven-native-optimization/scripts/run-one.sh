#!/bin/sh
# Parent-invoked local device helper. Never SSH/install/kill/reconfigure services.
# Usage: sh run-one.sh BINARY ASSETS OUTPUT_DIR CASE SOURCE_REV ORDER
set -eu
[ "$#" = 6 ] || { echo 'usage: run-one.sh BINARY ASSETS NEW_OUTPUT_DIR CASE SOURCE_REV ORDER' >&2; exit 2; }
BIN=$1 ASSETS=$2 OUT=$3 CASE=$4 REV=$5 ORDER=$6
case "$BIN:$ASSETS:$OUT" in /*:/*:/*) ;; *) echo 'binary, assets and output paths must be absolute' >&2; exit 2;; esac
SECONDS_PER_RUN=${SECONDS_PER_RUN:-30}
case "$SECONDS_PER_RUN" in 10|20|30|60|120) ;; *) echo 'SECONDS_PER_RUN must be 10,20,30,60,120' >&2; exit 2;; esac
case "$CASE" in static-off|static-passes|static-single|static-bypass|static-res|live-off|live-passes) ;; *) echo 'unknown case' >&2; exit 2;; esac
mkdir "$OUT" # refuse accidental overwrite; caller creates parent directory
printf 'source_rev=%s\ncase=%s\norder=%s\nseconds=%s\nwarmup_frames=60\n' "$REV" "$CASE" "$ORDER" "$SECONDS_PER_RUN" > "$OUT/metadata.txt"
sha256sum "$BIN" > "$OUT/binary.sha256"
snapshot() {
 date -u
 uname -a
 cat /proc/meminfo
 for p in /sys/devices/system/cpu/cpufreq/policy*; do
  [ -d "$p" ] || continue
  for f in scaling_cur_freq scaling_min_freq scaling_max_freq scaling_governor cpuinfo_max_freq; do
   [ ! -r "$p/$f" ] || { printf '%s=' "$p/$f"; cat "$p/$f"; }
  done
 done
 for p in /sys/class/thermal/thermal_zone*/temp; do [ ! -r "$p" ] || { printf '%s=' "$p"; cat "$p"; }; done
}
snapshot > "$OUT/before.txt"
set -- --assets "$ASSETS" --seconds "$SECONDS_PER_RUN" --no-vsync --fps 60 --seed 42 --cell 0 --cells 3 --render-size 384x288 --profile "$OUT/frames.csv" --profile-cadence 60 --receipt "$OUT/receipt.json"
case "$CASE" in
 static-*) set -- "$@" --freeze-scene;;
 live-*) set -- "$@" --autoplay --muted --audio-mode buffered --audio-buffer 1024 --source-vinyl --dev-overlay;;
esac
case "$CASE" in static-single) ;; *) set -- "$@" --dual-screen;; esac
case "$CASE" in static-off|live-off) set -- "$@" --gpu-profile-mode off;; *) set -- "$@" --gpu-profile-mode passes;; esac
case "$CASE" in static-bypass) set -- "$@" --post-bypass;; static-res) set -- "$@" --render-size 512x384;; esac
# Opt-in placement belongs to parent environment, not a default service operation.
if [ "${PLACE:-0}" = 1 ]; then set -- "$@" --place; fi
printf '%s\n' "$BIN" "$@" > "$OUT/argv-lines.txt"
status=0
"$BIN" "$@" > "$OUT/stdout.log" 2>&1 || status=$?
printf 'exit_status=%s\n' "$status" >> "$OUT/metadata.txt"
snapshot > "$OUT/after.txt"
exit "$status"
