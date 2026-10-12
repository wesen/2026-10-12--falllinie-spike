# Falllinie graphics spike

- Keep shell/Node/Python scripts under `scripts/`.
- The authoritative docmgr ticket and detailed diary are in the sibling claw-stuff repository: FALLLINIE-GLES-001.
- Preserve original shader bodies; adaptations must be explicit. Do not claim complete game parity from this spike.
- All GL/SDL video calls belong to the main thread. Compositor worker performs IPC only.
- Commit source, scripts and bounded evidence intentionally; never build outputs, target shared libraries or secrets.
- Run CPU tests and the relevant graphics/hardware gate at feature boundaries.
