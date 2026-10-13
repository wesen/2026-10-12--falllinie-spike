---
Title: JavaScript runtime research and integration design
Ticket: FALLLINIE-JS-001
Status: active
Topics:
    - research
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://fixtures/falllinie.html
      Note: Frozen original syntax and pure logic
    - Path: repo://scripts/2026/10/13/falllinie-js-runtime/04-compare.py
      Note: Independent source/native value checks
    - Path: repo://scripts/2026/10/13/falllinie-js-runtime/host.cpp
      Note: Measured C API ownership limits and bridge experiment
    - Path: repo://src/audio.cpp
      Note: Numeric bounded callback boundary
    - Path: repo://src/game_renderer.cpp
      Note: Generation cancellation and native render/state seam
    - Path: repo://src/runtime.cpp
      Note: Existing authoritative fixed tick and feedback sequence
ExternalSources: []
Summary: Evidence-backed ECMAScript embedding options and a proposed native-host contract for the actual Falllinie SDL2/GLES3 engine.
LastUpdated: 2026-10-13T14:30:00Z
WhatFor: Choose whether and where to retain authored JavaScript without restarting the native port.
WhenToUse: Before implementing a script runtime, bridge, reload path or browser-host alternative.
---

# JavaScript in the actual Falllinie engine

## 1. Recommendation and scope

**Keep the existing native game as the shipping candidate. Do not introduce JS merely to recover trees or trails: those systems already exist in candidate `24b50d8`.** Their source merge is not their device deployment. The parent's last console preflight was unreachable, and this research has no hardware acceptance authority.

Embedding means the C++ application creates and owns a JavaScript runtime: an interpreter and its managed object heap. A binding is a native function made callable from script. A snapshot is a copied, versioned description of state, not a second mutable owner. Garbage collection (GC) reclaims unreachable script objects; an application binary interface (ABI) specifies how the host and script exchange values. These distinctions matter because enabling JavaScript does not transfer control of rendering, input or audio to a browser.

If retaining authored JS is a desired development capability, start with **an optional, pure scoring/trick-rule module behind a narrow C API adapter**, preserving native rendering, terrain, meshes, particles and real-time audio. Use **Bellard QuickJS 2026-06-04 as the first experimental implementation pin**, with QuickJS-ng v0.17.0 kept as a measured alternative. This is a research recommendation, not authorization to change production architecture. Both pass actual source compatibility; Bellard is smaller and faster on the measured host row workload, while ng's active release stream and tooling are attractive. No target performance winner is established.

Do not adopt quickjspp unmodified with these pins. Its latest observed commit bundles QuickJS 2025-04-26; our smoke probes fail with current Bellard/ng APIs. Do not choose Duktape for unchanged HTML logic: required arrows, class syntax, destructuring and Map fail stock 2.7.0 probes. Do not choose V8 or JavaScriptCore solely because they JIT: neither was built or timed for this device, and both require a substantially heavier dependency and lifecycle qualification effort.

The guide distinguishes:
- **Observed:** frozen fixture/native source, official captures, tests and cross-link receipts.
- **Proposed:** APIs, ownership partitions, budgets, failure/reload policy and future files.
- **Unqualified:** device scheduling, thermal behavior, graphics acceptance, actual JSC/V8 build footprint and full simulation parity.

No production `src`, CMake, game assets, fixtures, or FALLLINIE-GAME-002 metadata were modified. No installs, device connection, deployment, browser restart, push or publication occurred.

## 2. What is already ported, and what is still different?

The original HTML is a single browser application with inline ECMAScript, CSS, embedded fonts, WebGL2 shaders, WebAudio synthesis, DOM screens and input handlers. The port is a C++17 SDL2/GLES3 application with native source-shaped equivalents, plus dual-screen/first-person functionality. Treat source code as more authoritative than prose in the HTML design-notes screen: some equations in the notes are older than the implementation.

### Native versus browser inventory

| Feature in original | Candidate source evidence | Status and remaining gap |
|---|---|---|
| Bank trees, grey cones, occasional course hazards | `src/world.cpp:18-40` `World::trees`; `:63-69` `World::objects` | 38 bank trees/cell plus filtered hazards; same definitions serve collision and geometry. Source present, device visuals/collision unqualified. |
| Tree collisions, bail tumble, recovery | `src/game.cpp:184-194` `hit_trees`, `do_bail`, `bail_step`; `:45-56` place | Source present; seeded effects instead of browser Math.random is an explicit adaptation. |
| Snow spray and rail sparks | `src/particles.cpp:5-10`; gameplay ground/land/rail/bail emitters | Bounded at 700 particles; billboards built per native view. Source present, not deployed evidence. |
| Board trail and discontinuities | `src/particles.cpp:11-14`; `src/renderer.cpp` `trail_mesh` draw and polygon offset | 0.35m spacing, 0.11m half-width, source simulation-time aging; native break-before flag replaces null trail sentinels. Source present. |
| Rails, gates, procedural terrain/kickers/cliffs/halfpipe | `src/world.cpp`, `src/terrain.cpp:20-76` | Native definitions, collision and object rendering exist. Cooperative native mesh job differs usefully from browser's synchronous finish step. |
| Spins/flips, ten grabs, six übers, combo/repetition/meter/GROTESK | `src/game.cpp:7-19,114-176`; `src/pose.cpp:10-44` | Source-shaped native logic and rig exist; oracle suite is not a proof of every input trajectory. |
| Music, ride feedback and procedural sound | `src/audio.cpp:27-427`, `src/audio_dsp.hpp` | Native bounded DSP/scheduler rather than browser nodes/timer. Event semantics are ported; signal-identical browser audio is not claimed. |
| Six print palettes and X-ray shader survey grid | native scene/post shaders and renderer | Colour/survey-grid shader path is not the full browser 2D X-ray maths overlay. |
| X-ray skeleton callouts, vectors, predicted flight, live equation labels | fixture `drawXray`, lines 1824-1954 | No equivalent Canvas2D overlay was found in native source. A distinct future feature, not solved by embedding ECMAScript alone. |
| Title, pause, score, tricks, combo and meter HUD | `src/game_renderer.cpp:14-40` | Native ASCII source-font presentation, known Unicode labels transliterated for display only. Browser Archivo/Plex variation, layout, animation and exact typography differ. |
| Notes screen, swatches, browser click panels | fixture lines 202-302, 1481-1550, 1775-1802 | Native HUD isn't the complete DOM/CSS design-notes experience. |
| Browser touch/pointer controls | fixture lines 1555-1615; native `src/runtime.cpp:3-32` | Native keyboard/raw SDL controller mapping exists; browser virtual stick/buttons are not native touch integration. |
| Best/palette persistence | `Game::set_best_path/save_best`; `src/main.cpp:98-107` | Native validated best-score file exists. Browser localStorage palette persistence not found; colour cycling is not persisted equivalence. |
| First-person lower view / dual SDL windows | `Renderer::render_first_person`, `src/main.cpp` | Native extension, not original HTML feature. |
| Cloudflare beacon script | fixture final line | Deliberately not needed in an offline native host; never fetch/execute analytics as part of extraction. |

