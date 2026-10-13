# ARM64 / V8 embedding qualification candidate

**Research only. Host execution checked; ARM64 execution PENDING.** No production source/build/launcher changes. Parent performs transfer/execution only after fresh review. Historical `scripts/2026/10/13/falllinie-js-runtime` remains unchanged and is read for source adapters/native oracles.

## Reproduce from repo root

```sh
P=ttmp/2026/10/13/FALLLINIE-JS-001--javascript-embedding-research-for-the-native-falllinie-engine/scripts/02-arm64-v8
python3 "$P/00-fetch.py"           # pinned downloads/private extraction, no install
bash "$P/02-build-host.sh"        # also extracts unchanged source fixture
bash "$P/05-run-host.sh"          # three embeddings sequentially, then 85-leaf comparator
bash "$P/05-run-host.sh" repeat-2
bash "$P/05-run-host.sh" repeat-3
bash "$P/07-validate-host.sh"     # static/unit/native CPU/sanitized adapters
bash "$P/03-build-arm64.sh"       # cross-link ONLY, Docker --network none
python3 "$P/06-package.py"        # dependency/version closure and per-engine packages
python3 "$P/10-summarize.py"      # derive summary from actual receipts
```

Prerequisites: GCC/G++, make, CMake, Node (oracle extraction only), Python3, curl/network through Python urllib, Git, dpkg-deb, tar, readelf, GNU time/timeout, shellcheck; existing native CPU suite requires SDL2 headers/pkg-config. ARM build uses existing image `sha256:8068d5a56ed677c85ffd8e02f9ee4e88c9804d56d070aead0d598c44eb0f83dd`, GCC12.2.0-14. No image construction, package installation or target contact occurs above. New generated outputs/downloads all stay in ignored `.work`; scripts do not overwrite committed evidence.

`00-fetch.py` checks Bellard 2026-06-04 archive SHA256 and ng v0.17.0 commit `6d46d07d04041b40f4f49eaa7fdebe44c314c699`. Debian `.deb` URLs/versions/hashes for each architecture are fixed in `debian-pins.json`, selected from official bookworm/main Packages over HTTPS. Fetch never consults a moving index to change these pins. No wrapper fork. Historical quickjspp-only results describe older bundled-header/current-library failures and current-engine API drift, not all wrappers.

## V8 provenance (do not relabel)

`v8-probe.cpp` calls actual `v8::V8`, platform, isolate, context, script and function APIs directly. It links **Debian libnode108 Node18.20.4+dfsg-1~deb12u2**, with headers from that exact architecture/version's libnode-dev package. Host reports **V8 10.2.154.26-node.37**. ARM runtime version still requires execution. No Node executable/`node::` initialization/npm/browser host is used. Default V8 JIT is enabled: this is **not interpretation**, not a latest standalone custom V8 build.

Pinned prebuilt libnode + dependency packages are the reproducible library input, not a locally reproduced V8 build. Internal upstream/Debian library compiler flags and exact compiler version are **unavailable** here (`.comment` is stripped). Harness compiler/flags and actual dynamic selections are recorded. Host GCC13.3 needs host libstdc++/libgcc; curated host dependencies exclude Debian GCC12 runtimes. ARM GCC12 uses pinned private Debian ARM libstdc++/libgcc. Installed Ubuntu `/usr/include/node`/libnode.so.109 and nvm Node24 are NOT mixed with these libraries. Node24 is only the historical/source JSON oracle producer.

## Implemented probe contract

