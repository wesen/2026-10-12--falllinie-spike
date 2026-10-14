# GPU006 workflow

Implementation commit: `32b8bfd`. Production defaults and launcher are unchanged.

1. `bash scripts/01-build.sh` (paths here are relative to the ticket).
2. `python3 scripts/05-qualify-build.py <implementation-commit>` verifies the pre-commit source manifest against that revision; it is not target qualification.
3. Restore device credentials **outside chat and repository files**. The existing external `/tmp/askpass.sh` helper requires `ROCKNIX_PW` in its environment. Export `SSH_ASKPASS=/tmp/askpass.sh` only after the credential has been restored.
4. `bash scripts/03-campaign.sh` deploys a separate candidate, checks executable identity and runs eight bounded cases. Expected elapsed time is approximately six minutes plus deployment/copy time. Each case has600frames and a45second countercapture; one case per nonreference configuration is exploratory evidence, not a statistical performance guarantee.
5. `python3 scripts/04-analyze.py scripts/evidence/target-<stamp>` requires a complete matrix, matching tracked trajectories and matching endpoint images within each resolution. It writes summary/provenance JSON in that campaign directory. It does not silently summarize incomplete evidence.

The first device attempt, `target-20261014T174658Z`, stopped at SSH authentication before deployment. No actual target results exist for GPU006 yet. The analyzer has syntax validation only, not an end-to-end run on GPU006 data. Do not turn this into a measured scaling claim.

Host checks passed for120frames at256×192: single-view uncapped versus dual-view60FPS policy have identical selected-state signatures and finalprint bytes. Their wall timings are not target GPU performance evidence. The feature-boundary build ran15CPUtests, including synchronous versus fixed1/32workunit terrain byte comparisons.

Raw BMP endpoint captures are retained for equality checking. The signature covers selected game/camera/residency fields, not every mesh/particle. All counters are whole-device; rates use contained adjacent host-poll windows, not an exact backend timestamp join. No governor, affinity, driver or productionlauncher changes.
