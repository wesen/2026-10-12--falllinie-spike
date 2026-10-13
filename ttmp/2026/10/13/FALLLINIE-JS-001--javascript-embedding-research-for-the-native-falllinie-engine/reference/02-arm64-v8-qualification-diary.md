---
Title: ARM64 and V8 qualification continuation diary
Ticket: FALLLINIE-JS-001
Status: active
DocType: reference
Topics: [research]
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Worker investigation for ticket-local probes; ARM64 execution belongs to parent."
LastUpdated: 2026-10-13
WhatFor: "Preserve exact implementation failures and qualification boundaries."
WhenToUse: "Review or reproduce the resumed interpreter slice."
---

## Step 1: Establish the resumed qualification boundary

I read the design guide, reference01 Steps5–6, historical script index, pin manifest, extraction/bridge harness and exact quickjspp diagnostics. This continuation deliberately preserves that experiment group. All new tools and evidence live in ticket scripts/02-arm64-v8, with dependencies and build artifacts ignored in .work. No target contact or production changes are authorized for this worker.

The existing Docker image supplies GCC12 AArch64 cross tools. Host installed V8 headers are 10.2.154.26 and libnode.so.109 belongs to Ubuntu Node18.19.1, unlike the active nvm Node24 executable. A pinned private Debian libnode bundle is a practical alternative to downloading/building GN/depot_tools; this is actual raw V8 embedding, not standalone latest V8 or Node JavaScript execution.

### Prompt Context
**User prompt (verbatim):** "ok, go ahead. Continue the diary. Keep all useful scripts in the scripts/ folder of the ticket (the previous JS ticket)."
**Assistant interpretation:** Prepare reproducible, bounded QuickJS/Bellard, QuickJS-ng and real V8 C++ host/ARM64 harnesses for parent review and subsequent device execution.
**Inferred user intent:** Resolve runtime feasibility with evidence, without changing the native game.

### What I did
- Inspected the clean isolated worktree at e1a80c3a32d6d0568bc4f9d1ac0824a51dc6bc04.
- Read the full investigation-entry diary format and historical source fixtures/oracles.
- Verified Docker toolchain image sha256:8068d5a56ed677c85ffd8e02f9ee4e88c9804d56d070aead0d598c44eb0f83dd.
- Located readable historical ignored engine sources/builds; did not modify them.

### Why
- Cross-link and actual target execution are distinct acceptance gates.
- Small private bundles avoid a multi-gigabyte V8 source/build tree and system installations.

### What worked
- Existing fixture extraction and 85-leaf oracle contract can be reused unchanged.
- Both pinned interpreter source trees remain readable.

### What didn't work
- No implementation failure yet. There is no file named quickjs-probe.cpp in the old group; the actual existing C API harness is host.cpp.

### What I learned
- quickjspp-only failures distinguish bundled-header/new-library link failures from include-adapted/current-header API drift; no broad wrapper rejection follows.

### What was tricky to build
- Header/library ABI pairing must be pinned; active Node executable version cannot supply libnode provenance.

### What warrants a second pair of eyes
- V8 handle/context/isolate/platform destruction order, copied buffers and honest measured-versus-pending labeling.

### What should be done in the future
- Parent runs device gates only after fresh review.

### Code review instructions
- Start at scripts/02-arm64-v8, compare against old source adapters and pins.

### Technical details
- Target preflight is historical/as-of: 975MiB RAM,356MiB available,no swap; no assumption about enduring process state.
- Heap caps are not RSS caps. No fatal OOM or unbounded workload will be sent to target.

## Step 2: Pinned private V8 bundle and first real embedding runs

I selected official Debian bookworm Node18.20.4+dfsg-1~deb12u2 libnode108 packages for both architectures, with package SHA256 pins and private extraction only. The new C++ executable initializes v8::V8/platform, constructs isolates/contexts and directly compiles/calls original fixture functions. It does not invoke node::Initialize, a Node executable, npm or JS shell. Both pinned QuickJS engines use the same fixture/adapters and deterministic workloads.

