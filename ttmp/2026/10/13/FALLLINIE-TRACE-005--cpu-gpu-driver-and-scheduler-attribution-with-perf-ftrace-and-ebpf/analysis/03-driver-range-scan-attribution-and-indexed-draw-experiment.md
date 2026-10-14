---
Title: Driver range-scan attribution and indexed draw experiment
Ticket: FALLLINIE-TRACE-005
Status: complete
Topics: [performance]
DocType: analysis
Intent: long-term
Summary: Event profiles and verified disassembly motivate an opt-in range-draw candidate; scene CPU improves but frame performance does not clearly improve.
---

# From CPU events to an indexed-draw experiment

## Attribution campaign

Five unchanged323b095runtime profiles at199Hzrequested frequency collected9895samples with zero reported loss. All use the mainTID/no-inherit, audio-disabled32sworkload,20scollection and fixed384×288dualview configuration. Raw evidence is scripts/11-cpu-attribution/evidence/target-20261014T152820Z.

| Event | Samples | Driver event weight | Hot PC0x42d6dc |
|---|---:|---:|---:|
| cycles opening | 2014 | 72.09% | 16.84% |
| L1D refill | 2040 | 58.52% | not displayed at0.5%cutoff |
| retired branch misprediction | 2008 | 70.51% | not displayed at0.5%cutoff |
| L1D TLB refill | 1904 | 72.10% | not displayed at0.5%cutoff |
| cycles closing | 1929 | 71.20% | 15.56% |

Different event weights are not comparable time fractions. Absence from a thresholded report is not zero activity. These records do not establish a precise cache-miss instruction or latency.

## Verified address and algorithm

The actual process executable map begins0x7f95d40000 with fileoffset0; the executable ELFLOAD also has offset/VA0. RuntimePC0x7f9616d6dc therefore maps to0x42d6dc. The partial frame-pointer caller0x7f961503ef maps to0x4103ef, the return-address-minus-one associated with BL at0x4103ec targeting0x42d6c4. Preserved mappings, loadsegments and disassembly establish this immediate call relation.

The helper at0x42d6c4 initializes vector minima/maxima, reads16unsigned32-bit values viaLD4, advances64bytes, reduces withUMIN/UMAX, handles a four-element remainder and merges results into caller-provided min/max outputs. The sampledADD follows the vectorload; it is not evidence that pointer arithmetic is intrinsically costly. The nearest exported symbol glViewport+offset is not a private-function name.

The renderer uses32-bit indexed draws. An index-range scan is therefore a concrete hypothesis. The partial callgraph does not independently prove every intervening GL layer; a controlled API change supplies stronger evidence.

## Candidate contract

Implementation487492c adds opt-in `--draw-ranges`. The same binary executes ordinary glDrawElements when off and glDrawRangeElements when on. For nonempty draws, supply start0/end(vertex_count-1) from the bound vertexbuffer. Cell/object/dynamicmesh bounds are local; ridergroups use the full riderbuffer domain. These are legal broad bounds, not necessarily minimal subgroup bounds. Empty draws preserve original API behavior.

Indexbytes/type/offset/order, shaders, draw grouping, simulation and audio-disabled policy remain unchanged. GLES3 typed entry loading follows the existingloader. Production launcher/default remain unchanged; no driver/governor/kernel modification.

The candidate binarySHA256 is b2d88ab072e888be4315173ab75af4a17325f2c03581c9d00e625f5d6113837c. Captured src hashes were verified against487492c after the precommit build;15CPUtests and both builds passed.

## Same-device qualification and ABBA result

Seven-cell seed42staticcaptures at384×288 produced byte-identical encodedscene,depth andfinalprint buffers. This qualifies that fixture only, not every live scene.

Four32scases with20scyclesprofiles used the same binary in off/on/on/off order. All completed withGL_error0, audio_disabled=true, expecteddraw_ranges receipt, completepairedcounts and zero reportedlostsamples. Raw data are scripts/13-index-ranges/evidence/target-20261014T154415Z.

| Case | Frame mean ms | Frame p95 ms | Top scene CPU ms | Top view CPU ms | Driver cycle weight |
|---|---:|---:|---:|---:|---:|
| baseline01 |18.264040|28.070701|2.909902|4.531863|72.23%|
| ranges02 |18.207474|28.260837|1.419323|3.207138|63.78%|
| ranges03 |18.241656|28.422115|1.592762|3.391486|65.00%|
| baseline04 |17.882124|27.579522|2.439544|4.094052|73.14%|

Paired scope means use20selectedscopes/view within each outercollectionenvelope. Whole-run frame statistics useframe>=60 and are a different scope.

Unfiltered symbol reports contain six/seven sampled offsets in helper range[0x42d6c4,0x42d758) for baseline01/04, totaling17.12%/16.76%roundedcycleweight. **Neither candidate recording contains a sampled offset in that range.** This is zero observed samples, not an exhaustive callcount proof. Combined with unchanged indexdata and the isolated API switch, it strongly supports the range-scan hypothesis.

Top sceneCPU decreases from2.44–2.91ms to1.42–1.59ms. Across equally sized sampled groups, summedtop/lower viewCPU averages approximately7.012ms baseline versus5.889ms candidate, about16%less. These are nonoverlapping sequential viewscopes; nestedscene/output scopes are not added again.

The pooled frame means are18.071014ms baseline and18.224560ms candidate. This is **not a demonstrated frame-time improvement**, nor sufficient evidence of a causal0.85%regression under drifting live conditions. Baseline04itself differs frombaseline01. Topoutputwall increases from3.19–3.97ms to3.95–5.06ms in the recorded cases; wall time can move into a later submission/output scope when earlierCPUwork decreases. That observation is not a proven GPUwait decomposition.

PeakRSSreported by the game is113732/122804/119788/112340KiB in runorder. The candidate's higher peaks are another observation to preserve, not yet a proven cost mechanism. Broad rider subgroup bounds may be worth tightening later, but this experiment does not establish that they caused any GPU/memory change.

## Decision

Retain --draw-ranges as an explicit diagnostic candidate, not the default or a production deployment. There is supported CPU-side progress and bounded image equality, but no end-to-end frame benefit established and no audio-enabled acceptance. This differs from the earlier shader null result: the present experiment changes a measured CPU hotspot and scope cost while leaving frame tails unresolved.

Next justified questions are tighter subgroup bounds, GPU/output dependency attribution and eventual audio-enabled comparison. Do not substitute uniform-cache guesses or claim cache latency from these event-location profiles. The living vault report is updated separately with the same scope distinctions.