- Original 18,149-byte fixture extraction and original `oracle()` are reused unchanged; frozen HTML SHA256 `cb9b4befd0252859d9c45246f94fbd7dd1662c6d034c6ccb95fbe16947e8b556` is enforced. No source/RNG semantic rewrite or global fast-math. Original bodies generated in `.work/pure.js`; explicit adapters generated separately in `.work/probe.js`.
- 85 selected leaves: hashes, seeded RNG, heights/normals/paint, exact Unicode repeated-score cases, trees. Native oracle rebuilt against current production source without editing it. Exact integers/labels, doubles abs1e-9 + rel1e-12. This is NOT full simulation parity.
- Eight fresh runtime/isolate + context create/compile/execute/dispose cycles, each sanity-checked. `compile_us` is the engine source compile API duration, **not** all subsequent V8 lazy compilation/JIT work. `execute_us` runs top-level fixture/adapters. First cycle is process-first runtime creation, not platform initialization.
- Cached-function `cold` line is first call of that **wrapper in the prepared fresh realm**; preceding workload dependencies can already be warm (e.g. terrain uses hash/RNG). No oracle/protocol invocation precedes timings. Then 30 untimed warmups +120 timed calls per named workload. No-op isolates adapter call/scope/conversion cost; not a language-independent pure instruction cost. Every return contributes to checked checksum. Percentiles select sorted index floor((n-1)*p/100), durations include call plus native numeric conversion (QuickJS releases return handles; V8 scopes end).
- `batch`: unchanged terrainRow(3,4),107 columns ×7 doubles, one native-owned copied buffer and finite sum. `sourceRules`:128 deterministic scoring pairs (256 scoreAir calls), plus original hashInt/rng; G.recent reset is explicit adapter state, not gameplay integration. `scalarBridge`:749 numeric callbacks; `batchedBridge`:749 JS writes +one copied batch. Different JS loop work prevents a bridge-only speedup conclusion.
- Retain128 ×128 Float64 elements (~128KiB backing data) plus objects. `churn`:500 small self-cycles per call, each with16 values. Default sustained300 blocks =2400 rows,307200 scoring evaluations,600000 cyclic objects. Same fixed work across engines. Range1..600; target script accepts only bounded integer count. This finite run is not ten-minute thermal/full-game qualification.
- Native consume rejects wrong width, BigInt64, nonfinite, >749, empty, detached, non-view and DataView; checks subarray offset and native copy unchanged after JS mutation. Native score result accepts finite integer pts0..1e6, UTF8 name≤256 bytes, integer ubers0..16; malformed/property-get exceptions rejected. It consumes source score output; no game effects. One intentionally thrown exception is caught and subsequent evaluation succeeds.
- QuickJS roots values/owner through copy; ng uses exact typed-array type; Bellard captures intrinsic brand getter before borrowing. V8 uses exact IsFloat64Array, detached/bounds checks and shared backing-store owner. No JS calls while borrowing bytes; backing bytes never retained in game/native jobs.
- QuickJS limits32MiB runtime allocations and512KiB stack. V8 constraints old24MiB/young8MiB, `--stack-size=512 --expose-gc`; host reports actual heap limit36MiB. **None of these is an RSS cap.** RSS sampled each sustained block, final kernel peak gated below192MiB. This is cooperative observation, not an OS-enforced hard memory boundary. No deliberate/fatal allocation test on target.
- QuickJS does not automatically execute jobs; known single Promise job explicitly drained with4-job ceiling. V8 kExplicit microtasks +one known finite checkpoint. Arbitrary promise queues, native blocking calls, modules and interruption are NOT qualified by this slice; V8 checkpoint cannot enforce a per-job budget through this API. Outer process timeout bounds trusted fixtures only.
- GC: QuickJS computed allocation/object metrics +explicit JS_RunGC wall duration; automatic GC count/pause causality **unavailable**. V8 actual GC prologue observations +explicit request count, heap/external stats, full RequestGarbageCollectionForTesting wall duration. These counts are not inferred pause attribution; external-memory bookkeeping need not drop after one GC. No claim every allocation is instrumented.
- Platform outlives all V8 isolates; persistent functions/script/context reset before Dispose; allocator survives isolate disposal. No SDL/GL/audio calls exist in these harnesses.

## Evidence and limitations