The first successful host smoke runs match the source workload values and pass copied-buffer/exception/job controls, with V8 identifying itself as 10.2.154.26-node.37. Cross-compilation then produced three AArch64 executables. Those facts are host execution and cross-link proof only; no ARM executable was run.

### Prompt Context
**User prompt (verbatim):** See Step 1.
**Assistant interpretation:** Build safe reproducible probes, preserving semantic fixture bodies and clear provenance.
**Inferred user intent:** Compare real embeddings, not wrappers or Node script timing.

### What I did
- Added fixed package manifest, hash-verifying 00-fetch.py and ticket-local source extractor.
- Implemented direct QuickJS C API and raw V8 API adapters, shared timing driver and explicit JS workloads.
- Added host and network-disabled Docker AArch64 build scripts.
- First smoke used 20 sustained blocks; the qualification default is being extended to 300 fixed blocks (2400 rows,307200 score evaluations,600000 cyclic objects).

### Why
- A private ~10MiB compressed ARM libnode package plus shared dependencies is practical; GN/depot_tools and a multi-gigabyte standalone source build are unnecessary for this approved alternative.
- Reuse historical source adapter/oracle definitions by reading them; new generated outputs stay ticket-local.

### What worked
- Fetch verified both interpreter pins and every downloaded Debian .deb SHA256.
- HOST BUILD PASS; three sequential direct C++ smoke runs ended PROBE PASS.
- ARM64 cross-link passed with GCC12; requested GLIBC versions do not exceed target's observed glibc2.44.
- Bellard/NG use true interpreters; V8 uses its default JIT and explicitly reports that distinction.

### What didn't work
1. First host build: `v8-probe.cpp:22:40: error: conditional expression between distinct pointer types ‘const uint8_t*’ {aka ‘const unsigned char*’} and ‘double*’ lacks a cast [-fpermissive]`. Fixed both ternary arms to const void* for the zero-byte buffer case.
2. First execution set LD_LIBRARY_PATH to the entire Debian host root. All three failed before main: `libstdc++.so.6: version 'GLIBCXX_3.4.32' not found`. The host uses GCC13 while Debian provides GCC12 runtime. Curated host bundle now excludes libstdc++/libgcc and uses installed matching host compiler runtimes. ARM builds use GCC12 plus the pinned Debian ARM runtimes. No system library was replaced.

### What I learned
- Dynamic dependency provenance must include transitive loader selection, not merely header/libnode pairing.
- V8 reports a 36MiB heap limit from 24MiB old/8MiB young constraints; generation constraints are not an exact total32MiB heap or RSS cap.

### What was tricky to build
- Persistent functions/context/script reset before isolate disposal; allocator survives isolate. Local handle scopes end before teardown; platform remains until all isolates are gone.
- Bellard lacks ng's typed-array type API. A captured intrinsic toStringTag getter reads the internal array brand before borrowing storage, rejecting BigInt64Array as well as wrong widths. No JS is called while bytes are borrowed.
- Native score validation and copying reject malformed results before accepting any numeric output; no gameplay state is mutated.

### What warrants a second pair of eyes
- No arbitrary/untrusted Promise queues are qualified: explicit V8 checkpoint drains one known finite job; QuickJS drains bounded known jobs. V8 does not expose a bounded per-job checkpoint through this API.
- Parse/compile timing is the engine API call, not all future lazy compilation or JIT costs. Automatic QuickJS GC counts remain unavailable; V8 records actual prologue observations, not inferred pause attribution.

### What should be done in the future
- Preserve final sequential host evidence, validate exact85 leaves and workload checksums, package per-engine bundles and review safe target commands.

### Code review instructions
- Read quickjs-probe.cpp, v8-probe.cpp, probe-common.hpp and workloads.js, then run 00-fetch.py,02-build-host.sh,03-build-arm64.sh,05-run-host.sh.

### Technical details
- Heap/stack/allocation bounds: QuickJS32MiB/512KiB; V8 old24MiB/young8MiB,stack512KiB; copied native buffer749 doubles; retained128×128 doubles; churn500 self-cycles per call.
- Historical quickjspp failures remain intact; this code neither uses nor forks that wrapper.

