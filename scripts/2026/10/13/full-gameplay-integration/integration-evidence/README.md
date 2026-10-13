# Integrated source candidate — host evidence only

Base: `d911ee4`. Authority: unchanged `fixtures/falllinie.html`, SHA256 `cb9b4befd0252859d9c45246f94fbd7dd1662c6d034c6ccb95fbe16947e8b556`. Parent camera-relative fix `7eac73f` is retained for both views. No device deployment, graphics run, physical sound or human playtest occurred. The handheld still runs the fixed **old prototype**.

## Reproduce from repository root

```sh
bash scripts/2026/10/13/full-gameplay-integration/check-gameplay-host.sh
scripts/2026/10/13/full-gameplay-integration/check-audio.sh
AUDIO_SANITIZE=1 scripts/2026/10/13/full-gameplay-integration/check-audio.sh
cmake -S . -B build/integrated-source
cmake --build build/integrated-source -j4
ctest --test-dir build/integrated-source --output-on-failure
build/integrated-source/test-terrain
mkdir -p build/jitter
c++ -std=c++17 -O2 -ffp-contract=off -Isrc \
  scripts/2026/10/13/render-jitter-investigation/precision-probe.cpp \
  -o build/jitter/precision-probe
build/jitter/precision-probe
```

CTest audio-lifecycle explicitly sets `SDL_AUDIODRIVER=dummy`: open/idempotent open, paused sample clock, mute/resume, close/callback join, reopen, destructor and subsystem ownership. It refuses any other driver and asserts no SDL video initialized. This is not backend/physical qualification.

Optional current DSP timing/evidence (no device; WAV remains outside repository):

```sh
scripts/2026/10/13/full-gameplay-integration/check-audio.sh \
  --render /tmp/falllinie-integrated-audio.wav 120
```

## Recorded results

- Gameplay/world/pose source oracles and explicit integrated executable link pass. CTest: 6/6 tests pass including terrain and production input/frame/audio seam.
- Original terrain: 268 cases, hash/RNG/mesh/topology/matrix/incremental identity. CPU sky ray error `2.96038603e-05` relative vs `0.0322687073` absolute; origin boundary jumps `3.2737144e-05` world-up, `3.84849886e-05` rolled-up (both <1e-4). This emulates GLSL arithmetic, not Mali scanout.
- Source final saturation now runs nonlinear processing at **2x**, with preallocated symmetric 49-tap Blackman half-band interpolation/anti-alias decimation. Exact original steady DC/odd curve, measured 24-base-sample impulse peak, reset identity, block independence and zero watched render allocations pass. One selected folded third-harmonic probe: **−103.314 dB vs 1x**, not a universal alias/audio-parity claim.
- Strict audio CPU and ASan/UBSan tests pass. Original 20 sections/2434 steps, complete 2.6s IR, all source reactions, queue/voice limits, rate/block independence remain checked. GROTESK false snapshots and repeated On event do not clear/duplicate its latched landing reaction.
- Current optional 120s/48kHz render: all 16 voice families, peak voices42, queue high water2, zero normal command/note drops, steals or nonfinite samples; peak .748301, RMS .132354. Host 512-frame cost mean .978062ms, p95 1.62701ms, max 3.42308ms, interval10.6667ms. This is x86_64 host cost, **not target headroom**. No one listened to this render.
- Historical source-audio logs/WAV hash in `../audio-evidence/` precede integrated 2x DSP and are labelled historical.

## Integration contracts / intentional native adaptations

- `NativeInputMapper::read` resolves SELECT+START before start/pause/new-run. Raw measured pad A0, B1/X2 boost, grab bits1/2/4/8 on4/5/6/7, dpad13–16; START9 pause, SELECT8 reproducible new seed. Source keyboard WASD/arrows, SPACE, SHIFT/B, J/K/U/I, P/ESC, M, C; native additions R new-run, F3 diagnostic, F10 quit. No X/Y debug actions. Six palettes.
- `NativeFrameClock::advance`: finite dt clamp[0,.05], begin_frame once, accumulator dt*pre-tick scale, max12 ticks at120Hz/discard overflow, real_time_feedback once. Main renderer update once, then `publish_feedback` publishes RideSound and consumes/clears events once; views do not simulate or consume events. No extra combo_tick.
- Title uses source autopilot controls/probabilities/hop/ballistic timing with separate seeded RNG. Audio opens lazily at start. `--autoplay` starts immediately and uses the same source demo policy as a native profiling extension.
- `Game::start` and `restart(seed)` invalidate generation (also for same seed), reset terrain/object definitions and renderer jobs/GPU maps. Main never uses legacy `reset`. Native new-seed sequence is reproducible from initial --seed; each full chosen seed is logged and can be replayed directly.
- Source newRun retains jump/grab edges, slowmo/flash/shake/HUD and recovery timers; place retains charge/crouch/grab/boost transients. Title suppresses source UI reactions. Camera-relative projection includes source eye shake; world physics/shading remain absolute. Print noise uses real frame time; trail age uses simulation time.
- One engine, one main-thread command producer. Pause freezes DSP/song, mute does not; slowmo/restart do not reopen or reset song. GROTESK is exclusively event-latched; snapshot flag is diagnostic. New-run deliberately clears lingering Music GROTESK/grind state (browser newRun omits those calls). Resume publishes unpaused state before enabling callback.
- Best-score file is SDL_GetPrefPath("Falllinie","Falllinie")/best-score, with Game's atomic rename save. Save failures log and appear on HUD. No arbitrary cwd/repository save path; not an fsync/power-loss guarantee.
- Native bounded title/pause/HUD/control typography, hotplug, application-wide focus loss, quit chord and new-run keys are adaptations, not DOM identity. DOM notes/touch markup and browser mathematical X-ray overlay remain unported (scene X-ray is present). Lower camera is live head anchored but retains board-nose/up extension rather than sideways source gaze.
- DSP remains native rather than WebAudio waveform-identical: oscillator band-limiting/phase, seeded randomness, biquad/automation implementation, compressor detector/lookahead and analytic curve/FIR implementation differ. Half-band adds .5ms at48kHz; reverb retains wet-only1024/rate latency/bursts. Overload-only output guard bounds FIR ringing to ±.85. Bounded voice/queue/pump resources can overflow; diagnostics expose it.

## Remaining acceptance gates

Independent review is required. Parent owns combined target workload/profile, fresh graphics evidence, physical controller comfort/UI legibility, real audio negotiation/underruns/listening and the final human playtest. Synchronous bounded restart/object/upload costs and cumulative terrain history remain target performance risks. Existing float world attributes/shading retain extreme-distance limitations beyond the relative projection repair. No complete browser parity, waveform identity or physical acceptance is claimed.

Final self-review also suppresses unchanged **effective quantized** ride commands on the sole producer (same source gain-setter policy). Main still publishes once/frame, but ten seconds/600 paused frames cannot fill the suspended callback queue; dropped sends do not poison the cache and are retried. Final CPU/sanitizer and all6 CTest gates were rerun after this correction. The optional offline-render timing/WAV above was measured at DSP milestone `cd514ea`; later transport deduplication is not a new target profile. Its external WAV SHA256 is `cad341086f51a07c0c80a158cf96569a2078b490637775a5c2b299401d7d8838`.

The full binary-capable patch is intentionally ignored/uncommitted at `../integrated-candidate.patch`, generated with `git diff --binary --full-index d911ee4 HEAD`. Final exact revision, absolute patch path/hash and independent-review status are in the authoritative external candidate handoff, not embedded in the patch itself.
