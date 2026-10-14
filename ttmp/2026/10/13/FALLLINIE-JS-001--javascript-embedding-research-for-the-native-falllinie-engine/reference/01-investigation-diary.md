---
Title: Investigation diary
Ticket: FALLLINIE-JS-001
Status: active
Topics:
    - research
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://scripts/2026/10/13/falllinie-js-runtime/README.md
      Note: Reproduction and expected limitations
    - Path: repo://scripts/2026/10/13/falllinie-js-runtime/evidence/comparison.json
      Note: Measured parity result
    - Path: repo://ttmp/2026/10/13/FALLLINIE-JS-001--javascript-embedding-research-for-the-native-falllinie-engine/sources/fetch-receipt.json
      Note: Actual fetched refs timestamps hashes and remaining failure
ExternalSources: []
Summary: ""
LastUpdated: 2026-10-13T10:10:27.260849613-04:00
WhatFor: ""
WhenToUse: ""
---

# Investigation diary

## Step 1: Scope and native source boundary inspection

I created a separate research ticket in the managed worktree rooted at native candidate `24b50d8ab6007ba9f66499354331d3d1c93ba30f`. This is documentation and isolated host experimentation only. The existing native trees/trails implementation is merged in this revision but not deployed; the parent reports its last console preflight unreachable. I will not access hardware or modify the parent ticket.

### Prompt Context
**User prompt (verbatim):** "Ok, now what about trees, particle trail, and other features that are missing form the port (compared to original HTML). And also, research how to port JS / integrate JS into our game engine."
**Assistant interpretation:** Compare runtime choices against the frozen HTML and actual C++17 SDL2/GLES3 boundaries, then propose a measured, incremental design rather than restarting the native port.
**Inferred user intent:** Understand remaining browser/native gaps and whether retaining authored JavaScript would reduce ongoing translation effort.

### What I did
- Read AGENTS, full-blown-tech-research-design, docmgr, diary and investigation-entry skills.
- Inspected runtime, game, terrain, world, renderer, pose, particles, audio, feedback and profiler sources, existing oracle generation, CMake and ARM64 toolchain.
- Ran `docmgr ticket create-ticket --ticket FALLLINIE-JS-001 --title 'JavaScript embedding research for the native Falllinie engine' --topics research` and added design/diary documents.
- Confirmed HEAD, clean initial index, x86_64 host GCC 13.3.0 and existing Docker toolchain image `sha256:8068d5a56ed677c85ffd8e02f9ee4e88c9804d56d070aead0d598c44eb0f83dd`.

### Why
Native APIs and frame ownership determine the cost and safety of embedding. A browser HTML file is not a stand-alone ECMAScript module.

### What worked
Separate ticket creation and source inspection. Existing world definitions serve both collisions and rendered objects; particles/trails and native music already exist.

### What didn't work
Parent discovery attempt (reported, not rerun here): Kagi `surf.sock connection refused at /home/manuel/snap/chromium/common/surf-cli/surf.sock`. The parent supplied no full CLI stderr or timestamp; this is the exact failure text available, not evidence of successful search. I will directly fetch known official/upstream URLs and preserve timestamps/hashes instead of restarting browser infrastructure.
Initial HTML read hit the tool's 50KB cap at embedded font line 5; subsequent inspection skips font payload and reads the script body in bounded chunks.

### What I learned
The native frame clock already implements source-shaped fixed 120Hz ticks and single frame scale sampling. Any script adapter must preserve it, not add another requestAnimationFrame loop.

### What was tricky to build
Research source captures must have docmgr metadata; large working source must be ignored, while bounded references and receipts remain committed.

### What warrants a second pair of eyes
The parent must independently qualify trees/trails on-device; nothing in this ticket establishes deployed parity.

### What should be done in the future
Run pure source/native oracles, typed-array ownership/error/interrupt probes and cross-link checks before settling on a runtime.

### Code review instructions
Review the research-only changed path inventory. Start at the guide and experiment README; check no production modifications.

### Technical details
Scripts belong under `scripts/2026/10/13/falllinie-js-runtime/`; `.work/` is ignored. No source/vendor binaries or engine downloads will be committed.

## Step 2: Direct upstream evidence and immutable experiment pins