Check exact symbols with `rg -n 'trees|trail_mesh|snow_mesh|Xray|xray' src fixtures/falllinie.html`. The parent owns the final gap prioritization and acceptance list. This ticket must not restart the completed native implementation or call source presence complete game parity.

## 3. ECMAScript is not a browser

A runtime implements syntax, objects, standard built-ins and execution. An embedder supplies capabilities such as clocks, event delivery, module resolution and external resources. DOM, `window`, WebGL, WebAudio and localStorage are **host APIs**, not ECMAScript language features.

```text
HTML document = markup + CSS + fonts + inline JS + external beacon
                             |
           browser host      |      ECMAScript engine
    DOM/layout/canvas/input  |  syntax/Map/Math/Array/Promise
    RAF/WebGL/WebAudio/store |  evaluation/heap/GC/functions
                             |
Native Falllinie host = SDL input + fixed clock + GLES renderer + DSP + files
                        (these do not appear just by calling JS_Eval)
```

Measured proof: both QuickJS variants compile the **whole main inline script** without syntax errors. Evaluating it in a fresh context fails at `document`; evaluating literal HTML fails at `<meta`. Fake `window={}` or a no-op RAF doesn't make the renderer, audio graph or HUD work. Stubbing browser functions may hide missing behavior and is therefore not a port strategy.

### Required source features

The actual script uses `const/let`, arrows and closures; classes and default arguments (`MB`); array/object destructuring, spread, for-of and iteration (`Map`, `Set`); computed object fields; template literals; exponentiation; Math.imul/hypot/sign/tanh; Unicode labels; Float32Array/Uint32Array for mesh transfer; and Promise chaining through document.fonts. It does not require JSX, npm packages, TypeScript or a DOM framework. ES modules are not needed by the gameplay bodies (the separate beacon is irrelevant), though modules are recommended for extracted authoring files. Async scheduling should not be enabled merely because Promise exists.

Native translation must reproduce JS `|0`, `>>>0`, Math.imul wraparound, remainder/negative-cell floor, Map insertion order, Math.round behavior and caller-selected normalization fallbacks. The seeded `rng/hashInt` terrain pathway differs from unseeded Math.random effects/music/autopilot. Avoid fast-math: Number semantics and floating-point threshold stability matter more than unchecked arithmetic throughput.

## 4. Runtime tradeoffs, versions and maintenance

Capture IDs below refer to `../sources/NN-*.md`; `fetch-receipt.json` has exact UTC timestamps, final URLs and raw hashes. All captures were fetched 2026-10-13. They are official/upstream direct URL evidence, **not successful web search**. Parent Kagi discovery failed: `surf.sock connection refused at /home/manuel/snap/chromium/common/surf-cli/surf.sock`. Initial direct 404s and one supplemental timeout-page 404 are retained; see diary.

| Option | Actual compatibility | Lifetime, errors, limits | ARM64/build/footprint | Maintenance/license | Decision |
|---|---|---|---|---|---|
| Native mechanical translation | Existing C++ source and six CPU tests; selected JS oracle parity | C++ RAII; no VM/GC; ordinary bounds/errors still needed | Existing device toolchain; native row far faster in host probe | Project ownership; translation has drift cost | Keep current shipping candidate; source oracle maintenance remains valuable. |
| Bellard QuickJS 2026-06-04 | Full inline parse + original pure functions PASS; official ES2025 claim (01/02) | Owned JSValue refs; JS_EXCEPTION/GetException; memory/stack limits, interrupt callback; explicit cycle GC | Host and AArch64 cross-link PASS; measured whole probe image ~0.98MB text+data+bss x86 | MIT; active 2026 release, not dormant (03) | First optional rules experiment, not target qualification. |
| QuickJS-ng v0.17.0, 6d46d07… | Same fixture tests PASS; current latest release API capture (18) | Similar concepts, **different C ABI**: rt-scoped class IDs, ArrayBuffer resize callback; limits/interrupts PASS | CMake target qjs; host/AArch64 link PASS; whole x86 probe ~1.22MB | MIT; maintained fork, releases and bug fixes evidenced in 18 | Alternative worth device A/B and tooling review. Do not mix headers/libraries/bytecode with Bellard. |
| Duktape stock 2.7.0 | Arrows/class/let destructuring/Map FAIL; exponentiation and Float64Array PASS | Value stack/stash roots, protected pcall/peval; allocator budget; compile-time execution timeout; refcount + mark/sweep | Small C distribution, ARM portability documented, host build PASS; probe ~0.42MB | MIT; fetched guide is older 2.6 and incomplete extraction, topical docs 20-22 more useful | Rejected for unchanged source; transpiling and Map/polyfill semantics erode size/maintenance advantage. |
| JavaScriptCore | Modern engine should support needed syntax; **not tested here** | JSGlobalContext/JSContextGroup retain/release; JSValueProtect/Unprotect; exception out-parameters; typed bytes pointer temporary | Linux integration through WebKitGTK/WPE or standalone JSC; dependency/toolchain work much larger. No cross-build receipt | Header BSD-style; full WebKit LGPL/BSD mix requires actual distribution audit. GTK 2.54.1 advertised (17); main API captured | Consider only if interpreter budget fails AND dependency/platform support is justified. |
| V8 | Modern engine should support source; Node oracle is not a custom V8 embedding test | Isolate/Context, scoped Local and persistent Global handles, TryCatch/MaybeLocal; resource constraints, termination, explicit microtasks | GN/depot_tools/host tools/sysroot and target_cpu arm64; 10 documents cross-build. No local build/size/timing | BSD-style + dependencies (25). Main 15.8.0.0 is candidate, not stable pin | Heavy alternative only after profiling proves need, not “fast therefore best”. |
| quickjspp latest observed 01cdd304… | Header-only C++17/CC0 wrapper; bundled 2025-04-26 | RAII/typed conversion/module/class conveniences; template instantiation may obscure copy/lifetime costs | Bundled header passes syntax smoke; current-library link fails, current-header adapted compile fails | Latest observed commit 2025-07-04, fetched README (14); no broad maintained-wrapper survey proven | Do not adopt unchanged or silently fork it. Narrow local adapter for this bridge is simpler. |