## Step 3: Expanded comparable workloads, validation and safe private packages

I completed the host qualification candidate with three sequential fixed-work runs of each real embedding, seven comparator regression tests, the current native12-test CPU suite and sanitized adapter smoke tests. All85 selected original leaves match Node exactly in the three engines; native maximum floating delta is2.220446049250313e-16. Every deterministic warm/cold/sustained workload checksum is checked, not just printed. The source routines remain extracted unchanged; benchmark/state/protocol wrappers are explicit.

The three ARM64 private bundles resolve all DT_NEEDED libraries and required bundled symbol versions through readelf, require no newer glibc than the observed target2.44 and contain no libc or system loader. SHA256SUMS and package archive hashes are retained. Transfer/run/collection scripts exist for parent invocation only, after review. They were NOT used against a device. Target qualification remains PENDING.

### Prompt Context
**User prompt (verbatim):** See Step 1.
**Assistant interpretation:** Deliver ready-to-review harnesses/evidence and conservative parent commands, with no production changes or invented hardware results.
**Inferred user intent:** Make actual target experiments safe and independently reproducible.
**Commit (code):** 5fbe46a44f8d670e33dcd58be1bf8f623da37621 — "research(js): qualify raw V8 and QuickJS host probes, prepare private ARM64 bundles"

### What I did
- Added native numeric callbacks, exact Float64 copy checks (including BigInt/wrong width/offset/nonfinite/oversize/empty/detached), source score output acceptance and malformed/property-get exception rejections.
- Added first-call +30 warmups +120 measured calls,128 scoring pairs per sourceRules call,300 sustained blocks and bounded retained/churn state.
- Moved oracle/protocol invocations after benchmarks so they do not pre-warm the row/scoring wrappers. Cold means first wrapper invocation in the prepared realm; shared dependencies can have preceding-workload warmth.
- Ran 05-run-host.sh plus repeat-2/repeat-3 without overlapping our own build jobs; generated source/node/native oracle and checksum comparisons.
- Ran 07-validate-host.sh: shell syntax/shellcheck, JS syntax, Python compile,7 comparator tests,12 native CPU tests, ASan/UBSan adapter smoke and host target-runner refusal.
- Added per-engine dependency closure/symbol-version/GLIBC checks, archive hashes and SHA256SUMS; hash-verified all three private bundles locally.
- Added parent-only credential-external transfer/collection scripts and safe target runner: aarch64,current256MiB MemAvailable,shared lock,existing-output refusal,180s timeout/5s kill grace,192MiB observed RSS threshold.
- Retained bounded evidence under scripts/02-arm64-v8/evidence, plus a derived actual-results summary. Raw build/dependency trees and ELFs stay ignored.

### Why
- 300 identical blocks mean2400 source rows,307200 scoring evaluations and600000 cyclic allocations during sustained stage, without a large heap or deliberate OOM.
- Native copied buffers and validated outputs avoid borrowed-pointer/game side effects. V8 explicitly controls one trusted finite microtask checkpoint; QuickJS's known queue drain has a count ceiling.
- Private pinned runtimes are not system installs; required notices accompany ignored packages.

### What worked
- All three sequential runs: PROBE PASS for Bellard/ng/V8,85 leaves each,7 workload checksum families. Native85 leaves pass with expected small numeric tolerance.
- Current native suite:12/12 passed. Comparator regressions:7/7 passed. Final sanitized adapters all pass with leak detection; underlying libraries are NOT instrumented.
- Host external peakRSS: Bellard6100–6356KiB,ng6488–6656KiB,V840552–40848KiB. Process wall times4.25–5.27s /5.14–6.24s /0.52–0.69s respectively.
- Row p50 ranges0.714–0.994ms /0.953–1.323ms /0.098–0.198ms, x86 only. SourceRules128-pair p50 ranges1.361–1.727ms /1.552–1.880ms /0.210–0.303ms. See exact p95/p99/max/rates in derived summary, not a target prediction.
- ARM64 cross-build, ELF/dependency/symbol version closure and package hash verification pass; actual target ldd/engine version/execution remain pending.
- Host target runner refuses with exact `REFUSED: target must be aarch64`, status2.

