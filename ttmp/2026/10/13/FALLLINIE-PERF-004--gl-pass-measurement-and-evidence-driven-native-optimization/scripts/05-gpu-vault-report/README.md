---
Title: GPU counter and shader report publication helpers
Ticket: FALLLINIE-PERF-004
Status: complete
Topics: [performance]
DocType: reference
Intent: long-term
Summary: Scoped append-only vault publication, using existing measurements only.
---

# GPU counter and shader report publication

`prepare.py` copies the four actual figures, two fixture images and evidence JSONs into the new dated vault asset directory. It refuses an existing directory rather than overwriting historical assets. It also extracts one actual central counter interval for the report's worked example. The narrative itself was authored and reviewed directly in the new vault note.

`validate.py` checks the report's local links, SVG/XML, JSON, PNG integrity, asset hashes, exact mean values, frame counts and explicit interpretation boundaries. It runs no game, compiler or device measurement.

The publication comprises exactly13new vault files: one4606-prose-word note and12assets. `validation.json` records filenames and the note hash. `vault-commit.txt`, `vault-push.txt` and `remote-verification.txt` preserve the successful scoped commit/push and independent remote-head check. Existing notes/assets were not edited.

Vault commit: `d769550d9730853d3cb221bec8bdbee777139487`.

Measured native checkpoint: `70c164a18ee10a6c321c767ea00e64e3faa8dc7e`; game binary runtime remains209a6d0. Native changes were not pushed. The combined shader stays diagnostic-only because the ABBA experiment did not show a convincing benefit.

The user requested publication followed by stopping for the night. No new experiment or optimization follows this publication.