Footprint numbers are **GNU size for these whole probe executables**, not stripped release package sizes, engine-only text or RSS. Bellard home advertises 367KiB for a tiny hello-world; that is not our retained-feature workload. JSC/V8 footprint is qualitatively larger by build/dependency structure, not a measured MB claim. License notices for three downloaded engines are under experiment `licenses/`; preserve upstream copyrights in source captures. Wrappers do not erase engine license obligations.

### API primers and source anchors

**QuickJS:** Runtime is a heap, Context a realm. Runtimes cannot exchange JS objects and a runtime is not concurrently thread-safe (02 §3.4.1). Eval/Call/getters return owned values; free them, duplicate retained values, and remember property setters commonly consume the value. Function arguments are borrowed. Check exceptions and conversion return codes before use. Keep context/runtime alive longer than every retained value. Finalizers can release native resources but cannot execute JS (02 §3.4.6). `JS_SetMemoryLimit`, `JS_SetMaxStackSize`, `JS_SetInterruptHandler` and `JS_RunGC` are useful controls, not hard real-time guarantees. Engine memory accounting is not total native mesh/external-buffer memory; budget those separately.

`JS_GetTypedArrayBuffer` returns an owned backing-buffer root and offset/length/element-size information; pointer access is borrowed. Do not call arbitrary JS while a pointer is borrowed: detach/resize/reentrancy can invalidate assumptions. Bellard external buffers take a free callback; ng 0.17 takes max length and `JSReallocArrayBufferDataFunc` (05, header source lines 1076-1093). Our explicit ng adapter refuses nonzero-size resize and frees once at zero. ng's `JS_NewClassID(rt,&id)` also differs. A runtime pin includes matching headers, compile options and architecture assumptions, not only a version string.

**Duktape:** A value on the stack/stash roots it; a raw heap pointer does not. Native APIs operate on a stack and may throw via longjmp. `duk_pcall`/`duk_peval` and carefully protected native boundaries are essential; never longjmp through C++ objects with active destructors. `duk_create_heap` allocator hooks can impose a budget; stock timeout configuration is disabled in downloaded duk_config.h. The sandbox page (21, “Bytecode execution timeout details”) says keep reporting timeout until the original protected call exits; native blocking C functions aren't preempted. Modules are host facilities/CommonJS extras, not a turnkey ES module loader (20). Transpilation must account for iteration and Map ordering, not only rewrite `=>`.

**JSC:** `JSGlobalContextCreateInGroup`, retain/release, and `JSValueProtect/Unprotect` expose explicit rooting (11/12). C functions and evaluation propagate exception values through out-parameters. `JSObjectMakeTypedArrayWithBytesNoCopy` requires a deallocator, including documented deallocator-on-exception behavior; `JSObjectGetTypedArrayBytesPtr` is explicitly not guaranteed valid across JSC API calls (13, source lines 91-98). The execution-time API exists in **JSContextRefPrivate.h** (23), not a portable public-host promise. No equivalent to QuickJS per-runtime hard allocation limit is established from the captured public C headers. A chosen Linux GLib/port API may offer different controls: prove them against the intended distribution before design approval. Context group may need a run loop; no concurrent access without host ownership discipline.

**V8:** Local handles die with their HandleScope; persistent/Global handles must be Reset. Use EscapableHandleScope when returning a local created in a scope (09). Check MaybeLocal/empty returns and TryCatch, rather than ToLocalChecked on fallible script calls. Isolate ResourceConstraints guide heap behavior; they are not a process-wide or exact OOM recovery guarantee (24). `TerminateExecution` can signal running JS; cancellation/recovery and native callbacks require explicit policy. ArrayBuffer BackingStore is lifetime-managed with allocator/shared ownership; scope-safe pointers are still necessary. ES module resolution and promise/microtask scheduling belong to the embedder. V8 creates no DOM or SDL bindings by itself.

### GC and predictability

QuickJS refcounting deterministically frees acyclic objects, but destruction cascades can be expensive and cycle removal is synchronous work. Duktape similarly combines refcounts with mark/sweep. JSC/V8 tracing collectors and JIT tiers can improve long-running throughput but add compilation, background work, code-space/RSS and pause behavior that must be profiled. None is hard real-time. Preallocating typed snapshots reduces cross-boundary allocation but does not remove original terrainRow's nested arrays, closures, Map caches or cycle churn. An explicit post-frame GC call cannot promise there will be no automatic collection inside a tick.

## 5. Experiments: what was actually measured

Experiment root: `scripts/2026/10/13/falllinie-js-runtime/` (called **P** below). Reproduction:

```sh
bash "$P/00-download-pins.sh"
bash "$P/03-run-host.sh"
python3 "$P/04-compare.py"
bash "$P/06-wrapper-check.sh"   # captures expected wrapper incompatibility
bash "$P/05-cross-link.sh"      # optional existing image, never runs target ELF
```

Assign P to the literal repo-relative path above, or copy the concrete commands from the README. No dependency installation occurs. `.work/` holds downloads/extracted JS/builds and is ignored. The three original license notices and bounded evidence are tracked. Source body extraction uses unique fixture markers and leaves fixture bytes unchanged. Wrappers explicitly set SEED=42, clear caches, create only G.recent and batch buffers; they are not edits to the source math/scoring bodies.