Between 14:11 and 14:25 UTC I fetched official pages and upstream code directly, because discovery through the parent's Kagi infrastructure had already failed. These captures are not search results. The runtime releases available today differ from older advice: Bellard has an active 2026 release, and ng has a materially changed C API.

### Prompt Context
**User prompt (verbatim):** See Step 1.
**Assistant interpretation:** Establish current release and API evidence, not reputation-based recommendations.
**Inferred user intent:** A defensible runtime choice that the next engineer can reproduce.
**Commit (code):** `21b68dc` — start isolated research ticket.

### What I did
- Added `01-fetch-sources.py`; downloads are timestamped with resolved URL, raw bytes and SHA256 in `sources/fetch-receipt.json`. HTML is converted using installed Defuddle from the exact saved body.
- Fetched Bellard home/manual/changelog, ng pinned README/header/CMake and GitHub release API, Duktape home/guide/features/modules/sandbox, V8 embed/cross/API/version/license, WebKit C API/typed arrays/private limits/port policy/current release page, and pinned quickjspp README.
- Downloaded Bellard 2026-06-04 (`b376e839b322978313d929fd20663b11ba58b75df5a46c126dd19ea2fa70ad2a`) and Duktape 2.7.0 (`90f8d2fa8b5567c6899830ddef2c03f3c27960b11aca222fa17aa7ac613c2890`). Cloned ng v0.17.0 at `6d46d07d04041b40f4f49eaa7fdebe44c314c699` and quickjspp at `01cdd3047ced48265b127790848a0ca88204f2c7`.
- Added reproducible pin downloader `00-download-pins.sh`. All full upstream work remains ignored in `.work/`; only bounded captures and license notices are committed.

### Why
Current versions, source-level ownership comments and ARM build receipts are stronger evidence than a generic framework recommendation.

### What worked
Official direct fetches and source builds. The ng release API identified v0.17.0, published 2026-09-18T21:39:55Z. Bellard home identifies 2026-06-04 and ES2025. WebKitGTK home advertises 2.54.1 (2026-10-02); V8 main header is 15.8.0.0 candidate, not a qualified stable pin.

### What didn't work
- Initial `01-fetch-sources.py` exited 1: `14-quickjspp-readme FAILED HTTP Error 404: Not Found` (README casing), `15-webkit-port FAILED HTTP Error 404: Not Found` (guessed index page), `17-webkit-release FAILED HTTP Error 404: Not Found` (releases.html). Full initial receipt preserved in `sources/initial-fetch-failures.json`. Corrected to readme.md, official DependenciesPolicy.html and WebKitGTK home; rerun succeeded.
- Supplemental guessed `https://wiki.duktape.org/HowtoTimeout.html`: `19-duktape-timeout FAILED HTTP Error 404: Not Found`, retained in final fetch receipt. The fetched Sandboxing page contains the timeout mechanism; no fabricated replacement capture.
- Inspection pipeline `curl ... https://duktape.org/ | head -40` printed `curl: (23) Failure writing output to destination`; head closed the pipe, not an upstream outage. Saved-body fetch later succeeded.
- Defuddle output for the long Duktape guide ended mid-word (`...control flags pac`). Capture explicitly labels this abbreviated extraction; raw full-body hash and smaller topical captures remain available. Do not cite it as the full 2.7 manual: its own heading says 2.6.0 (2020-10-13).

### What I learned
quickjspp's README is C++17/CC0 and its latest observed commit updated bundled QuickJS to 2025-04-26. This is maintenance evidence, not compatibility with the latest two engines.

### What was tricky to build
Avoid source capture metadata pollution in the parent ticket. Every Markdown capture carries this ticket's frontmatter. Mutable main-page snapshots need a timestamp and must not masquerade as immutable release refs.

### What warrants a second pair of eyes
JSC private execution-time API is not a supported public portable contract. V8 candidate version is not a shipping recommendation. Duktape latest-release completeness beyond the fetched 2.7 tarball is not independently proven.

### What should be done in the future
If a larger engine becomes necessary, pin a released JSC/V8 source plus full dependency/sysroot manifest; do not infer target availability from host packages.

### Code review instructions
Audit capture receipt against each source doc. Re-run selected names with `python3 .../01-fetch-sources.py 01-bellard-home`; exact working-source pins use `bash .../00-download-pins.sh`.

