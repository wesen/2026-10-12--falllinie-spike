#!/bin/sh
set -eu
uname -a
perf --version
cat /proc/sys/kernel/perf_event_paranoid
ls /sys/bus/event_source/devices
D=/sys/bus/event_source/devices/armv8_cortex_a55
for f in "$D/type" "$D"/events/* "$D"/format/*; do echo "FILE $f"; cat "$f"; done
# Explicit PMU avoids the unrelated DDR PMU's cycles alias.
perf stat -x , -e '{armv8_cortex_a55/cpu_cycles/,armv8_cortex_a55/inst_retired/}' -- sleep 0.1