### Correctness and control results

- Source fixture SHA256: `cb9b4befd0252859d9c45246f94fbd7dd1662c6d034c6ccb95fbe16947e8b556`.
- Node/Bellard/ng/native oracle: 85 leaves each, exact labels/integers. Max float delta Bellard/ng 0, native 2.22e-16; tolerance abs1e-9 + rel1e-12. Five terrain locations include a negative z, two trees from cell3, and two repeated scoring cases. This is selected-value coverage, not full simulation replay.
- Full inline compile-only passes both interpreters; browserless eval fails as expected. No browser API compatibility was fabricated.
- Numeric batch rejects wrong width, NaN, oversize; offset and copied values pass. Rooted external buffer survives GC and releases exactly once after dropping roots. Prototype is trusted-only: 8-byte element width isn't complete Float64 type checking, detachment/resizing/adversarial input and OOM at every bridge allocation remain future tests.
- 32MiB runtime limit and 512KiB stack limit configured in main probe. Separate 1MiB runtime rejects a 16MiB ArrayBuffer with OOM, then evaluates arithmetic. Infinite loop interrupted after ~20ms; no attempt to preempt a blocking native callback.
- Existing native suite: 6/6 passes (`evidence/native-tests.txt`). No graphics/hardware gate run.
- Both AArch64 ELFs cross-linked in existing image `sha256:8068d5a56ed677c85ffd8e02f9ee4e88c9804d56d070aead0d598c44eb0f83dd`, no network inside container, no execution. Interpreter `/lib/ld-linux-aarch64.so.1`; libc/libm/libstdc++/libgcc dependencies. Target loader/library versions and execution are still unqualified.

### Host microbenchmarks, not target FPS

x86_64 host, GCC13.3, `-O2` C++ host, upstream C defaults (Bellard make `-O2`, ng CMake Release), 30 warmups + 300 sampled calls, steady_clock. No CPU affinity/thermal governor qualification. First baseline after our build jobs completed:

| Work per call | Bellard p50 / p95 / p99 µs | ng p50 / p95 / p99 µs | Native |
|---|---:|---:|---:|
| Cached no-op JS_Call | .025 / .026 / .027 | .028 / .031 / .039 | Not compared |
| Original terrainRow 107 columns, 749 doubles packed, copied consume | 707.645 / 890.694 / 1399.510 | 962.927 / 1032.170 / 1108.690 | 17.537 / 17.655 / 23.098 µs |
| 749 scalar JS→C callbacks | 21.018 / 38.753 / 42.658 | 28.785 / 32.856 / 54.954 | Not compared |
| One callback, JS writes 749 doubles, native copies/sums | 20.552 / 24.360 / 31.510 | 24.748 / 27.976 / 31.077 | Not compared |
| Allocate 1000 self-cyclic objects | 110.541 / 123.244 / 128.238 | 89.548 / 111.140 / 124.932 | Not compared |

Repeats 2/3 row p50: Bellard 714.805/713.393µs; ng 975.141/967.724µs. Report all runs; do not cherry-pick tails or advertise a universal engine speed ranking. Native reuses a flat output buffer instead of constructing original nested JS arrays, so this is a useful architecture cost comparison, not a controlled language-only benchmark. Bridge comparisons also include different writing-loop costs. A cached no-op isn't a complete 120Hz tick.

One explicit GC: Bellard 31.594µs, used bytes 262241→184201; ng 22.088µs, 262856→204840. Cyclic object counts fell 1210→398 and 978→421. These show collection of garbage, not automatic GC count or maximum pause bounds. JS_ComputeMemoryUsage excludes full native/process RSS. A concurrent-build sample is preserved separately (`concurrent-load-run.log`) and deliberately excluded: it inflated ng row p50 to 2299µs. This demonstrates why host numbers cannot be extrapolated to device acceptance.

Whole ELF size text+data+bss: x86 Bellard978803B, ng1221832B, Duktape421942B; ARM cross Bellard901092B, ng1101363B. Build settings differ; no stripped shipping artifact or JSC/V8 comparison is claimed.

## 6. Architecture choices and decision records

### ADR-1: Ownership is a product/architecture choice, not a binding detail

**A: Keep mechanical translation.** Native Game remains authoritative; original HTML and extraction scripts serve oracle tests. Lowest runtime risk, strongest current terrain speed, but two authored implementations require parity discipline.

**B: JS rules or simulation with native renderer/mesh/audio.** Lowest-risk first slice is a pure score/trick function: C++ passes an immutable air summary; JS returns score and label; **native applies it once**, owns recent/combo/meter/persistence. JS doesn't retain an authoritative G.recent. To preserve original scoreAir unchanged, each rules call can receive a copied recent-label list and assign it to temporary adapter state; original scoreAir may mutate that temporary copy, but native alone commits history once after validation. If later the full simulation is moved, JS becomes the sole authority for R/G/input edges/RNG/rules, and C++ becomes a validated snapshot/command consumer. Don't concurrently update both native Game::step and JS stepRider. Shadow comparison runs may calculate both, but only one publishes events or persistence.

**C: Full browser host.** WPE/WebKitGTK/Chromium-like WebView could supply DOM/CSS/Canvas/WebGL/WebAudio and retain far more unchanged HTML. This replaces or surrounds the SDL renderer with another graphics/process/input stack, affects dual-window placement, compositor ownership, GPU buffers, audio backend and update/security maintenance. It is not “embed JSC”. HTML font/layout fidelity may justify it for another product; it is disproportionate to this game candidate without a separate hardware feasibility ticket. No browser-host build/device probe was performed here.

**Recommendation:** A ships while B is optional exploration. C requires explicit parent approval and new acceptance scope.

### ADR-2: Native terrain remains authority in the first JS-owned simulation design

Measured interpreter terrain rows are much slower than current native rows. Therefore do **not** route every `TerrainJob` height/mesh query through JS merely to claim a JS port. Native Terrain/World definitions stay authoritative for both collision and render geometry; extracted script physics calls a controlled native height/normal/cell snapshot API. Original JS terrain remains the oracle, not a second live producer. Crossing for a few collision/camera queries may be affordable; measure it on target. One combined `sampleSurface(x,z)` can return height+normal+params instead of many scalar calls where call-site ordering permits; preserve exact query coordinates and fallback behavior.