### Technical details
Sources captured under `sources/`; upstream license text under experiment `licenses/`. No browser restart, installs, pushes or publication.

## Step 3: Compatibility, source/native oracles, ownership and host timing

I extracted unchanged pure source bodies and evaluated them in Node, Bellard and ng C++ hosts. Both modern interpreters match the selected native and source values; Duktape cannot directly execute required modern constructs. The first generic external-buffer call failed against ng, proving why runtime API compatibility must be inspected rather than assumed.

### Prompt Context
**User prompt (verbatim):** See Step 1.
**Assistant interpretation:** Run bounded reproducible probes, separating syntax/semantics, bridge safety and performance evidence.
**Inferred user intent:** Decide whether JS can own authored rules while native systems remain safe and responsive.

### What I did
- Created `02-extract-source.cjs`, `probe.js`, `host.cpp`, `native-oracle.cpp`, `native-bench.cpp`, `duktape-probe.c`, `04-compare.py`, `03-run-host.sh`.
- Extracted 18,149 bytes of unchanged math/terrain/trees/grab/scoring/row bodies. Source hash `cb9b4befd0252859d9c45246f94fbd7dd1662c6d034c6ccb95fbe16947e8b556`; explicit adapters set SEED=42 and a minimal G.recent, outside fixture.
- Compiled all inline JS with COMPILE_ONLY, then evaluated it in a fresh browserless context. Tested HTML syntax failure and missing document error separately.
- Compared five hashes, twelve seeded RNG values, five height/normal/paint samples, two repeated-score cases and two tree definitions: 85 leaves in each engine/native comparison.
- Bound numeric row snapshot callback, copied 749 Float64 slots in one call, checked subarray offset, wrong width, nonfinite, oversize and external-buffer lease lifetime.
- Ran no-op, 107-column row, 749 scalar calls versus one copied batch, cyclic allocation churn, explicit GC, 20ms interrupt and 1MiB memory-limit probes. Three sequential timing runs are preserved; a concurrent-load run is separately labelled and excluded from baseline comparisons.
- Ran `05-cross-link.sh` in existing pinned Docker image with `--network none`, producing Bellard/ng AArch64 ELFs only, never executing them.
- Ran existing native CPU suite: six tests passed.
- Created quickjspp smoke and `06-wrapper-check.sh`, first testing bundled header against current libraries, then explicit include-only adapted working copies.

### Why
Actual fixture syntax, RNG coercions, score labels and terrain rows are the relevant workload. A batch crossing demonstrates costs and ownership that arbitrary arithmetic benchmarks miss.

### What worked
- `03-run-host.sh` and `04-compare.py`: PASS; Bellard/ng max float error 0 against Node, native 2.220446049250313e-16. Scores 10260 then 6155 for repeated `FS Rodeo 360 · Indy · Helikopter`.
- Both full inline syntax probes PASS; browserless execution fails `ReferenceError: 'document' is not defined` (Bellard) / `ReferenceError: document is not defined` (ng). HTML gives `SyntaxError: unexpected token in expression: '<'`.
- Copy/offset/lease checks PASS with release_count=1; both interrupts returned `InternalError: interrupted` at approximately 20ms; memory overflow returned `InternalError: out of memory`; subsequent arithmetic evaluates successfully.
- AArch64 cross-link passed, Debian compiler 12.2.0-14, ELF machine AArch64 and normal libc/libstdc++/libm/libgcc dependencies. This is not device/sysroot acceptance.
- Duktape rejects arrows, let destructuring, classes and Map; it DOES support exponentiation and Float64Array in this configuration.

