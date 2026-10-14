---
Title: Rendering measurements vault publication helpers
Ticket: FALLLINIE-PERF-004
Status: active
Topics: [performance]
DocType: reference
Intent: long-term
Summary: Publication-only commands and receipts for the measured rendering report.
---

# Rendering measurements vault publication

Publication-only tooling; no native runtime source changes, target execution, web research, installs or native push.

## Authority and inputs

The retained worker source branch started at `09430e4`; it is **not** the measured TRACE005 runtime. Read-only evidence came from `/home/manuel/code/wesen/2026-10-12--falllinie-spike` at `f400ffa7c658086cf775fd6d27c28403ba09c2bb`. The deployed runtime is `209a6d092885ff3f0a1efd1efab4252cb4ad2b56`, binary SHA256 `2872ede7219ccd91890de4c0d47d5af8fab687f2b5b2cf9605e25ecb1dfcb1d2`.

The user authorized one new dated report and a unique asset subtree in the go-go-parc vault. Both writing skills were read fully. Parent read the complete draft and explicitly approved scoped commit/push after two editorial changes, both applied. No earlier report or unrelated file was modified.

## Helper and boundary checks

`publication.py prepare` copies the five existing measured SVGs and their provenance/summary, extracts the actual frame60 paired sample, and records initial PERF004 table values and source hashes. It refuses an existing asset destination. It does not generate new measurement data or regenerate figures.

`publication.py validate` is the **pre-publication** gate. It checks the pinned source revision, every figure/input hash in the source provenance, source/raw frame counts and means, same-row paired counts/means/sample arithmetic, GPU anomalous-sample population and aggregate totals, perf denominator, note numeric anchors, relative assets/related notes, SVG XML, word count, two Mermaid blocks, unchanged vault base and exact intended new-file scope. It writes `validation.json` and its readable log is retained separately.

These gates intentionally bind to vault base `da2c32a7a2978e26c875c553652378c4cdbee617`; do not rerun `prepare` after publication or expect `validate` to pass its new-file/base guards on an already committed report. For post-publication verification, compare current note/asset hashes with `validation.json` and check the independently observed remote commit in `publication-receipt.json`. The report's raw-data calculations remain directly reproducible from the pinned input paths and source provenance.

Commands used from the retained native worktree:

```sh
T=ttmp/2026/10/13/FALLLINIE-PERF-004--gl-pass-measurement-and-evidence-driven-native-optimization
python3 "$T/scripts/02-vault-report/publication.py" prepare
python3 "$T/scripts/02-vault-report/publication.py" validate
# Parent editorial approval, two requested edits, then one final validation:
python3 "$T/scripts/02-vault-report/publication.py" validate
```

Vault git add/commit/push were performed only after approval, with the exact ten-file set cross-checked against the validation manifest. `git ls-remote origin refs/heads/main` independently matched local HEAD after push, the parent commit is the preserved `da2c32a`, and the vault is clean. `vault-commit.log`, `vault-push.log` and `publication-receipt.json` are the publication receipts. No broad host/device test matrix was rerun for prose publication.

## Result and limits

Published note: `Projects/2026/10/13/PROJECT REPORT - Falllinie Rendering Measurements - CPU Execution, Mali Driver Work and GPU Timer Limits.md`; assets: sibling `_assets/falllinie-render-measurements/`. Final prose count: 5,758 words, two Mermaid diagrams, five exact measured SVGs, four evidence JSON assets.

Vault commit/remote: `c7b1535f1d2eb951d03001cc6ee8273904f56359` on `origin/main`.

The report distinguishes measured CPU execution and non-running estimates from unresolved Mali elapsed-query semantics. It includes the original PERF004 target results and schema2/capture corrections, actual TRACE005 campaign results and a real paired sample. Optimization/parallelization/precomputation candidates remain hypotheses. More precise Mali profiling (GPU/driver identity, counter interfaces, Streamline/gatord compatibility; Offline Compiler/API/frame tooling secondary) is explicitly future research, not confirmed compatibility or executed tooling.
