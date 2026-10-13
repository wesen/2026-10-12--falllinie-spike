#!/usr/bin/env bash
# Parent-only invocation AFTER review/approval. Runs ONLY this trusted private bundle.
set -euo pipefail
P=$(cd -- "$(dirname -- "$0")" && pwd)
[[ $(uname -m) == aarch64 ]] || { echo 'REFUSED: target must be aarch64' >&2; exit 2; }
blocks=${1:-300}; [[ $blocks =~ ^[0-9]+$ ]] && ((blocks>=1 && blocks<=600)) || exit 2
available=$(awk '/^MemAvailable:/ {print $2}' /proc/meminfo)
if ! [[ $available =~ ^[0-9]+$ ]] || ((available<262144)); then echo 'REFUSED: need >=256MiB current MemAvailable' >&2; exit 2; fi
LOCK="$(dirname -- "$P")/.probe-lock"
mkdir "$LOCK" || { echo 'REFUSED: another probe owns lock; do not remove while running' >&2; exit 2; }
pid=''
cleanup(){ rmdir "$LOCK"; }
stop(){ if [[ -n $pid ]]; then kill -TERM "$pid" 2>/dev/null || true; wait "$pid" 2>/dev/null || true; fi; exit 130; }
trap cleanup EXIT; trap stop INT TERM
cd "$P"
sha256sum -c SHA256SUMS
[[ ! -e result.log ]] || { echo 'REFUSED: preserve existing result.log before rerun' >&2; exit 2; }
# The bundle never contains libc or a loader; target's own loader remains authoritative.
{
 date -u +%FT%TZ; uname -smr; getconf GNU_LIBC_VERSION
 echo "target_execution=ACTUAL parent_invoked=true sequential_lock=true blocks=$blocks timeout_seconds=180 RSS_gate_kib=196608"
 echo "pre_run_memavailable_kib=$available"
 awk '/^MemTotal:|^MemAvailable:|^SwapTotal:/ {print}' /proc/meminfo
 awk '/^Features|^CPU implementer|^CPU architecture|^CPU variant|^CPU part|^CPU revision/ {print; if(++n==36)exit}' /proc/cpuinfo
 for cpu in /sys/devices/system/cpu/cpu[0-9]*; do
  for f in scaling_cur_freq scaling_governor; do
   if [[ -r $cpu/cpufreq/$f ]]; then printf '%s %s=' "${cpu##*/}" "$f"; cat "$cpu/cpufreq/$f"; fi
  done
 done
 env -u LD_PRELOAD LD_LIBRARY_PATH="$P/lib" ldd ./probe
} > preflight.log 2>&1
# No arbitrary command arguments, privileged operations, service changes or process scans.
start=$SECONDS
set +e
env -u LD_PRELOAD LD_LIBRARY_PATH="$P/lib" timeout --kill-after=5s 180s ./probe ./pure.js ./probe.js "$blocks" > result.log 2>&1 &
pid=$!; wait "$pid"; rc=$?; pid=''
set -e
printf 'process_elapsed_integer_seconds=%s exit=%s includes_platform_and_teardown=true\n' "$((SECONDS-start))" "$rc" > process-time.log
cat process-time.log
[[ $rc == 0 ]] || exit "$rc"
grep -q '^PROBE PASS$' result.log
# Actual measurements must still be copied back and checked with 04-compare.py.
echo 'TARGET PROCESS PASS; independent oracle comparison remains required'