### What didn't work
1. First `03-run-host.sh` failed compiling ng: `error: invalid conversion from ‘void (*)(JSRuntime*, void*, void*)’ to ‘size_t’` and `error: cannot convert ‘bool’ to ‘void*’`. ng v0.17 JS_NewArrayBuffer takes max_len and a resize/free callback, not Bellard's free callback. Added explicit `QUICKJS_NG` probe branch which refuses resizing and releases on size==0.
2. Second run stopped in the Duktape probe because I expected `2 ** 3` to fail; actual output was `2 ** 3 => PASS: 8`. Corrected the test expectation rather than asserting broad ES2015/2016 absence.
3. quickjspp header syntax-only tests initially passed because `#include "quickjs/quickjs.h"` selected its bundled 2025 header despite -I ordering. Linking current libraries failed: `undefined reference to JS_SetHostUnhandledPromiseRejectionTracker`; ng also reported `undefined reference to __JS_FreeValue`.
4. Explicit include-only adapted copy failed compile against Bellard: `JS_SetHostUnhandledPromiseRejectionTracker was not declared ... did you mean JS_SetHostPromiseRejectionTracker?`; ng additionally fails JS_IsArray(ctx,v) and JS_NewClassID(&id), which now require different arguments. Diagnostics are saved, not fixed by speculative wrapper forks.
5. Bookkeeping inspection `tail -1 .../native-repeat-{2,3}.txt` returned `tail: option used in invalid context -- 1`; files were intact and inspected separately. No probe depended on that command.

### What I learned
Bellard is smaller and faster than ng on these x86 row samples; native is much faster still. Neither interpreter is established as the target winner. Bridge batching reduces calls but the JS array-writing loop can dominate; an observed batching benefit is not a promise of device frame budget. C API differences make a narrow owned adapter more credible than a generic C++ binding wrapper here.

### What was tricky to build
Live JS values must root array storage through pointer access. No JS calls occur while borrowing bytes; native copy is consumed after release. The lease callback cannot execute JS or GL and must outlive the storage owner. Initial benchmark overlap with native/cross builds changed timings substantially, so reruns intentionally removed our own concurrent work.

### What warrants a second pair of eyes
The prototype checks element width, not full Float64 type identity, and allocates a vector per bridge call; it is NOT a hardened production API. Automatic GC count/causality is not instrumented; only cyclic workload distribution and before/after explicit GC are measured. Scalar and batch loops have different JS overhead. Three sequential samples do not qualify thermal/tail latency on ARM.

### What should be done in the future
Expand deterministic replay to full simulation, detached/resizable arrays, module lifecycle and restart generations; use allocator accounting and profiler markers for automatic GC; run the parent-owned device gate only after approval.

### Code review instructions
Read script README, run `00-download-pins.sh`, `03-run-host.sh`, `04-compare.py`. `06-wrapper-check.sh` intentionally preserves nonzero inner compiler statuses but completes the evidence collection. Check baseline versus concurrent-load result labels. No production build dependencies were changed.

### Technical details
Receipts under `scripts/2026/10/13/falllinie-js-runtime/evidence/`; cross ELFs, libraries and working engines remain ignored. `ctest --test-dir .../.work/native-tests --output-on-failure` passed 6/6. Graphics/hardware gate deferred: research does not change a graphics feature and has no device permission.

## Step 4: Foundational guide, proposed boundaries and research acceptance

I wrote the long-form guide around actual native seams and the measured interpreter results, then linked the ticket, reproduction README, tasks, source receipts and docmgr relations. The recommendation remains an optional future rules experiment, not replacement of the shipping native candidate. Source/native gaps, deployment status and unmeasured alternatives are explicit.

### Prompt Context
**User prompt (verbatim):** See Step 1.
**Assistant interpretation:** Deliver an engineer-readable research/design package and acceptance evidence, without implementing production JS integration.
**Inferred user intent:** Make the next architectural choice informed and executable while retaining completed native progress.
**Commit (code):** `1fb960d` — pin/probe runtimes and bounded evidence; final guide checkpoint follows.

### What I did
- Wrote the substantial guide: conceptual embedding/browser separation, actual feature/gap inventory, evidence-backed matrix, runtime C API/lifetime/GC primer, chosen alternatives, concrete native-file seams, immutable schemas, fixed120Hz frame contract, seeded RNG streams, cooperative rows/generation cancellation, numeric audio boundary, copy/lease safety, exception/fault/security/hot reload policy and eight implementation phases.
- Corrected native source line anchors against numbered source, and checked native X-ray is a shader grid rather than the browser Canvas2D equation/callout overlay.
- Clarified optional stateless scoring: pass a copied recent-label list into temporary adapter state and preserve repeat multiplication before five-point rounding; native commits history once. No independent mutable JS score-history owner is proposed.
- Wrote experiment README, `evidence/pins.json`, ticket index/README and approval-gated follow-up tasks; related six primary guide files and three diary evidence files through docmgr.
- Ran `bash -n` for all five shell scripts; `node --check` on extractor/probe; Python py_compile and independent oracle comparator; `git diff --check`; docmgr doctor; production diff scope check.

