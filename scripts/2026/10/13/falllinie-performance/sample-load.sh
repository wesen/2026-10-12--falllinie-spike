#!/bin/sh
# Read-only 1 Hz system facts; no governors, registers or tracing policies are changed.
PID="$1"
while kill -0 "$PID" 2>/dev/null; do
 printf 'time '; date +%s
 head -n 1 /proc/stat
 printf 'process '; cat "/proc/$PID/stat" 2>/dev/null || true
 grep -E '^(MemAvailable|SwapTotal|SwapFree):' /proc/meminfo
 grep -E '^(pgmajfault|pswpin|pswpout) ' /proc/vmstat
 for pressure in /proc/pressure/cpu /proc/pressure/memory /proc/pressure/io; do test ! -r "$pressure" || { printf '%s ' "$pressure"; head -n 1 "$pressure"; }; done
 for f in /sys/devices/system/cpu/cpufreq/policy*/scaling_cur_freq /sys/class/devfreq/*/cur_freq /sys/class/thermal/thermal_zone*/temp; do
  printf '%s ' "$f"; cat "$f" 2>/dev/null || true
 done
 sleep 1
done
