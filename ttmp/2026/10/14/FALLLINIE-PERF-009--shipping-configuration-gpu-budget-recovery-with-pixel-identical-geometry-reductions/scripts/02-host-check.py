#!/usr/bin/env python3
"""Host GL qualification of PERF-009 policies: same replay signature, identical captures.

Runs the host build under xvfb at the shipping internal size and compares every
captured image of each candidate configuration against the original policy.
Host GL is not Mali; this only checks wiring and pixel identity on a second driver.
"""
import json,os,subprocess,sys
from pathlib import Path
from capture_tools import compress,compare
P=Path(__file__).resolve().parent;R=Path(subprocess.check_output(['git','-C',str(P),'rev-parse','--show-toplevel'],text=True).strip());E=P/'evidence/host';E.mkdir(parents=True,exist_ok=True)
CONFIGS={
 'original':['--shadow-mode','original','--view-culling','off','--short-indices','off'],
 'defaults':[],
 'fog':['--fog-cells','constant'],
 'cull':['--view-culling','on'],
 'short':['--short-indices','on'],
 'clear':['--output-clear','on'],
 'combined':['--fog-cells','constant','--view-culling','on','--short-indices','on','--output-clear','on','--shadow-mode','bounds'],
}
frames=sys.argv[1] if len(sys.argv)>1 else '240'
signatures={};stats={}
for name,extra in CONFIGS.items():
 out=E/name;out.mkdir(exist_ok=True)
 args=['xvfb-run','-a',str(R/'build/integrated-source/falllinie-spike'),'--assets',str(R/'assets/shaders'),'--benchmark-replay','--frames',frames,'--seed','42','--render-size','384x288','--dual-screen','--no-vsync','--fps','0','--receipt',str(out/'receipt.json'),'--capture-dir',str(out/'capture'),*extra]
 (out/'argv.json').write_text(json.dumps(args,indent=2)+'\n')
 with (out/'game.log').open('w') as log:subprocess.run(args,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=180)
 receipt=json.loads((out/'receipt.json').read_text());assert receipt['gl_error']==0
 signatures[name]=receipt['replay_signature'];stats[name]=receipt['geometry_draws'];compress(out)
assert len(set(signatures.values()))==1,signatures
comparisons={name:compare(E/'original/capture',E/name/'capture') for name in CONFIGS if name!='original'}
result={'frames':int(frames),'signature':signatures['original'],'draw_stats':stats,'comparisons':comparisons,'scope':'Host GL wiring and pixel identity; not Mali timing or device qualification'}
(E/'validation.json').write_text(json.dumps(result,indent=2)+'\n')
for name,c in comparisons.items():print(name,'different_pixels',sum(v['different_pixels'] for v in c.values()),'stats',stats[name])
assert all(v['different_pixels']==0 for c in comparisons.values() for v in c.values()),'candidate differs; inspect retained images'