### Why
Measured interpreter compatibility does not resolve ownership, interrupted tick recovery, external memory, native audio safety or renderer state migration. Those constraints must be taught before an intern writes bindings.

### What worked
- Static script checks, parity comparator and diff check passed.
- `docmgr doctor --ticket FALLLINIE-JS-001 --stale-after 30`: ticket `All checks passed`; global INFO `no_vocabulary` means unknown-value checks skipped because no vocabulary is configured. I did not initialize global metadata or mutate parent tickets to suppress an informational finding.
- `git diff 24b50d8 -- src CMakeLists.txt assets fixtures tests` is empty (0 lines). All edits remain research documentation, scripts, ignored-work rule, attribution and bounded evidence.

### What didn't work
No new blocking failure in this phase. Source-fetch404s, expected wrapper incompatibilities and Duktape expectation correction remain visible in Steps2/3 rather than rewritten as success.

### What I learned
The lowest-risk authoring seam is stateless rules because it avoids frame/terrain/audio authority migration. Host results favor Bellard over ng for this sample but neither qualifies ARM frame predictability; native terrain is substantially faster and should remain the initial authority.

### What was tricky to build
A full JS simulation would need a new render-only snapshot seam, not field writes into Game with missing private rail/input/timer state. Restart must preserve current seeded-stream and input-edge lifetimes. Source score repetition must occur before final rounding, not on already-rounded base points.

### What warrants a second pair of eyes
Required parent reviewer gate: inspect ownership recommendations, source/native gap inventory and exact measured-versus-proposed claims. The prototype is not hardened and V8/JSC performance remains unmeasured. Device work belongs exclusively to parent acceptance.

### What should be done in the future
Parent decides whether optional JS authoring is justified, then approves the scoped rules/lifecycle phase. Full simulation, modules, reload/migration, detached/resizable/adversarial bridge tests and device profiling are future gates, not completed integration.

### Code review instructions
Start at ticket index/guide and experiment README, compare scripts/evidence, inspect source-fetch receipts and pin manifest, run the listed bounded experiments. Verify no production diff and empty final index before cherry-picking the research checkpoint sequence.

### Technical details
Doctor receipt: experiment `evidence/docmgr-doctor.txt`. Existing native CPU receipt: `evidence/native-tests.txt` (6/6). Cross-link receipt: `evidence/cross-link.txt` (AArch64, never executed). Changed paths and final commit/ref/worktree are provided in the runtime handoff artifact. No reMarkable/vault publication was requested or performed.

## Step 5: Parent review corrections without repeating runtime experiments

The independent reviewer supported the actual oracle/probe/benchmark evidence and ownership recommendations, but blocked final research acceptance on stale deployment wording and an inconsistent ng README receipt. The parent corrected those exact findings and added ticket-local navigation to the dated experiment group. No interpreter or production code changed.

### Prompt Context
**User prompt (verbatim):** See Step 1. The immediate trigger was an automated completed-review notification, not a new authored implementation request.
**Assistant interpretation:** Correct the research package before acceptance; preserve historical failures and measured-versus-proposed distinctions.
**Inferred user intent:** A reproducible, source-grounded report rather than a misleading success summary.

### What I did
- Verified the preserved child worktree was clean at d973e98 before editing. The child's earlier final orchestration abort was not a reason to launch another execution mode.
- Labelled unreachable/not-deployed context historical and annotated the guide/index with the subsequent failed native device qualification. Original diary observations remain intact.
- Restored uppercase `README.md` in `01-fetch-sources.py`.
- Added `07-reconcile-capture-receipt.cjs`: validates the retained ng capture's URL, timestamp and hash against the successful initial receipt; archives the complete later failed retry before rebuilding the central retained-capture index.
- Added linked experiment README/pin manifest/cross-link receipt and `scripts/01-research-script-index.md` within the ticket.

### Why
A later failed fetch must not erase the provenance of a retained earlier successful capture. Combining retained-capture records is legitimate only with original successful timestamps/hashes, explicit historical retry preservation and no invented network success. Deployment observations also require an as-of boundary.

