---
Title: JavaScript runtime research script index
Ticket: FALLLINIE-JS-001
Status: active
Topics: [research]
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Links to the self-contained dated experiment group; no duplicated scripts or production integration."
LastUpdated: 2026-10-13T15:00:00Z
WhatFor: "Navigate research commands, pin manifest and recorded evidence from the ticket."
WhenToUse: "Reproduce or review the isolated interpreter experiments."
---

# Research scripts

- [Commands and experiment README](../../../../../../scripts/2026/10/13/falllinie-js-runtime/README.md)
- [Pinned versions and archive hashes](../../../../../../scripts/2026/10/13/falllinie-js-runtime/evidence/pins.json)
- [Recorded AArch64 cross-link](../../../../../../scripts/2026/10/13/falllinie-js-runtime/evidence/cross-link.txt) — not device execution.
- [Historical receipt reconciliation](../../../../../../scripts/2026/10/13/falllinie-js-runtime/07-reconcile-capture-receipt.cjs) — validates the successful ng capture metadata and preserves the failed lowercase retry; no new fetch.

The historical experiment group remains unchanged. Its README enumerates earlier extraction, host, comparison and cross-link verbs.

## Resumed ARM64 / V8 qualification

All new useful tooling is ticket-local as requested:
- [Probe contract and reproducibility](02-arm64-v8/README.txt)
- [Pinned sources/private packages](02-arm64-v8/00-fetch.py) and [Debian pins](02-arm64-v8/debian-pins.json)
- [Host build](02-arm64-v8/02-build-host.sh), [ARM64 build](02-arm64-v8/03-build-arm64.sh), [private packaging](02-arm64-v8/06-package.py)
- [Comparator](02-arm64-v8/04-compare.py), [host run](02-arm64-v8/05-run-host.sh), [validation](02-arm64-v8/07-validate-host.sh)
- [Transfer](02-arm64-v8/08-deploy.sh), [owned target run](02-arm64-v8/run-target.sh), [collection](02-arm64-v8/09-collect.sh)
- [Reviewed parent orchestration](02-arm64-v8/11-parent-arm64-qualification.sh), [actual target summary generator](02-arm64-v8/12-summarize-target.py)
- [Actual ARM64 oracle comparison](02-arm64-v8/evidence/target-corrected-v2/comparison.json) and [measured summary](02-arm64-v8/evidence/target-corrected-v2/summary.json)

The three engines actually executed on the handheld. This is a finite isolated workload gate, not live game integration, a latest standalone V8 build or thermal/60FPS acceptance. Original and corrected host evidence remain distinct.