If retaining JS terrain is itself the approved goal, switch BOTH script collision and native row sampling to that provider together. Native consumes `terrainRows(cell,start,count)` snapshots and builds topology; disable native height definitions for live gameplay. This alternate mode has its own source-of-truth and performance gate, not a silent cache layered atop different terrain math.

### ADR-3: Narrow adapter before a general class-binding layer

Use a small `ScriptRuntime` owner and functions accepting plain numeric schemas, not exported mutable `Game*`, `World*`, vectors or renderer objects. That prevents stale references, opaque ownership and per-particle crossing. RAII implementation is reasonable; broad future-proof runtime polymorphism isn't required. The prototype's compile-time API difference is research comparison scaffolding, not a proposal to ship two engines at once.

## 7. Proposed concrete engine integration contract

All code names in this section are **future API sketches**, not implemented production interfaces.

```text
main thread (SDL/GL owner)
  NativeInputMapper → FrameClock / application actions
                             ↓ Controls snapshot
                      ScriptRuntime (owned here)
                    optional rules OR simulation
                  ↓ validated native-owned snapshots
  Terrain/World → mesh jobs → Renderer → GLES / two SDL views
                  events → publish_feedback → AudioEngine SPSC queue
                                                  ↓
                                         real-time audio callback
                                         native numeric DSP ONLY
compositor worker: IPC ONLY; no runtime, no SDL video, no GL
```

A separate JS worker is not recommended initially: the current main-thread jobs and source-shaped clock make ordering easier to prove. If approved later, give it its own runtime and copy numeric messages; no JSValue crossing and no GL calls. A worker can't make slow simulation synchronous deadlines disappear.

### File-level seam map

| Existing seam | Proposed responsibility/change if phase approved |
|---|---|
| `src/main.cpp:90-160` | Create/destroy ScriptRuntime main thread; act on start/pause/restart before ticks; keep SDL video/audio lifecycle and both render views native. |
| `src/runtime.cpp:48-62` / `runtime.hpp` | Retain authoritative fixed accumulator; optional overload targets one simulation provider. Native title autopilot stays or moves as a complete RNG/input-owned unit. |
| `src/game.hpp`, `game.cpp` score helpers | First pure rule adapter; native AirState→immutable summary, validate result, native repetition/combo mutation once. Full simulation phase replaces authority rather than “syncing” public fields both directions. |
| `src/terrain.hpp/cpp` | Keep native height/normal/cell definitions first; add batched surface provider only if approved. TerrainJob owns no borrowed JS buffers. |
| `src/world.hpp/cpp` | Shared immutable per-cell rails/trees; active rail owns a copy. No retained references into evictable cell maps. |
| `src/pose.hpp/cpp` | Consume complete pose/air snapshot or explicit animation summary; keep native IK/geometry unless pose port is independently approved. |
| `src/game_renderer.cpp`, `renderer.hpp/cpp` | Split immutable RenderSnapshot read from terrain streaming; snapshot contains pose/effects/HUD metadata, not a shadow mutable Game. All uploads/draws main-thread. |
| `src/feedback.hpp`, `audio.hpp/cpp` | Validate enum/numeric event batch main-thread; reuse trigger/set_ride and bounded SPSC. No strings/JS handles/script evaluation in callback. |
| `src/profiler.hpp/cpp` | Add script_eval/tick/snapshot_copy/terrain_bridge/GC CPU markers and counters; preserve async nonwaiting GPU queries. |
| Proposed `src/script_runtime.hpp/cpp` | One runtime owner, RAII values, restricted loader, exception/interrupt/memory policy. No SDL/GL in finalizers. |
| Proposed `tests/test_script_rules.cpp`, `test_script_lifecycle.cpp`, `test_script_replay.cpp` | Host correctness first; no graphics dependency needed for pure rules. |

Current `Renderer::update_player(Game&)` reads many public fields and synchronously owns streaming, camera/pose and upload work. A script snapshot cannot be substituted by writing random fields into Game: private rail/input/safe timers and RNG would still be missing. Review this seam before full simulation migration; a render-only immutable state type is a coherent later change, not part of this research.

### Proposed ABI v1: snapshots and commands

Separate Float64 simulation values from Float32 mesh storage. JS Number and native double perform simulation; cast to Vertex's **11 floats / 44 bytes** only at mesh construction (`terrain.hpp:38-39`). Quaternions are `[x,y,z,w]`, board X=nose/Y=normal/Z=toe; arrays cannot silently reorder axes.

```cpp
// Conceptual host-facing structures, not packed C++ layout exposed to JS.
struct Header { uint32_t abi, generation, tick, count; };
struct ScriptControls { double steer, drive; uint32_t flags, grabs; };
struct SurfaceBatch { std::vector<double> xyz_height_normal_paint; };
struct NumericEvent { uint32_t kind, flags; double amount; int32_t points; };
struct RenderSnapshot {
  uint64_t generation, tick;
  // finite position3, velocity3, q4, qVis4, boardNormal3,
  // crouch/lean/steer/grab amount/mode/active uber + timers;
  // score/combo/meter/HUD (bounded UTF8), particles/trail arrays if JS-owned
};
```

Do not memcpy a C++ struct with compiler padding into a typed array. Specify slot names and offsets in one manifest; generate/static-assert native constants and JS accessors from it. ABI header should be integer storage (`Uint32Array`), not Float32 and not lossy double IDs beyond 2^53. Bridge initially uses owned copies and bounds: controls ranges [-1,1], grab mask0..15, mode/enum valid, all numeric outputs finite, quat length plausible, counts/byte lengths checked before multiplication. Suggested design limits: ≤64 events/frame, ≤700 particles/trail records, ≤4 trick lines and bounded UTF8 bytes. These are proposed bounds, not tested production contracts.

