---
Title: GPU006 benchmark tooling workflow
Ticket: FALLLINIE-GPU-006
Status: active
Topics: [performance]
DocType: reference
Intent: long-term
Summary: Rebuild, deploy and analyze the repeatable GPU scaling campaign.
---

# GPU006 workflow

Implementation commit: `32b8bfd`. Production defaults and launcher are unchanged.

1. `bash scripts/01-build.sh` (paths here are relative to the ticket).
2. `python3 scripts/05-qualify-build.py <implementation-commit>` verifies the pre-commit source manifest against that revision; it is not target qualification.
3. Use the installed Ed25519 SSH key. The runner uses batch mode, strict known-host checking and `~/.ssh/id_ed25519` (override with `SSH_IDENTITY_FILE`). No password or askpass environment is required.
4. `bash scripts/03-campaign.sh` deploys a separate candidate, checks executable identity and runs eight bounded cases. Expected elapsed time is approximately six minutes plus deployment/copy time. Each case has600frames and a45second countercapture; one case per nonreference configuration is exploratory evidence, not a statistical performance guarantee.
5. `python3 scripts/04-analyze.py scripts/evidence/target-<stamp>` requires a complete matrix, matching tracked trajectories and matching endpoint images within each resolution. It writes summary/provenance JSON in that campaign directory. It does not silently summarize incomplete evidence.

The first device attempt, `target-20261014T174658Z`, stopped at SSH authentication before deployment and remains preserved. The subsequent key-authenticated campaign, **target-20261014T180649Z**, completed all eight cases. The analyzer ran successfully on actual device data: matching selected-state signatures, same-resolution endpoint images, valid GL/probe status and complete paired records. See analysis/01 in the ticket for results and qualifications. `06-results-figure.py` regenerates the figure for that pinned campaign.

Host checks passed for120frames at256×192: single-view uncapped versus dual-view60FPS policy have identical selected-state signatures and finalprint bytes. Their wall timings are not target GPU performance evidence. The feature-boundary build ran15CPUtests, including synchronous versus fixed1/32workunit terrain byte comparisons.

Raw BMP endpoint captures are retained for equality checking. The signature covers selected game/camera/residency fields, not every mesh/particle. All counters are whole-device; rates use contained adjacent host-poll windows, not an exact backend timestamp join. No governor, affinity, driver or productionlauncher changes.
