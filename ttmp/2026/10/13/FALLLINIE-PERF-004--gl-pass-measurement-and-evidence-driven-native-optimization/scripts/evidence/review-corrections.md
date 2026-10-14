# Evidence-integrity correction annotation

Applies to the original tooling candidate `7e58e0f09080a6ae72b72a293c37f0053ddcea18` and its retained host receipts. The independent review accepted the measurement direction but identified two P2 findings: CPU-scope completeness was not enforced, and the capture comparison overstated what it tested. Original raw receipts, logs and the historical diary entries are **not rewritten** by this annotation.

## CPU evidence integrity

Previously a header-only `.passes.csv` or an orphan child could pass analysis despite GPU scope records and zero reported CPU drops. The correction adds final stats `schema_version=2,cpu_samples`, counted when queued CPU scope rows are flushed (including shutdown). Analysis retains CPU scope identities, reconciles the emitted/attempted row count, enforces parent closure, and requires a CPU counterpart for every issued GPU scope regardless of valid/disjoint/pending/pending_disjoint status. CPU buffer drops remain an error. Sampled frames must contain their top-view CPU record. Genuine unsupported GPU queries can still have complete valid CPU evidence and an empty GPU CSV.

Historical unversioned stats are explicitly **schema1** and lack the count needed to attest optional CPU scopes. The corrected analyzer fails closed with: `legacy schema 1 lacks CPU completeness count; retain raw receipts and regenerate with schema 2`. This is a version migration, not evidence that the old timing values were necessarily wrong. Old receipt JSON and raw timing rows remain readable, but must not be relabeled complete or patched with guessed counters. Six retained host receipts were parsed unchanged and all six old profiles produced the explicit schema1 diagnostic. New receipt JSON format is unchanged.

## Withdrawn capture interpretation

The old smoke comment “no query-caused rendering difference” and diary Step2 “query-noninterference evidence” are withdrawn. The comparison establishes only **POST-RUN UNINSTRUMENTED FIXTURE REPEATABILITY**. `main` detaches measurement before capture and renders a fresh frame. The compared images are not images of sampled query-active draws. Query-active image equivalence is **not tested and explicitly deferred**; no new capture path was added. This annotation supersedes any broader interpretation of retained command notes or the prior implementation handoff, without modifying those historical raw artifacts.

## Focused validation (correction only)

- Added regression cases first: the old analyzer ran19 tests and failed with `FAILED (failures=11, errors=1)`. The failures reproduce header-only/truncated/orphan acceptance, lack of all-status GPU/CPU matching and missing explicit schema handling; the error is the not-yet-implemented integrity output key. Exact original output: `corrections-analyzer-red.log`.
- After correction:19/19 analyzer tests passed (`corrections-analyzer-green.log`). Includes valid off/unsupported CPU-only evidence, all four GPU statuses, parent closure independent of count mismatch, missing/empty/truncated files, legacy version diagnostics and nonadditive distributions.
- Rebuilt only `test-profiler` and `falllinie-spike`; CTest restricted to `^profiler$`:1/1 passed (`corrections-profiler-test.log`). Assertions cover flush/destructor CPU counts, unsupported/cadence accounting and32 emitted/8 dropped for an intentionally overflowing40-scope queue, alongside existing query lifecycle checks.
- One fresh existing-Xvfb/llvmpipe static dual8-frame run (no captures): `GL_error=0`,48 valid pass-query rows and64 CPU scope rows, schema2, no drops/misuse. Corrected analyzer accepted it. See `corrections-host-summary.json`.
- No broad physics/audio/GPU matrix rerun, no device run, no new system dependencies or source optimization. Earlier13/13 full CTest and six-case graphics results remain historical, not newly rerun claims. Mali qualification, observer effects and source optimization acceptance remain parent gates.