For pure rules, input includes spin/flip/air-time/switch flag, ordered grab/uber summaries and a copied last-ten recent-label list. Return final points+canonical label/uber count; native appends that accepted label exactly once and caps history at ten. This preserves repeat multiplication BEFORE the original final five-point rounding: do not round base points first and then apply decay. The wrapper must reset temporary G.recent from native input for every call, never retain it as a second authoritative history. Native owns repetition history and combo banking. Preserve exact Unicode label in gameplay state; renderer transliteration is presentation only. Reject malformed output before any scoring side effect.

### Frame, slowmo, pause and restart

The native clock (`runtime.cpp:48-56`) matches fixture frame lines1804-1820: clamp nonfinite/negative/raw input safely to 0..0.05, snapshot `slowmo>0 ? .4 : 1` **once** before ticks, decrement pre-existing slowmo by real raw delta only when not paused, add scaled raw to accumulator, fixed unscaled dt=1/120, maximum12 ticks then drop accumulator. Newly earned slowmo starts affecting physics on the next frame. JS must not scale dt again.

```text
actions (native, first)
raw = finite_clamp(frameDelta, 0, .05)
scale = provider.beginFrame(raw)      // one slowmo read; preexisting countdown
if !paused:
  acc += raw * scale
  while acc >= 1/120 and ticks < 12:
    provider.step(controls, 1/120)    // includes comboTick exactly once
    acc -= 1/120
  if ticks == 12: acc = 0
provider.realTimeFeedback(raw, scale, controls)
validate + commit snapshot and event batch
native stream/pose/camera/render both views; native publish audio
```

Pause stops physics/particle aging and freezes audio device clock through existing `AudioEngine::pause`; real-time visual smoothing, flash/inversion/shake and source UI timers continue as the native contract does. Do not “improve” pause by stopping all clocks. Title simulation/autopilot runs but audio starts only on drop-in; score/UI side effects remain gated. Mute changes native gain, not time.

Restart increments generation and resets native accumulator at the application action boundary. Preserve source restart asymmetries intentionally: original newRun retains some input edges, slowmo/flash/shake and recovery/UI transient state; `Game::reset` documents this. A fresh JS realm every new run would reset more than source behavior unless state is explicitly reinitialized. Native best score and music song clock survive run restart; reset particles/world caches/jobs and appropriate gameplay state, not DSP song state. Keep original held-grab/jump behavior in replay tests.

### Source-seeded randomness, not one global Math.random

Keep source terrain `rng(hashInt(cell multiplier + offset))` as uint32 Mulberry32 with the same draw order. Seeded procedural definitions must not consume the effects/music stream or change when the renderer requests another cell. Native candidate effects use `seed ^ 0xaba17835`, TitleAutopilot uses `seed ^ 0x416e79`, new-run selection uses `seed ^ 0x97f4a71b`, and renderer camera shake starts at `0x821371`; audio owns its separate initialized RNG/noise streams. These are explicit deterministic adaptations to browser effects that use Math.random, not a claim of identical browser random sequences. Current Game::restart does not reseed the existing effects RNG, and title/camera stream state also lives across a run boundary. Preserve those lifetimes unless the parent approves a changed replay policy.

A script simulation adapter should receive explicit stream state/services and consume effects draws only where source call sites do. Do not globally override Math.random across terrain, UI, camera and audio and then call this deterministic parity. Export/import owned stream states on reload; record initial seeds and draw-order-sensitive events in replay evidence. Native audio remains its stream authority; no JS audio randomness crosses into the callback. Pure scoring needs no randomness and is therefore the safest first authoring seam.

## 8. Terrain rows, cooperative jobs and generations

Current `TerrainJob::step(3.5)` samples and triangulates rows cooperatively, at least one row before budget check; renderer essential cell/cell+1 priming is explicitly synchronous. Budgets are cooperative, not hard caps. Current seven visible cells span current-1..current+5 and retirement extends current+7. `Renderer::update_player` destroys its old CPU job before using a new generation and deletes GL cells main-thread.

For optional JS terrain, expose one row or a small row batch, not `height(x,z)` for every one of thousands of mesh vertices:

```js
// Proposed signature; job identity is host-controlled, counts bounded.
terrainRows({abi:1,generation,cell,startRow,rowCount}, outputFloat64)
// each column: x,y,z,lip,stripe,relativeHeight,lateralU
// source UCOLS has 107 columns; ROW=.8, ROWS=138;
// sample halo rows -1 through ROWS+1; native finish uses adjacent rows.
```

Native topology needs normal neighbors and sample halos; do not truncate to only rendered rows. Returned buffers are copied into native job storage at the end of each call and carry generation/cell/row coverage. Job can yield only after a complete row; use interrupt deadline as a fault escape, not ordinary partial row cancellation. An interrupted provider may have advanced caches/state: invalidate that job/runtime rather than upload partially initialized output.

Restart flow: stop new calls → cancel/drop old jobs and leases → increment generation/seed → clear provider caches/cumulative-drop sequence → prime essentials → accept only new generation results. Cell cache eviction must not invalidate collision iteration: native World rails copies feature vectors and active rail copies rail definition. JS Map objects are rooted while copied; never expose native pointers to evictable map contents. No stale result can be uploaded just because the cell number is unchanged after seed restart.

Cumulative cliff drops can grow with highest explored cell despite bounded parameter/feature maps. Bound playable coordinate/cell range (current CLI4096) and test distant cell queries; a memory limit is not permission to silently change world values. Cooperative rows shouldn't add an uncontrolled recursive build of every uphill cell inside a nominal3.5ms job.

## 9. Typed-array ownership and numeric audio boundary

**Default: copy.** Engine-owned typed snapshot → native-owned fixed-capacity buffer → native validation → renderer/DSP commands. Retain root while accessing; validate detached/resizable state, exact type, offset, count and overflow-safe byte ranges. No JS evaluation/property callbacks while borrowing; release JS root only after copy. GL may consume native memory in bufferData on the main thread, never borrowed VM storage owned by a worker or reset runtime.

