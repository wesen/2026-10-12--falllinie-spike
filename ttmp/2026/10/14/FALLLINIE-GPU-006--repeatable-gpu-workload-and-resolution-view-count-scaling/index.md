---
Title: Repeatable GPU workload and resolution view-count scaling
Ticket: FALLLINIE-GPU-006
Status: complete
Topics:
    - performance
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-10-14T14:20:19.2632681-04:00
WhatFor: ""
WhenToUse: ""
---

# Repeatable GPU workload and resolution view-count scaling

This ticket contains the delivered intern guide, opt-in deterministic live replay, and the actual resolution/view-count scaling experiment. Production rendering defaults remain unchanged.

- [Intern guide and implementation contract](design-doc/01-intern-guide-to-repeatable-gpu-benchmarking-architecture-and-implementation.md)
- [Measured device results and interpretation](analysis/01-measured-replay-resolution-and-view-count-scaling-on-mali-g52.md)
- [Detailed implementation and investigation diary](reference/01-gpu-benchmark-implementation-diary.md)
- [Reproduction commands and tooling](scripts/README.md)
- [Task checklist](tasks.md)

The pre-implementation guide was delivered to reMarkable at `/ai/2026/10/14/FALLLINIE-GPU-006`. The measured-results document is the authoritative current result; original failure evidence is retained in the diary and evidence directory.
