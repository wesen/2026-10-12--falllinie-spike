#!/usr/bin/env python3
"""Existing host Xvfb/Mesa functional gate only, never Mali performance evidence."""
import csv
import json
import os
from pathlib import Path
import subprocess
import sys
from analyze import analyze

S = Path(__file__).resolve().parent
ROOT = S.parents[5]
BIN = Path(sys.argv[1]).resolve() if len(sys.argv)>1 else S/'.work/build/falllinie-spike'
OUT = S/'.work/host-smoke'
OUT.mkdir(parents=True, exist_ok=True)
env = dict(os.environ, LIBGL_ALWAYS_SOFTWARE='1', SDL_AUDIODRIVER='dummy',
           XDG_DATA_HOME=str(OUT/'data'), TMPDIR=str(OUT))
cases = {
    'off-single': ['--freeze-scene', '--gpu-profile-mode', 'off'],
    'off-dual': ['--freeze-scene', '--dual-screen', '--gpu-profile-mode', 'off'],
    'passes-dual': ['--freeze-scene', '--dual-screen', '--gpu-profile-mode', 'passes'],
    'view-dual': ['--freeze-scene', '--dual-screen', '--gpu-profile-mode', 'view'],
    'bypass-dual': ['--freeze-scene', '--dual-screen', '--post-bypass', '--gpu-profile-mode', 'passes'],
    'live-dual': ['--autoplay', '--dual-screen', '--muted', '--dev-overlay', '--gpu-profile-mode', 'passes'],
}
results = []
for name, flags in cases.items():
    folder = OUT/name
    folder.mkdir(exist_ok=True)
    cmd = ['xvfb-run', '-a', str(BIN), '--assets', str(ROOT/'assets/shaders'),
           '--frames', '8', '--no-vsync', '--render-size', '384x288',
           '--profile-cadence', '1', '--profile', str(folder/'frames.csv'),
           '--receipt', str(folder/'receipt.json'), *flags]
    if name != 'live-dual':
        cmd += ['--capture-dir', str(folder/'capture')]
    (folder/'command.json').write_text(json.dumps(cmd))
    with (folder/'stdout.log').open('w') as log:
        run = subprocess.run(cmd, cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=90)
    text = (folder/'stdout.log').read_text()
    assert run.returncode == 0 and 'GL_error=0' in text, (name, text)
    receipt = json.loads((folder/'receipt.json').read_text())
    assert receipt['measurement']['fixture'] == ('live-or-default' if name == 'live-dual' else 'static-terrain')
    summary = analyze(folder/'frames.csv', 0)
    (folder/'analysis.json').write_text(json.dumps(summary, indent=2))
    scopes = summary['gpu_scopes_no_cross_mode_sum']
    if name == 'bypass-dual':
        assert not any('/post' in key for key in scopes)
        assert not any('/post ' in key for key in summary['cpu_scopes_nested_not_additive'])
    if name == 'live-dual':
        assert '0/dynamic_prep (parent=scene)' in summary['cpu_scopes_nested_not_additive']
        assert '1/dynamic_upload (parent=scene)' in summary['cpu_scopes_nested_not_additive']
    results.append({'case': name, 'result': 'passed', 'renderer': next(x for x in text.splitlines() if x.startswith('GL_RENDERER=')), 'stats': summary['whole_run_query_stats']})
# POST-RUN UNINSTRUMENTED FIXTURE REPEATABILITY only (same top view).
# main detaches measurement, then renders a fresh frame for capture. These
# assertions do NOT establish query-active image equivalence; that gate is deferred.
assert (OUT/'off-single/capture/print.bmp').read_bytes() == (OUT/'passes-dual/capture/print.bmp').read_bytes()
assert (OUT/'off-single/capture/print.bmp').read_bytes() == (OUT/'view-dual/capture/print.bmp').read_bytes()
assert (OUT/'off-single/capture/print.bmp').read_bytes() == (OUT/'off-dual/capture/print.bmp').read_bytes()
for flags in (['--profile-cadence', '0'], ['--profile-cadence', '3601'], ['--profile-cadence', '1x'],
              ['--gpu-profile-mode', 'bad'], ['--gpu-profile-mode', 'passes'],
              ['--freeze-scene', '--autoplay'], ['--play', '--freeze-scene'],
              ['--freeze-scene', '--resize-check'], ['--freeze-scene', '--dev-overlay']):
    run = subprocess.run([str(BIN), *flags], capture_output=True, text=True, timeout=5)
    assert run.returncode == 1, flags
    results.append({'case': flags, 'result': 'rejected', 'diagnostic': run.stderr.strip()})
(OUT/'summary.json').write_text(json.dumps(results, indent=2))
print(json.dumps(results, indent=2))
