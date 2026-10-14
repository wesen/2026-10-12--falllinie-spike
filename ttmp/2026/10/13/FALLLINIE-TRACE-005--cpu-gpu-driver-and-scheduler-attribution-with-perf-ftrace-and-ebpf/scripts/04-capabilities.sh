#!/bin/sh
# Read-only: no perf/BPF feature probes, attachments, mounts or settings writes.
set -eu
printf 'inventory_utc='; date -u +%FT%TZ
uname -smr
for tool in perf trace-cmd bpftrace bpftool timeout; do
 printf 'tool_%s=' "$tool"; command -v "$tool" || true
done
if command -v perf >/dev/null 2>&1; then perf --version; fi
for p in /proc/sys/kernel/perf_event_paranoid /proc/sys/kernel/kptr_restrict /proc/sys/kernel/unprivileged_bpf_disabled /sys/kernel/security/lockdown; do
 if [ -r "$p" ]; then printf '%s=' "$p"; cat "$p"; else printf 'unavailable=%s\n' "$p"; fi
done
awk '/^MemTotal:|^MemAvailable:|^SwapTotal:/ {print}' /proc/meminfo
if [ -e /sys/kernel/btf/vmlinux ]; then ls -l /sys/kernel/btf/vmlinux; else echo BTF_vmlinux_absent; fi
if [ -r /proc/config.gz ]; then
 zcat /proc/config.gz | grep -E '^(CONFIG_(BPF|BPF_SYSCALL|BPF_JIT|BPF_EVENTS|DEBUG_INFO_BTF|PERF_EVENTS|FTRACE|FUNCTION_TRACER|FUNCTION_GRAPH_TRACER|KPROBES|UPROBES|KPROBE_EVENTS|UPROBE_EVENTS|TRACING|TRACEPOINTS|SCHEDSTATS|SCHED_DEBUG)=|# CONFIG_(BPF|BPF_SYSCALL|BPF_JIT|BPF_EVENTS|DEBUG_INFO_BTF|PERF_EVENTS|FTRACE|FUNCTION_TRACER|FUNCTION_GRAPH_TRACER|KPROBES|UPROBES|KPROBE_EVENTS|UPROBE_EVENTS|TRACING|TRACEPOINTS|SCHEDSTATS|SCHED_DEBUG) is not set)' || true
else echo kernel_config_unavailable; fi
for root in /sys/kernel/tracing /sys/kernel/debug/tracing; do
 [ -d "$root" ] || continue
 echo "trace_root=$root"
 for f in tracing_on current_tracer trace_clock buffer_size_kb set_event; do
  if [ -r "$root/$f" ]; then printf '%s=' "$f"; head -c 2048 "$root/$f"; printf '\n'; fi
 done
 if [ -r "$root/available_events" ]; then
  grep -E '^(sched:(sched_switch|sched_wakeup|sched_wakeup_new|sched_process_exit)|syscalls:(sys_enter_ioctl|sys_exit_ioctl)|mali[^:]*:|gpu[^:]*:|dma_fence:)' "$root/available_events" | head -100 || true
 fi
 if [ -d "$root/instances" ]; then printf 'existing_instances:\n'; ls -1 "$root/instances"; fi
 for event in sched/sched_switch sched/sched_wakeup sched/sched_process_exit; do
  if [ -r "$root/events/$event/format" ]; then echo "format=$event"; cat "$root/events/$event/format"; fi
 done
done
printf 'GPU_modules:\n'; grep -Ei 'mali|panfrost|panthor' /proc/modules || true
printf 'active_falllinie:\n'; pgrep -a -f '^(/[^ ]*/)?falllinie-(spike|perf004)( |$)' || true