Measured x86 host p50 ranges across three sequential runs: row Bellard0.714–0.994ms / ng0.953–1.323ms / V80.098–0.198ms; sourceRules(128 pairs) Bellard1.361–1.727ms / ng1.552–1.880ms / V80.210–0.303ms. External process peak RSS ranges: Bellard6100–6356KiB / ng6488–6656KiB / V840552–40848KiB. All engines/native match85 leaves; modern embeddings have max float error0, native2.22e-16. Full metrics/rates/percentiles/hashes in `evidence/results-summary.json`. No target timing exists.

`evidence/host/` contains three sequential samples, environment, true process wall time (GNU time, includes platform/teardown), native oracle/row benchmark, comparison receipts. Probe internal elapsed excludes platform init/final teardown and includes source read through final GC. Process elapsed/ru_maxrss and /proc VmRSS can differ slightly; keep both, not an invented common value. Host frequency/governor/load is not locked; timings are not ARM predictions. Native row allocation strategy differs from JS and is labelled as such.

`evidence/host-build-provenance.log`: harness compiler/flags, upstream Bellard make recipe, actual ng C flags, CPU, host ldd selections. `evidence/arm64-cross-link.log` (retention trims trailing whitespace only; original generated .work log unchanged): ELF64 AArch64, target loader, DT_NEEDED/RUNPATH/ABI version requirements. `evidence/package-audit.json`: every bundled library's AArch64 identity, dependency/version closure and required GLIBC≤2.44. This does not prove target loader acceptance; target `ldd` runs only in parent-owned preflight.

`07-validate-host.sh` runs existing native12-test CPU suite and7 comparator regression tests. Sanitizers instrument our C++ adapters, **not** prebuilt third-party libraries; V8 UBSan vptr is disabled because Debian libnode does not export required RTTI. ASan leak detection with16MiB quarantine/256KiB thread quarantine bounds host instrumentation overhead. Default quarantine previously tripped RSS safety threshold; receipt retained separately, not a target result. Sanitized timings are excluded from performance evidence.

No HTML/browser/full physics replay, hot reload, modules, zero-copy leases, fatal OOM recovery or production integration is claimed. Historical interrupt/OOM/lease/wrapper receipts remain discoverable in the old experiment. No graphics/hardware feature was changed; parent owns actual device/graphics/audio qualification.

## Parent-only transfer, run and collect (after reviewer approval)

Bundles under `.work/packages/{bellard,ng,v8}-arm64.tar.gz` contain executable, fixtures, manifests, SHA256SUMS, licenses and private library closure. Do not commit them. Libraries exclude libc and loader; target's system loader/libc/libm are preserved. libnode requires transitive Node/ICU/OpenSSL libraries despite using raw V8 APIs. Node's JS builtin packages are not bundled or initialized. No system installs/replacements.

Credentials use SSH agent/config externally. Optionally set `PROBE_SSH_CONFIG` to a local config path; never record its content, passwords or environment dumps. Transfer refuses existing engine directory/archive; no automatic probe execution.

```sh
# SSH_DEST is an externally configured host alias or user@host, not a password.
bash "$P/08-deploy.sh" SSH_DEST bellard
# On target, one engine at a time; no services/processes stopped:
bash /storage/apps/falllinie-js-probes/bellard/run-target.sh 300
# Back on host:
bash "$P/09-collect.sh" SSH_DEST bellard
# Repeat transfer/run/collect sequentially for ng, then v8.
python3 "$P/04-compare.py" "$P/.work/target-results"
```

Manual SSH command uses the parent's existing transport/config. Runner requires aarch64, current≥256MiB MemAvailable, hash verification, shared atomic lock, refuses overwriting result.log, logs only selected nonsensitive hardware/dependency fields, and applies180s timeout +5s kill grace to its own process. Signal handling terminates only its owned timeout/probe. Never remove a lock while a probe might be alive. A failed/repeated run must be archived by parent before replacing that private bundle. No enduring native-process/headroom state is assumed from the historical target-preflight.log.

**Remaining target gates:** actual loader/version correctness, 85 leaves +workload checksums, RSS<192MiB, startup/call/tail/GC observations under real hardware conditions and parent acceptance. Longer replay/thermal/coexistence/production bridge hardening are outside this slice.
