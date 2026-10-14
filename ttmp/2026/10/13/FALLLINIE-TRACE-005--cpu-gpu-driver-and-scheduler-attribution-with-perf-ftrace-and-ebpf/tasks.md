# Tasks

## TODO
- [x] Document CPU wall versus thread time, GPU timing limitations and proposed perf ftrace eBPF attribution experiments <!-- t:j3sb -->
- [x] Publish validated textbook-style tracing deep dive to go-go-parc with scoped commit and remote verification <!-- t:ymp7 -->
- [x] Verify actual device tracing capabilities and collect bounded PID/TID/monotonic-identified perf recording; scheduler/driver wait attribution remains unavailable, not causally established <!-- t:gcop -->
- [x] Implement reviewed opt-in paired render-thread CPU/wall timing with strict schema and validity tests <!-- t:d8cq -->
- [x] Qualify paired clocks and supported perf CPU attribution on device with matched baseline cases and loss accounting; no quantified overhead bound claimed <!-- t:90uh -->
- [ ] Remaining capability limit: distinguish blocked versus runnable delay and identify exact driver/fence waits; current kernel FTRACE/KPROBES disabled, no kernel/policy changes authorized
- [ ] Remaining GPU question: explain impossible startup elapsed values and nonadditive postwarmup query totals before causal per-pass cost ranking
