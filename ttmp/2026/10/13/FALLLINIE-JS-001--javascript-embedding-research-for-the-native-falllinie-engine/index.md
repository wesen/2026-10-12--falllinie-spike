---
Title: JavaScript embedding research for the native Falllinie engine
Ticket: FALLLINIE-JS-001
Status: active
Topics:
    - research
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://scripts/2026/10/13/falllinie-js-runtime/README.md
      Note: Reproducible commands and bounded evidence
    - Path: repo://scripts/2026/10/13/falllinie-js-runtime/evidence/pins.json
      Note: Pinned engine archives and hashes
ExternalSources: []
Summary: Completed research and isolated probes; optional rules embedding recommendation, no production integration or device acceptance.
LastUpdated: 2026-10-13T14:35:00Z
WhatFor: Find the engine-specific runtime tradeoffs, design, sources and reproducible evidence.
WhenToUse: Review JS authoring feasibility before approving a production implementation phase.
---

# Falllinie JavaScript embedding research

Research complete; implementation/device decisions remain parent-owned. Native trees, spray, trail, rail objects and audio exist in base candidate `24b50d8`; they are **not deployment-qualified** here.

## Read first

- [Research and integration guide](design-doc/01-javascript-runtime-research-and-integration-design.md): actual browser/native gaps, runtime comparison, experiments and proposed ownership/API/clock/audio/terrain/reload contracts with phased implementation plan.
- [Chronological investigation diary](reference/01-investigation-diary.md): exact authored request, reported Kagi failure, direct-fetch404s, ng API compile failure, corrected Duktape expectation, wrapper failures and validation.
- [Tasks](tasks.md) and [changelog](changelog.md): completed research versus approval-gated follow-ups.
- `sources/`: 24 official/upstream Markdown captures with metadata and per-fetch timestamps/hashes; `fetch-receipt.json` indexes retained captures using their actual successful timestamps/hashes and includes one supplemental404. `initial-fetch-failures.json` preserves the initial attempts; `fetch-receipt-lowercase-retry.json` preserves the later two-failure retry (including the mistaken lowercase ng README request). No successful fetch is fabricated. Long Duktape manual extraction is explicitly abbreviated.
- [Experiment README](../../../../../scripts/2026/10/13/falllinie-js-runtime/README.md): pinned downloads, extraction, host/native/Duktape/wrapper probes, cross-link commands and evidence.
- [Ticket script index](scripts/01-research-script-index.md): navigable experiment/evidence paths without duplicating tooling.
- [Pin manifest](../../../../../scripts/2026/10/13/falllinie-js-runtime/evidence/pins.json) and [cross-link receipt](../../../../../scripts/2026/10/13/falllinie-js-runtime/evidence/cross-link.txt).

**Current parent status after review:** native code has been deployed; combined audio/performance qualification failed and remains ongoing. Earlier unreachable/not-deployed diary entries are historical. No JavaScript device execution has occurred.

## Recommendation

Keep native candidate as shipping architecture. If JS authoring is approved, start with optional stateless score/trick rules using a narrow Bellard QuickJS2026-06-04 C API adapter; ngv0.17.0 remains a measured alternative. Do not adopt quickjspp unchanged or assume embedding creates DOM/WebGL/WebAudio. Full browser host and full simulation ownership need separate approval.

## Evidence summary

Both QuickJS variants parse actual main inline source and match85 selected Node/native oracle leaves (native max float delta2.22e-16). Numeric typed snapshot copy/offset/lease/error/interrupt/memory-limit probes pass; Duktape rejects required arrows/classes/Map. Existing native CPU suite passes6/6. Both interpreter hosts cross-link to AArch64 in the existing offline container. Host rows favor current native code strongly; these are not deviceFPS results. V8/JSC remain documentation/source analysis, not measured alternatives.

No production source/CMake/assets/fixtures/tests or parent ticket edits. No device access, deployment, system installs, push, upload or browser infrastructure restart. Relevant graphics/hardware acceptance is deferred to parent, not silently marked passing.
