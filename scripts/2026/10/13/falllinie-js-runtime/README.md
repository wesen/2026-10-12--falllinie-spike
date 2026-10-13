# Falllinie JS runtime experiments (research only)

Run from repo root. No production integration or device operation is performed.

```sh
P=scripts/2026/10/13/falllinie-js-runtime
bash "$P/00-download-pins.sh"
bash "$P/03-run-host.sh"
python3 "$P/04-compare.py"
bash "$P/06-wrapper-check.sh"
# Optional, requires the existing image ID embedded in script; no builds/installs/network:
bash "$P/05-cross-link.sh"
# Official page captures are network-dependent; returns nonzero for preserved 19-timeout 404:
python3 "$P/01-fetch-sources.py"
```

Prerequisites already available during research: GCC/G++13.3, Node24.18, CMake3.28.3, make, curl, Git, Python3. Defuddle is required only for captures. Native project ctest separately uses SDL2 development headers/libraries and pkg-config. Nothing installs dependencies. Cross-link script uses existing Debian12 ARM64 compiler image by image ID, `--network none`, and **does not execute target binaries**.

## Files and evidence

- `00-download-pins.sh`: checks Bellard 2026-06-04 and Duktape2.7 tar hashes; checks ng v0.17.0 and wrapper commits. Exact pins and attribution are in guide/diary and `licenses/`.
- `01-fetch-sources.py`: known official/upstream URLs only, per-fetch timestamp/resolved URL/hash; raw downloads ignored. Writes docmgr source captures and receipt. Optional arguments select capture names. Current 19-timeout URL remains a documented404. No Kagi success claimed.
- `02-extract-source.cjs`: unchanged frozen math/world/scoring/terrainRow bodies, no DOM; wrappers are explicit. Writes ignored `pure.js`, `inline.js`, oracle and source hash.
- `host.cpp` and `probe.js`: C++17 C API embedding for both pinned interpreters. Full-inline syntax, source/native oracle, typed row snapshot copy/offset/lease, wrong-width/nonfinite/oversize, exceptions, GC/cyclic workload, memory limit and deadline probes. No quickjs-libc capabilities.
- `native-oracle.cpp`, `native-bench.cpp`: selected original-vs-native values and comparative flat row work. Reuses actual native source, never modifies it.
- `duktape-probe.c`: checks expected compatibility rejections; exponentiation/Float64Array are expected successes.
- `04-compare.py`: exact labels/integers and floating tolerance abs1e-9 + rel1e-12. PASS85 leaves per engine/native comparison, max delta2.22e-16 native.
- `05-cross-link.sh`: two AArch64 ELF receipts; not hardware qualification.
- `06-wrapper-check.sh`, `quickjspp-smoke.cpp`: captures expected current-engine compatibility failures. Script exits0 after collection, prints inner compiler statuses1; that is NOT wrapper acceptance. Original bundled header and explicit include-only adapted ignored copy are tested separately.
- `evidence/`: bounded original receipts, three sequential baseline timing samples, explicit `concurrent-load-run.log` excluded from baseline, size/cross/native-test and failed-wrapper diagnostics. Scripts generate fresh output only in ignored `.work/`; committed evidence is intentionally not automatically overwritten.

## Expected outputs and limitations

`03-run-host.sh`: `HOST PROBE PASS` twice, `EXPECTED COMPATIBILITY REJECTIONS PASS`, comparison PASS. Both modern interpreters parse main inline script but fail browserless execution at document; HTML is syntax-invalid. Both release one external lease exactly once, reject a16MiB allocation under1MiB limit and interrupt an infinite loop at approximately20ms. This is not a frame budget.

`06-wrapper-check.sh`: bundled-header/current-library link failure; include-adapted/current-header compile failure. Do not patch production around these probes or confuse syntax-only compilation with link/run compatibility.

Measure repeats only when our own CPU build jobs are done:

```sh
for n in 2 3; do
  for e in bellard ng; do
    "$P/.work/host-$e" "$P/.work/pure.js" "$P/probe.js" "$P/.work/inline.js" > "$P/.work/$e-repeat-$n.txt"
  done
  "$P/.work/native-bench" > "$P/.work/native-repeat-$n.txt"
done
```

Host row p50 about0.71ms Bellard /0.97ms ng /0.0175ms native in the three sequential samples. Native allocation shape differs; no language-only or targetFPS conclusion. Explicit GC observed, automatic GC count/causality and processRSS not instrumented. V8/JSC not compiled/timed; whole JS simulation, hardened detached/resizable/type-safe protocol, modules, reload/migration and native script audio path remain design-only.

## Source licensing and storage

Full upstream source is scoped to ignored `.work/quickjs-2026-06-04`, `.work/quickjs-ng`, `.work/duktape-2.7.0`, `.work/quickjspp`. No downloaded engine source/binaries are committed except bounded attributed excerpts/source captures and MIT license notices. Wrapper is CC0 with its bundled QuickJS MIT; its README capture supplies that attribution. Original fixture is existing project evidence, extracted copies remain ignored. No ROMs, target libraries, secrets, uploads or pushes.
