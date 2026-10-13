# Independent review checkpoint

- `dsp-a0d5326.patch`: exact f1402c7→a0d5326 DSP/test scope.
- `overlay-eb6c1a5.patch`: exact a0d5326→eb6c1a5 overlay/CMake/tests scope.
- `overlay-dsp.md`: accepted frozen eb6c1a5 for further qualification, not release. Parent's later explicit buffer option is outside that review scope; it has separate dummy/backend qualification evidence.
- `js-research.md`: original review blocked final research acceptance pending status/provenance corrections. Parent corrected all three findings in research720afc7 (main ee81963); `../validate-research-acceptance.cjs` checks the original successful capture identity, preserved failed retry, case-correct URL, historical status annotation and navigable links. Ticket doctor passes. This is research-package acceptance only, never approval to integrate JS.

The preserved reviewer records are not rewritten to erase their original verdicts. Device audio deadlines and60FPS remain unresolved; neither review nor screenshots certify audible or physical human acceptance.