### What didn't work
1. Initial shellcheck failed with `SC2054 (warning): Use spaces, not commas, to separate array elements.` for -fsanitize=address,undefined; quoted that single argument. Intentional whitelisted client-side SSH expansion produced SC2029 informational findings; documented narrow suppression. Memory guard's A&&B||C produced SC2015; rewrote as if.
2. V8 sanitizer link initially failed: `undefined reference to 'typeinfo for v8::ArrayBuffer::Allocator'` (ld emits backtick quotes). Debian V8 does not export needed RTTI; disabled UBSan vptr for that adapter only, leaving ASan and other UBSan checks enabled. This is an instrumentation limitation, not evidence that every library is sanitized.
3. After expanding detached/output tests, default ASan quarantine ran until `FAIL RSS safety threshold exceeded`. Retained the exact bounded failure log separately. Kept192MiB gate unchanged and bounded host-only ASan quarantine to16MiB/thread256KiB; final three sanitized adapters pass. No device OOM/fatal allocation test was run.
4. `readelf -p .comment` on both libnode packages returned `Warning: Section '.comment' was not dumped because it does not exist`. Exact prebuilt V8 library compiler/flags therefore remain unavailable; harness compiler/flags and pinned binary provenance are recorded instead of invented.

5. Initial staged `git diff --cached --check` rejected readelf's generated trailing whitespace (first diagnostic `evidence/arm64-cross-link.log:17: trailing whitespace.`). Trimmed trailing whitespace only in the retained log; original generated .work receipt remains untouched. No measurements or diagnostics changed.

### What I learned
- Script compile APIs do not isolate all V8 lazy compilation/JIT work. First wrapper invocation, warmed measured calls, create/compile/execute/dispose and process startup wall time are distinct measurements.
- V8 GC callbacks supply actual observations; they do not attribute each latency tail. QuickJS automatic GC counters remain unavailable. External-memory bookkeeping need not immediately fall after one full GC.
- RSS safety observation is cooperative, not an OS hard cap. ASan memory overhead must not be mistaken for target VM footprint.

### What was tricky to build
- Bundle transitive version validation must check providers, not only matching SONAMEs. GCC13 host needs installed libstdc++ while GCC12 ARM can use pinned Debian libraries.
- UTF8 byte limits differ from JS string character counts; native validation uses UTF8 bytes. Native score output has source fields name/pts/ubers, not an invented label field.
- Known-job policies are explicit and honest about V8 checkpoint API limitations. No arbitrary Promise chain is exercised under a purported per-job budget.

### What warrants a second pair of eyes
- Fresh reviewer must inspect handle/exception lifetimes, source extraction/adapters, benchmark ordering/definitions, loader bundling and target script signal/lock ownership.
- Core engine libraries were not locally sanitizer-built. Fatal OOM, untrusted modules, interrupted partial game ticks, reload/generations and long replay/thermal behavior remain outside this slice.
- Parent must run true ARM64 ldd/version/85-leaf/checksum/RSS/tail/GC gates before accepting hardware qualification.

### What should be done in the future
- Parent reviews exact candidate patch, then transfers/runs/collects each engine sequentially. Preserve existing target results before any rerun. Do not stop desktop/game processes or rely on old preflight headroom.

### Code review instructions
- Begin at scripts/02-arm64-v8/README.md and quickjs-probe.cpp/v8-probe.cpp/probe-common.hpp/workloads.js.
- Reproduce host commands and package checks; inspect evidence/results-summary.json and individual run logs. Require target execution evidence separately.
- Verify production/history diff is empty and git index clean before applying candidate.

### Technical details
- Archive hashes/bytes and full dependency audits are in evidence/package-audit.json and evidence/results-summary.json; source/artifact SHA256SUMS are in ignored packages.
- Fixtures are pinned, all new useful commands live in this ticket, no third-party source/binaries or credentials are committed. Parent diary01/tasks/changelog/index remain untouched.
- No production graphics/audio feature changed, so graphics/device gate is explicitly deferred to parent rather than claimed run by this worker.