**Optional zero-copy lease:** native allocates backing storage with stable lifetime and explicit generation. JS view cannot retain an ephemeral stack vector or reallocating std::vector buffer. The external-buffer callback owns release and must never call JS, SDL video or GL. Stop all users before runtime teardown; detach/invalidate views on revoke; no resizing/transfer/shared buffers in initial ABI. Pool storage is not reused until both producer/consumer ownership and VM finalization obligations are satisfied. A raw pointer plus a promise that JS “probably won't retain it” is not a lease. ng and Bellard callback differences must be handled at the pinned API, and external allocations need separate accounting.

The probe proves a basic GC-rooted lease release, not revocation across hot reload, detached buffers or worker races. Prove those before enabling zero-copy in production. For small snapshots, the engineering risk is unlikely worth saving a few kilobytes of copy.

Audio: main-thread script emits bounded numeric `{kind,amount,flags}` and RideSound fields; translate into existing `GameEvent`/`RideSound`, then reuse `publish_feedback` and `AudioEngine::trigger/set_ride`. Only eight audio-relevant event kinds currently become native numeric commands; HUD labels and score event strings stay main-thread. Audio queue capacity256 and voice capacity128 already bound callback work. Validate enum/finite ranges; maintain ordered On/Off events, especially GROTESK and Grind. Do not coalesce distinct transitions into a single snapshot and lose latches.

Never run JS, GC, bindings, module loading, JSON parsing, logs or exceptions in the real-time audio callback. Never let a lease finalizer free memory still used by DSP. On script fault: pause native audio first (joins callback), silence ride/clear pending staged script events, then display fault overlay; resume requires explicit restart/rollback. Command overflow must be observable (existing dropped_commands) and critical Off transitions need a reviewed safe recovery path rather than silent drop.

## 10. Errors, deadlines, security and hot reload

### Exception/fail policy (proposed)

At initialization, verify required exports and ABI in a staging context before it can publish state. Before each JS call install an elapsed-time deadline and allocation budget. Every Eval/Call/conversion checks JS_EXCEPTION or engine-specific equivalent. Convert message/stack to bounded native text; retain original source filenames/line numbers. Exception formatting can itself throw or OOM: use a fixed native fallback message. In product design, show a native error overlay with module/hash/generation/tick/category; never depend on JS/DOM to render its own failure.

For a pure rules module, reject malformed result and fail visibly; optionally disable rules mode only at an explicit restart boundary. Do not silently switch score formula mid-air. For JS-owned simulation, a partial tick is not rollback-safe: freeze last committed snapshot, stop tick publication, pause/silence audio, cancel pending generation jobs. Native fallback requires an explicitly exported validated complete checkpoint or a new run, not copying public Game fields and hoping private state matches.

Suggested initial limits32MiB runtime,512KiB stack and separately bounded native buffers are experiment configuration, not approved device budgets. Per-tick deadline needs measured allowance within the frame, not the20ms probe used to demonstrate interruption. An interrupt callback checks steady_clock without allocation/logging. It cannot preempt a blocked native call. Module loader and native functions must be bounded and synchronous; filesystem parsing happens outside frame-critical calls. Promises/jobs, if introduced, get an explicit bounded drain at a named frame boundary; no unbounded ExecutePendingJob loop.

### Capabilities and bytecode

Use core engine only, not quickjs-libc std/os, filesystem/process/network bindings or shared-array threading. Whitelist versioned embedded modules and normalized relative paths; reject traversal, absolute paths and `.so` native module loading. No arbitrary browser URLs, beacon or npm dependency fetching. Engine isolation is not an OS sandbox for hostile downloaded code: memory corruption in engine/native bridge can compromise process. Trusted developer scripts first; hostile mods require a separate process/capability/security design. Cache labels never become file paths or shell commands.

QuickJS `qjsc` packages bytecode into C and can remove source parsing if eval is excluded; **that is not native-code AOT acceleration**. Bytecode is version-specific and not checked as a safe untrusted input (02 §3.4.5). Pin engine/host compiler/endian/format expectations; bytecode build tools must run on host, not cross-built qjsc. Start with trusted source text for inspection/hot reload. Test bytecode generation separately if needed; no bytecode experiment occurred here. Duktape dump/load and V8/JSC caches/snapshots likewise aren't stable portable game-save formats. Don't persist engine heap pointers or cached compiled handles across runtime replacement.

### Atomic reload with migration

Reload is a development feature, requested on main-thread at a safe frame boundary while paused. Create a fresh staging runtime/context with identical capability limits; parse source, resolve whitelist modules, check exports, run smoke and deterministic state import tests. Export **plain versioned data**, never closures/Map objects/JSValue handles. Include seed, RNG streams, generation/tick, R/G state, input previous edges, accumulator/slowmo, active rail copy, combo/recent order, air grabs/ubers and meaningful timers. Native terrain generation and native audio song clock remain their respective authorities.

For initial pure rules module, no migration should be necessary: module is stateless and native owns history. Full simulation migration is substantially harder. Require explicit fromVersion→toVersion conversion; reject unknown versions, malformed/nonfinite/out-of-range data. Stage outputs/events without publication. Swap only after successful import and replay sanity, invalidate old jobs/leases, drop old handles before destroying old context. Failed staging reload retains original live runtime/snapshot. Do not fall through with half old/half new state; record hashes and error in native overlay. Prefer new-run-only reload until full migration tests are implemented.

## 11. Phased, intern-ready implementation plan (approval gates)

These are future implementation tasks, not this ticket's edits. Parent approves any production phase.