### What worked
`node scripts/2026/10/13/falllinie-js-runtime/07-reconcile-capture-receipt.cjs` validated the retained capture and preserved the failed retry without network access. The central index now has one supplemental failure; the archived later retry still has both failures. Existing runtime measurements were not rerun or altered.

### What didn't work
The reviewed central receipt contained `HTTP Error 404: Not Found` for lowercase ng `readme.md`, contradicting its retained uppercase successful capture and the one-failure summary. `docmgr doctor --ticket FALLLINIE-JS-001 --stale-after 30` initially reported `missing_numeric_prefix` for newly added `scripts/README.md`; renamed it to `scripts/01-research-script-index.md` and corrected its link. The information-only `no_vocabulary` finding is existing repository configuration, not an experiment failure.

### What I learned
A source-capture index and a chronological fetch-attempt log have different purposes. Both must remain navigable and cannot be conflated into a fabricated final successful run.

### What was tricky to build
Reconciliation uses the existing successful receipt, not metadata reconstructed from Markdown or a new fetch. The helper checks all three identity fields before changing the index and preserves the retry bytes. Ticket links must climb five levels from the ticket root and six from its scripts directory.

### What warrants a second pair of eyes
Confirm both historical failures remain available, capture identity is unchanged, and no statement implies device JS execution or native acceptance. The original review found no underlying experiment/design blockers.

### What should be done in the future
Only approve optional JS production work through a separate implementation decision; native audio/frame qualification remains unresolved. V8/JSC target performance and hardened adversarial/OOM bridges remain unmeasured future gates.

### Code review instructions
Inspect the fetch helper URL and reconciliation assertions, central versus archived receipt, guide opening annotation and ticket-relative links. Run the reconciliation helper, `git diff --check` and ticket doctor. Do not repeat the broad runtime matrix for these documentation-only corrections.

### Technical details
Parent native DSP/overlay code at eb6c1a5 independently passed review for further qualification, not release. A subsequent 55-second native 48kHz/1024-frame muted overlay run still recorded 3/2574 callback budget misses, maximum39.8702ms versus21.3333ms budget and about50FPS. These are callback wall-budget observations, not driver XRUN counts. No JS binary was run on the handheld.

## Step 6: Resume real ARM64 and V8 qualification

The user authorized resuming interpreter experiments, including actual ARM64 execution and V8 embedding. New useful tooling and bounded evidence will live under this ticket's scripts/02-arm64-v8 rather than extending the old external experiment group. Existing captures and previous results remain historical and unchanged; production gameplay/audio ownership is not being migrated.

### Prompt Context
**User prompt (verbatim):** "ok, go ahead. Continue the diary. Keep all useful scripts in the scripts/ folder of the ticket (the previous JS ticket)."
**Assistant interpretation:** Execute the previously described QuickJS/QuickJS-ng handheld and real V8 C++ embedding qualification, with continued evidence-backed diary and ticket-local tooling.
**Inferred user intent:** Resolve actual device/runtime feasibility and wrapper constraints before choosing live game integration.

### What I did
Read latest diary checkpoint, prior wrapper diagnostics and subagent execution/review rules. Created ticket scripts/02-arm64-v8/evidence and added explicit ARM64, V8 and review tasks i082/gplk/aaim. Read-only target preflight confirmed aarch64, kernel7.2.7,glibc2.44,975MiB RAM/356MiB available/no swap,ARM SIMD/atomics features,timeout utility and no node executable. No native Falllinie process appeared in the bounded process listing. This is an as-of observation, not permission to stop other applications.

### Why
Cross-linking is not execution proof. V8 C++ embedding must be measured separately from Node script execution and QuickJS wrappers. A1GiB physical device requires conservative benchmark memory limits rather than unbounded allocation stress.

### What worked
Read-only SSH preflight succeeded and bounded target receipt was saved. Existing production source/worktree was clean before ticket-only preparation.

### What didn't work
The initially guessed ~/.pi/agent/skills/pi-subagents/SKILL.md path returned ENOENT; loaded the actual project package skill path instead. No interpreter execution or new V8 result exists yet.

### What I learned
Current target has only975MiB total RAM and no swap. Benchmark limits must account for total native/process overhead, not only JS heap accounting.

### What was tricky to build
Old ignored build trees are absent on parent main and must be rebuilt or reused only with pinned provenance. Wrapper bundled-header/current-library and current-header compile failures are distinct from underlying runtime support.

