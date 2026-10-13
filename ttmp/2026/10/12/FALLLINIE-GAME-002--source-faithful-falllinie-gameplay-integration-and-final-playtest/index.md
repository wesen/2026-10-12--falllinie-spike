---
Title: Source-faithful Falllinie gameplay integration and final playtest
Ticket: FALLLINIE-GAME-002
Status: active
Topics:
    - handheld
    - retro-gaming
    - architecture
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-10-12T20:50:58.369727063-04:00
WhatFor: ""
WhenToUse: ""
---

# Source-faithful Falllinie gameplay integration and final playtest

This ticket owns the source-faithful intern integration guide, shared-state dual views, ongoing frame-performance investigation, and the final physical playtest after remaining gameplay/audio features are integrated.

- [Intern implementation guide](design-doc/01-intern-guide-source-faithful-gameplay-audio-objects-and-incremental-streaming.md)
- [Detailed investigation diary](reference/01-investigation-diary.md)
- Source mechanics baseline: `e68c9cb`; dual views and sky continuity: `cec4fcd`.
- Profiling has identified hidden background instances and synchronous cell-build hitches. Isolated dual-view production is currently about 47 FPS, not qualified as drop-free 60 Hz.
- Guide delivery to reMarkable is pending; full source audio, runtime pose/grabs/combos, objects and cooperative streaming remain implementation tasks.