1. **Read and reproduce (research acceptance).** Read runtime/Game clocks, terrain/world ownership, feedback/audio queue, Renderer::update_player, pose/particles and fixture source. Run pin/probe commands and existing ctest; inspect exact source oracle JSON and failed wrapper diagnostics. Gate: explain why HTML Eval fails and why both simulation owners can't run live.
2. **Optional stateless rules spike.** Proposed `src/script_runtime.hpp/cpp`, `assets/scripts/rules.js`, `tests/test_script_rules.cpp`; separate CMake option defaultOFF. Pin one core engine and licenses deliberately; no qjs-libc. Extract scoring/trick pure bodies through explicit adapters, keep repeat history native. Add RAII value owner, bounded result schema/errors and ABI checks. Gate: all native rule fixtures/source oracles pass; mode disabled builds unchanged. Do not rewrite existing Game physics or shaders.
3. **Lifecycle/fault harness before renderer use.** Proposed `tests/test_script_lifecycle.cpp`; OOM at every conversion path, deadline, invalid return, detached/resizable buffers, retained callbacks, context reset/teardown, module denial, pending-job bounds. Add native fault overlay proposal under renderer HUD separately. Gate: no leak/UB, no side effects from failed output, deterministic restart behavior, native audio callback untouched. Run ASan/UBSan host build and CPU suite; sanitizer not run in this research.
4. **Shadow replay comparison.** Proposed `scripts/.../generate-script-replay.cjs`, `tests/test_script_replay.cpp`; identical seed and recorded controls/raw frame deltas. One provider publishes effects; comparison provider never scores/audio/persists. Cover ground/air/rails/trees/bails, Unicode/repetition, held jump/grabs, pause/slowmo boundaries, title/drop-in/new seed. Gate: explicit tolerated numeric drift and equal discrete decisions over long runs; don't suppress mismatch logging to pass.
5. **Only if approved, JS simulation authority.** Introduce immutable `RenderSnapshot` seam in `game_renderer.cpp`/pose rather than mutating a shadow Game. Keep native terrain/world authoritative and expose bounded queries/definition batches. Retire native live physics calls in this mode; retain as oracle/native mode. Stage events and snapshots; native audio/render/meshes remain owners. Gate: end-to-end CPU contracts, profiling and failure-policy review before any hardware deployment.
6. **Terrain alternate experiment, not mandatory migration.** Batch107-column rows and halo coverage into native TerrainJob. Tag generation/cell/row range; bound finish/upload jobs and external memory. Gate: source mesh normals/indices/paint parity, seed restart while job pending, no stale upload, measurable target benefit. If no benefit, keep native provider.
7. **Reload and packaging.** New-run-only reload first, then optional explicit migration. Add schema-version roundtrip/reject tests; source hashes/module manifests and license notices. Compare trusted source versus matching qjsc artifacts only after establishing reproducible host-tool generation. Gate: failed reload leaves original running state; unknown bytecode never accepted.
8. **Parent-owned ARM/device acceptance.** Cross-link is already demonstrated but not execution. Parent tests target loader/libstdc++ compatibility and headroom, then runs relevant dual-screen/audio/graphics gate. Instrument script/tick/GC/copy/terrain chunks with Profiler and record p50/p95/p99/max frame times, RSS growth, GC allocations, event queue high water/drops, tick debt and thermal behavior. Script mode must not regress current candidate acceptance. No worker in this ticket performs this phase.

Suggested acceptance budgets should be derived from native baseline and device headroom, not host multiplication: physics is120Hz but render need not be120FPS; both views share main thread, swaps and DSP run concurrently. Reserve explicit time for GPU submission, stream jobs and audio command publication. Reject a runtime that passes average timing but exhibits deadline/GC-tail stalls during ten-minute replay or repeated restart/reload cycles.

## 12. Validation matrix and unresolved decisions

| Test family | Required assertions | This ticket |
|---|---|---|
| Syntax/pure values | Original extracted functions, required language/built-ins, seed hashing | Measured two interpreters + Duktape subset;85 leaves |
| Full simulation replay | 120Hz input edges/modes/combo/RNG, pause/slowmo boundaries | Native existing tests only; JS replay future |
| Bridge | Exact Float64 type, lengths/offsets/detach/resize/finite/overflow/fuzz | Basic copy/offset/width/NaN/size/lease measured; hardening future |
| Lifecycle | root retention, destructor order, failed reload, stale generation, interrupted partial tick | Basic lease + interrupt/OOM measured; complete suite future |
| Modules/async | traversal/native module denial, relative imports, pending-job budget | Design only; not experimentally qualified |
| Audio | no VM in callback, ordered latches, silence/fault/pause, queue overflow | Native CPU lifecycle pass; script path not built |
| Profiling | frame and per-call tails, automatic GC causality/RSS, native-vs-script A/B | Host microbench/explicit GC; on-target unqualified |
| Graphics | identical shaders, objects/trails/two views, no stale upload | Source inspection only; parent gate required |
| Packaging/license | pinned source/compiler/headers/license, no binaries/secrets committed | Three source pins verified; bounded evidence tracked |

Remaining parent decisions: is JS authoring/hot reload valuable enough to justify a new runtime, or is source-derived oracle generation sufficient? Should first phase remain pure rules only? What target headroom, script trust model and fatal-script UX are acceptable? Full browser fidelity (notes/touch/X-ray labels/fonts) is a separate backlog, not a reason to quietly replace native graphics. These questions do not block delivery of this research; they block future production scope approval.

## 13. Onboarding checklist and references

- Confirm `git diff 24b50d8 -- src CMakeLists.txt assets fixtures tests` is empty for research commits.
- Read `src/runtime.cpp:48-62`, `game.hpp` clock comments, `game.cpp` reset/place/land/score, `game_renderer.cpp:42-58` generation and streaming.
- Read `terrain.hpp` Vertex/TerrainJob, `world.hpp` owned active-rail constraint, `feedback.hpp` numeric audio contract, `audio.cpp:378-414` lifecycle and transfer.
- Run README commands; expected PASS is distinct from expected wrapper compilation failure.
- Read `sources/02-bellard-manual.md` §3.4, ng API capture05, Duktape20-22, JSC11-13/23, V809/10/24, quickjspp14 and release captures01/03/16-18.
- Never treat a source capture of WebKit main or V8 candidate as a release pin. Exact immutable source refs inspected: WebKit9f4a36c5c53a4fa67421fa25fdf5194572d29795 (private limits); V8cce823212e9a36b5e22c72bb7edc4e017045bd5e (isolate/license). Version/header captures on main have receipt hashes but aren't guaranteed same commit unless re-fetched pinned.
- Use `reference/01-investigation-diary.md` for chronological failures and reasoning; use experiment `evidence/` for outputs, not prose as performance proof.

The recommendation is deliberately narrower than “port HTML by Eval”: preserve the native game's progress, learn from authored JavaScript with reproducible source oracles, and introduce an optional constrained rules module only when its development benefit has been approved and its lifecycle/performance has been qualified.
