---
Title: Historical GPU006 authentication failure before deployment
Ticket: FALLLINIE-GPU-006
Status: active
Topics: [performance]
DocType: reference
Intent: long-term
Summary: Preserved failed preflight; superseded by the later key-authenticated campaign.
---

# Device campaign not started

Attempt: 2026-10-14T17:46:58Z.

The initial SSH preflight failed with exit255 before deployment or a game/counter workload:

```text
Permission denied, please try again.
Permission denied, please try again.
Received disconnect from 192.168.0.202 port 22:2: Too many authentication failures
Disconnected from 192.168.0.202 port 22
```

The external helper references `ROCKNIX_PW`; a presence-only check found that variable unset. No credential value was read or retained. Restore the credential externally and launch a new campaign, preserving this failed attempt. Full stderr is in ../campaign-launch.txt. This directory contains no timing or GPU results.
