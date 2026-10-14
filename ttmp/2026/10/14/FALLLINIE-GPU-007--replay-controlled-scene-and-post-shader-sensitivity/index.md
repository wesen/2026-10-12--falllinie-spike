---
Title: Replay-controlled scene and post shader sensitivity
Ticket: FALLLINIE-GPU-007
Status: complete
Topics:
    - performance
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-10-14T15:21:05.855226964-04:00
WhatFor: ""
WhenToUse: ""
---

# Replay-controlled scene and post shader sensitivity

Completed a replay-controlled, explicitly non-parity scene/post sensitivity experiment. Canonical assets and runtime defaults remain unchanged.

- [Experimental contract](design-doc/01-scene-and-post-shader-sensitivity-contract.md)
- [Measured results and limitations](analysis/01-measured-non-parity-scene-and-post-sensitivity.md)
- [Detailed chronological diary](reference/01-shader-sensitivity-investigation-diary.md)

Reproduction: scripts/01-prepare.py creates the diagnostic packages;02-host-check.py checks linking/consistency;04-campaign.sh runs the bounded key-authenticated matrix;05-analyze.py accepts a campaign directory;06-figures.py regenerates the published checkpoint's figures. Actual evidence is scripts/evidence/target-20261014T191037Z. These tools remove rendering work and change images; do not install the diagnostic assets as production replacements.