### What warrants a second pair of eyes
Runtime/handle lifetimes, exception and typed-buffer boundaries, fair cold/warm timing, accurate engine/library/build labeling and device safety. Embedded V8 via libnode is not Node JS execution, but must not be misreported as a custom standalone V8 build.

### What should be done in the future
Build isolated harnesses, review exact candidate, then parent execute bounded device tests with actual version/configuration/startup/memory/GC receipts. No JS enters audio callback,GL worker or live simulation in this phase.

### Code review instructions
Start at new ticket scripts/02-arm64-v8 and previous external experiment README/pin manifest. Require actual AArch64 run records and fixture comparisons; do not accept ELF identification alone.

### Technical details
Parent owns device transfer/run and final acceptance; isolated worker owns harness/build/reproducibility scripts and a focused continuation diary. Preserve historical script group rather than moving or silently rewriting its evidence. Credentials and arbitrary environment values stay outside receipts; useful scripts use explicit non-sensitive configuration and ignored .work directories.

## Step 7: Correct the reviewed boundaries and execute all three engines on ARM64

The worker completed direct QuickJS, QuickJS-ng and raw V8 C++ harnesses, then an independent reviewer identified two bounded P2 defects. The same writer corrected them and proved red/green behavior before a targeted reviewer accepted the final candidate. Parent verified archives/source identities, integrated the scoped scripts/diary, and actually executed all three private bundles on the handheld.

All three target processes exited0 and passed85 selected oracle leaves,seven deterministic workload families and19 native protocol checks. V8 was substantially faster on the warmed workload but had higher startup/memory costs and significant first-call/tail costs. This is a finite isolated feasibility gate, not production or full-game performance acceptance.

### Prompt Context
**User prompt (verbatim):** See Step6.
**Assistant interpretation:** Continue until actual handheld execution and independently checked receipts exist, not merely cross-built packages.
**Inferred user intent:** Measure interpreter/V8 feasibility and native boundary correctness without rewriting the game.
**Commit (code):** Worker5fbe46a/684c2d3, corrected6dd6f89/3535a2d; parent fast-forwarded to3535a2d before device tests.

### What I did
- Read complete worker/reviewer handoffs. Initial review found logging included in source-read/first-call/explicit-GC durations, and finite elements could overflow native sum to infinity before copied-state commit.
- Routed both findings to the same writer; corrections captured operation endpoints before output and rejected nonfinite sum before modifying any749 copied slots/count/sum. New two-MAX_VALUE regression failed the original helper/all three original embeddings, then passed corrected code. Host3runs per engine,11comparator regressions,12native CTests and bounded sanitizer checks passed; original receipts unchanged.
- Read targeted follow-up: both findings resolved, no new issues. Independently verified full corrected patch SHA256361d6b23dc89c7a74cbd030d6bf795354b4952048a11377ad76b1f4c246c4f08, archive hashes, every bundle SHA256SUMS and manifest hash map against current committed harness sources. No production/historical-experiment diff.
- Integrated candidate via fast-forward. Saved original/correction review receipts as raw .txt inside ticket evidence. Added11-parent-arm64-qualification.sh and12-summarize-target.py under the requested ticket scripts directory.
- Transferred separate private bundles to /storage/apps/falllinie-js-probes and ran Bellard,ng,V8 sequentially,300fixed blocks each, existing owned180s timeout/5s grace/headroom/hash/lock guards. Used externally supplied credentials; no library install/service/game-process/scheduling changes.
- Collected actual preflight/runtime/log/timing receipts, independently compared locally, and retained bounded evidence under scripts/02-arm64-v8/evidence/target-corrected-v2. Updated README/script navigation to label historical PENDING manifests versus actual execution.

### Why
Numerical overflow and logging contamination were small code changes but directly affected the new boundary/measurement claims. Correcting them before device execution avoided collecting misleading first-call evidence. Private dependency closure permits real raw V8 embedding without altering system libraries or launching Node scripts.

