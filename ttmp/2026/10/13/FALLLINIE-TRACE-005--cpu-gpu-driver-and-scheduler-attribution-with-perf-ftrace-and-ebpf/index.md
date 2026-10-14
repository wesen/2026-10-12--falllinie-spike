---
Title: CPU GPU driver and scheduler attribution with perf ftrace and eBPF
Ticket: FALLLINIE-TRACE-005
Status: active
Topics: [performance]
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Textbook-style tracing publication and bounded future attribution plan; no new runtime tracing implemented or executed.
LastUpdated: 2026-10-13T21:25:00-04:00
WhatFor: Separate render CPU execution from blocking, scheduler delay and GPU elapsed measurements before optimizing.
WhenToUse: For tracing capability approval, measurement interpretation and continuation of the published research.
---

# CPU, GPU, driver and scheduler attribution

This documentation ticket explains which questions paired thread clocks, perf samples, scheduler events and bounded driver/BPF tracing can answer. Existing render `cpu_ms` is wall duration, not charged CPU execution; GPU elapsed queries are not utilization. Target tracing capabilities and causal driver/GPU attribution remain unverified.

- [Design and experiment contract](design-doc/01-tracing-architecture-evidence-and-bounded-experiment-plan.md): source evidence, correlation architecture, finite phases, privacy, rollback and acceptance.
- [Investigation/publication diary](reference/01-tracing-investigation-and-publication-diary.md): actual user request, research/validation failures, editorial gate and publication receipt.
- [Scripts and provenance](scripts/00-publication-helpers.md): offline research, original figures and publication checks.
- [Tasks](tasks.md): documentation/publication completion is separate from the still-open target capability/tracing gate.

Vault note: `Projects/2026/10/13/PROJECT REPORT - Falllinie Tracing - CPU Execution, Driver Waits and GPU Attribution.md` in `/home/manuel/code/wesen/go-go-golems/go-go-parc`.

PERF-004 owns the separate pass-measurement implementation and final measurement report. This ticket reads a bounded parent-collected excerpt with read-time hashes, without editing or staging its untracked target files. No source/shader/audio change, kernel trace, tool installation, policy change or native push is authorized by this publication.
