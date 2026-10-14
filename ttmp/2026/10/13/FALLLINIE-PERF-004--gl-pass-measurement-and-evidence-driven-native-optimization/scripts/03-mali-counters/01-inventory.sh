#!/bin/sh
# Read-only GPU identity/interface inventory. No ioctls, collectors or settings writes.
set -eu
date -u +%FT%TZ
uname -smr
printf 'device-tree model='; tr '\000' ' ' < /proc/device-tree/model; printf '\n'
printf 'device-tree compatible='; tr '\000' ' ' < /proc/device-tree/compatible; printf '\n'
ls -l /dev/mali* 2>/dev/null || true
for f in /sys/module/mali_kbase/version /sys/module/mali_kbase/srcversion /sys/module/mali_kbase/taint; do
 [ ! -r "$f" ] || { printf '%s=' "$f"; cat "$f"; }
done
modinfo mali_kbase 2>/dev/null | grep -E '^(filename|version|srcversion|vermagic|description|parm):' || true
for d in /sys/class/misc/mali*/device /sys/devices/platform/*gpu*; do
 [ -d "$d" ] || continue
 printf 'gpu_device=%s\n' "$d"; ls -1 "$d"
 for name in gpuinfo gpu_id core_mask available_frequencies clock scaling_cur_freq; do
  [ ! -r "$d/$name" ] || { printf '%s/%s=' "$d" "$name"; head -c 4096 "$d/$name"; printf '\n'; }
 done
done
for d in /sys/class/devfreq/*; do
 [ -d "$d" ] || continue
 echo "devfreq=$d"
 for f in name governor cur_freq available_frequencies trans_stat; do
  [ ! -r "$d/$f" ] || { printf '%s=' "$f"; head -c 4096 "$d/$f"; printf '\n'; }
 done
done
printf 'driver_libraries:\n'
find /usr/lib /usr/lib64 -maxdepth 2 -name '*mali*' -print 2>/dev/null || true
for f in /usr/lib/libmali.so*; do
 [ ! -f "$f" ] || sha256sum "$f"
done
printf 'existing_capture_tools:\n'
for tool in gatord gator gator.ko malioc renderdoccmd; do command -v "$tool" || true; done
printf 'relevant_active_processes:\n'; pgrep -af '[f]alllinie|[g]atord|[h]wcpipe|[m]ali-counter' || true
printf 'gpu_boot_messages:\n'; dmesg | grep -iE 'mali|panfrost|panthor' | tail -40 || true
