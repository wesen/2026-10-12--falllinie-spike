#!/usr/bin/env python3
"""Broaden the requested MOC; preserve the Falllinie catalogue and old report bodies."""
from pathlib import Path
import json,re
P=Path(__file__).resolve().parent
V=Path('/home/manuel/code/wesen/go-go-golems/go-go-parc')
D=V/'Projects/2026/10/13'
old=D/'MOC - Falllinie Optimization Reports.md'
new=D/'MOC - Optimization and Performance Engineering.md'
assert old.exists() and not new.exists()
notes=list(V.rglob('*.md'))
selected=[]
def link(stem,label,backlink=True):
    matches=[x for x in notes if x.stem==stem];assert len(matches)==1,stem
    if backlink:selected.append(matches[0])
    return f'[[{stem}|{label}]]'
screencast=link('ARTICLE - Screencast Studio - Performance Investigation Approaches and Tricks','Screencast investigation methods')
x264=link('01-REPORT-x264-Go-CGO-Page-Fault-Root-Cause-Analysis','x264 / Go / CGO page-fault investigation')
resize=link('ARTICLE - Measuring Before Optimizing - An X11 Window Manager Resize Path','X11 resize investigation')
paint=link('ARTICLE - Optimizing an X11 Window Manager Paint Path','X11 paint-path optimization')
memory=link('ARTICLE - Publish Vault Memory Optimization - From OOM Incidents to Phase-Attributed Baselines','Publish Vault memory investigation')
metrics=link('ARTICLE - Screencast Studio - Prometheus Metrics Architecture and Field Guide','Screencast metrics field guide')
wifi=link('ARTICLE - Optimizing WiFi Image Upload on ESP32-P4 - From 6 Seconds to Sub-Second','ESP32-P4 Wi-Fi image upload')
spi=link('ARTICLE - ESP32-P4 PicoCalc Display Optimization - Queued SPI and Dirty Rectangles','PicoCalc queued SPI and dirty rectangles')
sql=link('ARTICLE - SQLite in the Browser - Measuring and Fixing sql.js Performance in Static Code Review Sites','Browser SQLite query optimization')
gojs=link('ARTICLE - Goja Site Rendering Performance Internals and Export Boundary Deep Dive','Goja rendering and export-boundary costs')
sim=link('PROJECT REPORT - researchctl CPU GPU Codesign Experiment Runtime Deep Dive','researchctl CPU/GPU experiment runtime')
jsdiscussion=link('CHATGPT TRANSCRIPT - JavaScript Interpreter Instrumentation — Goja eBPF','Goja/eBPF instrumentation conversation',False)
wifidiscussion=link('CHATGPT TRANSCRIPT - eBPF for Wi-Fi Disconnects','Wi-Fi disconnect tracing conversation',False)
intro=f'''---
title: Optimization and Performance Engineering
aliases: [Falllinie performance reading map, MOC - Falllinie Optimization Reports, Optimization MOC]
type: article
status: active
created: 2026-10-13
tags: [moc, performance, optimization, profiling, ebpf]
---

# Optimization and performance engineering

A question-oriented map of performance investigations across the vault: Linux profiling and eBPF, memory attribution, graphics and display pipelines, runtime boundaries, database queries, and experimental methodology. The Falllinie reports form a detailed case-study series below; they are no longer the scope of the whole index.

> [!summary]
> Begin with a defined workload and quantity. Distinguish execution, waiting, allocation, traffic and static estimates. Read corrections and rejected experiments alongside successful changes. A tracing proposal, simulation or archived conversation is not a measured production result.

## Start with the question, not the tool

| Question | Start here | Important distinction |
|---|---|---|
| Is the benchmark measuring the real workload? | {screencast}; {resize} | Synthetic clients and nested display servers can change the conclusion. |
| Is time spent executing, blocked, or runnable? | Falllinie tracing/rendering series below; off-CPU resources below | CPU samples and wall-minus-CPU residuals do not identify the same thing. |
| Are page faults or memory growth actually causing the slowdown? | {x264}; {memory} | Resident memory, heap allocation, faults and sustained CPU work require separate attribution. |
| Which pixels or bytes need to be processed at all? | {paint}; {spi}; {wifi} | Work avoidance, transfer granularity and queue ownership can matter more than instruction tuning. |
| Is the runtime boundary or query shape expensive? | {gojs}; {sql} | Measure conversion, allocation and query expansion rather than assuming the runtime itself is the cause. |
| What should be instrumented continuously? | {metrics} | Operational metrics locate changes; targeted profiling answers narrower causal questions. |

## Linux profiling, eBPF and corrected causal explanations

The strongest older eBPF-related investigation found is {x264} (April 2026), under `Research/Institute/Technical Reports/`, rather than the Projects folder. It records uprobes, bpftrace fault stacks, allocator comparisons and memory-map analysis. **Read its two errata first.** The initial CGO/malloc explanation was disproved; the later buffer-pool/page-fault explanation described real memory behavior but did not explain sustained CPU load. The final CPU diagnosis was silently rejected encoder property settings, leaving a more expensive default configuration active. Some lower sections preserve earlier interpretations: the errata is essential context, not an optional footnote.

Pair that report with {screencast}, which explains reproducible workloads, real-browser versus synthetic baselines and why disproven hypotheses remain useful. The Falllinie tracing → rendering → GPU sequence below provides a newer example of choosing available instrumentation on a constrained kernel rather than assuming every eBPF or scheduler facility exists.

Two archived conversations broaden the questions but are **discussion sources, not independently qualified experiments**: {jsdiscussion} (July 2026) and {wifidiscussion} (August 2026). Use them for investigation ideas and then consult implementation evidence and tool documentation before treating suggested probes as working measurements.

## Memory, allocation and observability

{memory} distinguishes Go heap from RSS and attributes peak usage to load/index phases. It records both completed representation/indexing changes and remaining uncertainty inside index construction. This is a useful companion to the x264 report: neither a large virtual mapping nor a high fault count is, by itself, a complete explanation of runtime cost.

{metrics} covers the application's metrics architecture. Read it alongside a targeted profiler investigation rather than treating metrics, CPU profiles, execution traces and heap snapshots as substitutes for one another. The Falllinie buffered-audio report below adds ownership and scheduling constraints: moving work off a deadline-sensitive callback does not remove that work from the process.

## Graphics, display and transfer costs

- {resize} follows changing diagnoses, a misleading harness, decomposition and bounded image-difference decisions.
- {paint} records work avoidance, glyph caching, backing-store strategies, rejected changes and differences between Xephyr and the target display stack.
- {spi} connects clock-source selection, transfer size, queued DMA lifetime and dirty-region geometry. Its queued-transfer ownership invariant matters as much as the throughput number.
- {wifi} separates payload compression, decompressor stack requirements and timeout configuration, while keeping future transfer ideas distinct from implemented changes.

Compare these with Falllinie's GPU reports below: fewer source operations, a smaller transfer or an apparently cheaper path is a hypothesis until the complete target workload supports it. Explicitly accepted small visual differences in one project do not grant permission to relax another project's fidelity contract.

## Runtime boundaries, database work and simulation

{gojs} profiles JavaScript object construction and `goja.Value.Export()` at the native UI boundary, and broadens benchmarks to avoid optimizing only one page. {sql} shows how compatibility-view expansion and query shape can dominate perceived browser responsiveness; moving execution to a worker and reducing query work address different problems.

{sim} documents a deterministic simulator, event-derived metrics and durable experiment manifests. It is valuable for experimental architecture and reproducibility, but its simulated execution is not evidence of a measured hardware speedup. Likewise, Falllinie's isolated ARM64 JavaScript embeddings below do not establish integrated game-frame headroom.

## Online references: choose the instrument for the quantity

These are external references, not additional local experiments. Search-discovered methodology links and official tool/project documentation were selected for the questions covered by this MOC; tool availability and syntax still depend on the target's versions and kernel configuration.

### Methodology and CPU attribution

- [Brendan Gregg — USE Method](https://www.brendangregg.com/usemethod.html): utilization, saturation and errors as a systematic resource investigation checklist.
- [Brendan Gregg — Linux perf examples](https://www.brendangregg.com/perf.html): CPU sampling, counters and tracing examples; select events appropriate to the claim.
- [Brendan Gregg — Off-CPU Analysis](https://www.brendangregg.com/offcpuanalysis.html): investigating time not executing, with contextual stacks and explicit scope.
- [Brendan Gregg — Flame Graphs](https://www.brendangregg.com/flamegraphs.html): interpretation of aggregated stacks; horizontal position is not a time axis.

### eBPF and language-runtime diagnostics

- [bpftrace official project](https://github.com/bpftrace/bpftrace) and [versioned 0.22 reference](https://bpftrace.org/docs/0.22): probe types, language and aggregation. Select documentation matching the installed release; this link is not a claim that 0.22 is current.
- [BCC project and tools](https://github.com/iovisor/bcc): Linux BPF-based tracing utilities and examples. Check kernel support, permissions and collection overhead before use.
- [Go diagnostics](https://go.dev/doc/diagnostics): profiling, tracing and runtime diagnostics; choose CPU, heap, blocking or execution-trace evidence according to the question.

### GPU counters and static shader analysis

- [Arm libGPUCounters](https://github.com/ARM-software/libGPUCounters): hardware counter collection; product-specific event definitions and derived denominators are part of interpretation.
- [Pinned Mali-G52 counter reference used by Falllinie](https://github.com/ARM-software/libGPUCounters/blob/378c7b59e2dae87cbbffb0ac9cf96ed13c939823/docs/mali-g52_counter_reference.html): the definitions associated with that actual campaign, rather than an unqualified latest mapping.
- [Arm Mali Offline Compiler](https://developer.arm.com/tools-and-software/mali-offline-compiler): target-specific shader resource estimates. Static cycles, register allocation and occupancy guide experiments; they do not replace live timing or appearance checks.

## Falllinie case-study series

'''
body=old.read_text().split('# Falllinie optimization reports\n',1)[1].lstrip()
body=body.replace('This index connects the eight reports','This case-study series connects the eight reports',1)
body=re.sub(r'^(#{2,5}) ',r'\1# ',body,flags=re.M)
new.write_text(intro+body)
old.unlink()
for path in (V/'Projects').rglob('PROJECT REPORT - Falllinie *.md'):
    s=path.read_text();s=s.replace('[[MOC - Falllinie Optimization Reports|','[[MOC - Optimization and Performance Engineering|');path.write_text(s)
for path in dict.fromkeys(selected):
    s=path.read_text();assert '## Optimization knowledge network' not in s
    path.write_text(s+'\n\n## Optimization knowledge network\n\nNavigation addendum: [[MOC - Optimization and Performance Engineering|Optimization and Performance Engineering MOC]] connects this historical report to related CPU/eBPF, memory, graphics, runtime-boundary and Falllinie investigations, plus external tool references. The findings above are unchanged; consult any existing errata before reusing causal conclusions.\n')
changed=[str(new.relative_to(V)),str(old.relative_to(V))]+[str(p.relative_to(V)) for p in dict.fromkeys(selected)]
(P/'generalization-files.json').write_text(json.dumps(changed,indent=2)+'\n')
print(f'Created general MOC; preserved Falllinie section; added {len(set(selected))} older-report backlinks.')
