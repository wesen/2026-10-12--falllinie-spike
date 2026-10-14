Falllinie JS report publication helpers

Evidence revision: 22241cab96cf61928bde3eec61c88071e8eda402 (immutable input).
No engine/build/device experiments are run here. The report is append-only.
Publication date follows local `date -I`: 2026-10-13 (America/New_York).

Foundational explanation dependency order, fixed before drafting:
1. ECMAScript language versus host capabilities; embedding and native authority.
2. Runtime/heap versus context/realm; binding and copied snapshot.
3. Managed value, root, borrowed bytes, native-owned copy, validation and commit.
4. Exceptions, GC and explicit jobs; interpreter versus JIT.
5. Source oracle, protocol tests, checksum, lifecycle/first-call/warm/process timing.
6. Binary/header ABI pairing and cross-link versus actual target execution.
7. Proposed bounded rules contract, shadow comparison and single commit authority.

Representative trace: spin=2*pi, flip=-2*pi, t=1.3, Indy .5s/.3s tweak,
Helikopter 1.5s, perfect landing; empty copied history -> 10260 points;
same canonical name in history -> 6155. Original scoreAir mutates temporary
G.recent; pure production adapter requires explicit supplied history.

Commands (from this folder):
 python3 generate_assets.py
 python3 validate_report.py

Generator creates only the unique report asset subtree in the vault. It reads
committed receipts and source at the immutable revision, recomputes hashes,
and derives numbers without rerunning native/device/build matrices.
Validator checks prose word count, metadata/fences, portable figure links,
SVG XML/no external dependencies, receipt hashes and numeric table agreement.
Native helpers may be committed scoped; never push native repository here.
Vault publication requires parent editorial/technical approval before scoped
commit and existing origin/main push. Preserve all unrelated state.