### What worked
| Target measurement | Bellard2026-06-04 | ng0.17.0 | V8 10.2.154.26-node.37 |
|---|---:|---:|---:|
| Terrain row107columns p50 |6.337195ms|7.347798ms|0.985520ms|
|128scoring pairs/256scoreAir calls p50|14.312937ms|16.077189ms|2.190660ms|
| First terrain wrapper call |6.713437ms|8.387858ms|24.181260ms|
| Sustained heavy block p50 |122.357619ms|138.382295ms|11.846073ms|
| Sustained heavy block p99/max |127.302429/135.605699ms|143.319816/156.141492ms|25.541855/47.716492ms|
| Reported getrusage peak RSS |4888KiB|5220KiB|40528KiB|
| External integer process wall |40s|46s|5s|
| Explicit requested GC wall |0.698818ms|0.648069ms|3.570213ms|

All85leaves/7workload families passed. Engine float delta against retained host Node oracle was2.220446049250313e-16 for both QuickJS engines and0 for V8; labels/integers exact. Native reference here is retained HOST oracle, not a new ARM64 native speed benchmark. All19protocol checks and intentional exception/recovery/controlled single job/lifecycle sequences passed.

Actual loader used private pinned libraries, system libc/loader,glibc2.44,aarch64/A55 features. V8 reported raw C++ API through Debian libnode108 Node18.20.4+dfsg-1~deb12u2/defaultJIT/not-standalone. Target preflight frequencies were1416000kHz,ondemand; no frequency stability control or policy change. All observed memory values remained well below192MiB guard.

### What didn't work
Original overflow helper/embeddings failed exactly with `FAIL finite input sum overflow accepted` / `FAIL Error: native sum overflow accepted`; preserved under review-corrections-v2/before-fix. Worker ticket doctor scanned six downloaded ng .work Markdown files despite attempted ignore flags, plus existing vocabulary warnings; actual diagnostics preserved rather than claiming clean doctor. An initial parent read guessed the workflow-owned correction handoff path and got ENOENT; located the resumed-child-owned output and read that actual file. No device transfer/loader/process/comparison failure occurred.

### What I learned
V8 warmed row cost was about0.986ms versus6.337/7.348ms for interpreters in this one configuration, but first wrapper execution took24.181ms and heavy-block max47.716ms. Those costs matter before introducing frame-sensitive JS. Higher~40MiB process memory is feasible for this isolated run but is not a combined game memory qualification.

### What was tricky to build
V8 comes from a pinned older libnode library, not Node execution or a new latest standalone build. First function calls follow eight lifecycle cycles and are not whole-process-cold. Scalar749calls and batched749writes+copy perform different JS work: their timings cannot isolate bridge-only speedup. Sampled VmRSS and getrusage peak are distinct observations; summary records both.

### What warrants a second pair of eyes
Module ownership, hard execution budgets, long GC/scheduler tails and combined gameplay/native memory remain future integration risks. Automatic GC pause causality is unavailable; V8 prologue counts do not attribute measured tails to GC. QuickJS automatic GC counts remain unavailable. Explicit single trusted microtask is not an arbitrary hostile-job budget.

### What should be done in the future
Use these results to choose a bounded rules-module integration separately. Keep native terrain/render/audio ownership for the first slice; do not infer60FPS or full simulation parity from isolated fixed work. Consider current standalone V8, repeated/randomized/thermal runs, real command replay, module reload and supported stronger memory/time containment before shipping a scripting runtime.

### Code review instructions
Read both raw parent review receipts and worker diary02 Step4, current harness/protocol, manifest/source hash audit and actual target logs/preflights/comparison/summary. Run12-summarize-target.py and04-compare.py on retained target data; no device rerun needed for documentation. Inspect scoped final diff for credentials/generated third-party bytes/production changes.

### Technical details
Corrected archives verified: Bellard4a00559d1299eac38310fea8aa27ed809356366c5bba9b31aa3c0e747a84514b; ngbf6416eb3933e5385ab2042d034b64a94b711c446ab1353da96426ed47713f85; V8df7d4c3b1c9ac9d8231703663c234019c518b9f9093ad4769c91e1df1aa94a87. Default sustained workload totals2400rows,307200scoreAir evaluations and600000cycles per engine; not an actual game-frame loop. V8 explicit GC reduced used heap from2514352to676552bytes; no automatic pause attribution. Heap limits are not OS RSS limits. Older cross-build manifests remain immutable PENDING snapshots; parent actual execution receipts supersede that status for the tested packages only.
