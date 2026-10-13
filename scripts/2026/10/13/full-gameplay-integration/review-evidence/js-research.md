## Review

**Scope:** Completed research at `d973e98f423f1e65100f91fa68403cfc3eb74d85`, not production JS integration. Read-only: no edits, commands, tests, installs or device access.

Paths below are relative to the supplied worktree. `P` means `scripts/2026/10/13/falllinie-js-runtime/`; `T` means `ttmp/2026/10/13/FALLLINIE-JS-001--javascript-embedding-research-for-the-native-falllinie-engine/`.

### Correct

- **Source-derived comparisons are supported.** `P/02-extract-source.cjs:5–14` extracts fixture bodies with explicit adapters; `P/probe.js:2–10` defines the selected oracle workload. The committed Node/native/interpreter outputs support 85 leaves, identical scoring labels and scores 10260→6155, with native maximum float delta `2.22e-16`. This is correctly described as selected-value coverage, not full simulation parity.
- **Embedding probes match their bounded claims.** `P/host.cpp:22–32,58–82` covers rooted copying, offsets, width/NaN/size rejection, exactly-one lease release, exceptions, interruption and allocation limits. Guide lines 149–150 explicitly acknowledge trusted-only operation and missing adversarial/OOM hardening.
- **Benchmark qualifications are appropriate.** `P/probe.js:12–23` and `P/native-bench.cpp:5–8` use matching row coordinates but different allocation shapes. Scalar/batch loops also differ. Guide lines 156–170 disclose those differences, repeats, unqualified host scheduling and explicit-GC-only measurements. Cross-link evidence establishes AArch64 ELF construction—not target execution or performance.
- **Runtime distinctions are verified.** ng’s captured header documents different class-ID and external-buffer APIs (`T/sources/05-ng-api.md:727,1096–1113`). Wrapper diagnostics corroborate incompatibility. Duktape’s actual rejection/success results support its exclusion for unchanged source. V8/JSC remain explicitly documentation-only, with JSC’s private timeout API and temporary typed-array pointer documented.
- **Proposed ownership is coherent.** Guide lines 178–190 retain one authoritative simulation/history/terrain provider; lines 194–229 keep SDL/GL on the main thread. The 120Hz contract matches `src/runtime.cpp:48–56`; audio recommendations preserve the bounded native callback rather than introducing JS there. GC, security, bytecode and reload recommendations are clearly future design gates.
- **Checkpoint sequence exists.** Worktree Git metadata’s `logs/HEAD:3–5` confirms `21b68dc → 1fb960d → d973e98`, matching the durable handoff. Cleanliness and production-diff absence remain parent-verified facts; I did not run Git verification.

### Findings / smallest corrections

1. **P1 — Historical deployment context needs an explicit current-status annotation.**  
   `T/design-doc/01-javascript-runtime-research-and-integration-design.md:34` presents unreachable preflight as the parent’s latest state; diary line 28 records “not deployed.” The supplied review context says the native parent has since been deployed and failed combined audio/performance acceptance.  
   **Smallest fix:** Label the earlier statements historical/as-of research, preserving the diary chronology. Add a current parent-status note: deployed, combined audio/performance gate failed, acceptance still pending. Do not imply any device JS execution.

2. **P1 — Capture reproduction and the central receipt disagree with the retained ng README.**  
   `P/01-fetch-sources.py:11` requests lowercase `readme.md`; `T/sources/fetch-receipt.json:30–34` records its 404. The retained capture uses uppercase `README.md` and has a successful timestamp/hash (`T/sources/04-ng-readme.md:10–17`), also preserved in the initial receipt. The final receipt therefore contains **two** failures, contradicting the one-failure summary in `T/index.md:26` and the README’s expected capture outcome.  
   **Smallest fix:** Restore the uppercase ng URL, associate the successful capture with its existing successful receipt, retain the later failed attempt separately, and correct the summaries. No runtime experiment needs repeating.

3. **P2 — Ticket index lacks navigable links to the dated scripts.**  
   `T/index.md:27` provides only a code-formatted experiment-root path, and `RelatedFiles` is empty at line 9.  
   **Smallest fix:** Add explicit relative links to the experiment README and pin/evidence manifest. The README already enumerates the individual scripts, so moving or duplicating them is unnecessary.

### Merge verdict

**BLOCK final research acceptance pending corrections 1–2; include correction 3 in the same small documentation checkpoint.** The underlying research is sound and does not require production changes, child revival or alternate execution. This verdict is solely for the research package—not approval to ship JS in the engine.

<summary>
This turn: Reviewed research artifacts and found three small corrections.
Session so far: Oracles, probes, benchmarks, API distinctions and ownership recommendations are supported.
Issues: Historical deployment wording, ng capture receipt mismatch and missing script index links.
Next steps: Correct documentation and receipts, then accept research only.
</summary>