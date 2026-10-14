---
Title: Shadow footprint rejection and fidelity-bounded scene optimization
Ticket: FALLLINIE-GPU-008
Status: complete
Topics:
    - performance
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-10-14T16:08:26.375228411-04:00
WhatFor: ""
WhenToUse: ""
---

# Shadow footprint rejection and fidelity-bounded scene optimization

Designed the five proposed scene-reduction options and implemented the first image-preserving candidate: conservative shadow-footprint rejection. Original remains the default; shadow-off is an explicit non-parity diagnostic.

- [Design and scope](design-doc/01-scene-cost-reduction-options-and-conservative-shadow-footprint.md)
- [Measured results, mathematical derivation and screenshots](analysis/01-measured-shadow-footprint-optimization-and-screenshot-qualification.md)
- [Detailed chronological diary with actual screenshots](reference/01-shadow-optimization-and-visual-evidence-diary.md)

Scripts01–07 reproduce build, host qualification, bounded target captures/campaign, analysis and figures. Actual campaign: scripts/evidence/target-20261014T194947Z. Remaining design alternatives are not implemented or measured; the bounds candidate remains opt-in pending broader/integrated qualification.
