#!/usr/bin/env python3
"""One bounded functional replay per asset package; no host performance claim."""
import hashlib,json,os,subprocess
from pathlib import Path
P=Path(__file__).resolve().parent
R=Path(subprocess.check_output(['git','-C',str(P),'rev-parse','--show-toplevel'],text=True).strip())
E=P/'evidence/host';E.mkdir(parents=True,exist_ok=True)
variants=['baseline','post-copy','scene-flat','both'];signatures=set();images={}
for mode in variants:
    out=E/mode;out.mkdir(exist_ok=True)
    args=['xvfb-run','-a',str(R/'build/integrated-source/falllinie-spike'),'--assets',str(P/'inputs/assets'/mode),'--benchmark-replay','--frames','60','--seed','42','--render-size','256x192','--dual-screen','--no-vsync','--fps','0','--receipt',str(out/'receipt.json'),'--capture-dir',str(out/'capture')]
    (out/'argv.json').write_text(json.dumps(args,indent=2)+'\n')
    env=os.environ.copy();env['FALLLINIE_SHADER_DUMP_DIR']=str(out/'submitted-shaders')
    with (out/'game.log').open('w') as log:subprocess.run(args,env=env,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=40)
    receipt=json.loads((out/'receipt.json').read_text());assert receipt['frames']==60 and receipt['gl_error']==0 and receipt['benchmark_replay'] and receipt['audio_disabled']
    signatures.add(receipt['replay_signature']);images[mode]={kind:(out/'capture'/f'{kind}.bmp').read_bytes() for kind in ['scene','depth','print']}
assert len(signatures)==1
assert len({hashlib.sha256(images[m]['depth']).hexdigest() for m in variants})==1
assert images['baseline']['scene']==images['post-copy']['scene']
assert images['scene-flat']['scene']==images['both']['scene']
assert all(images[m]['print']!=images['baseline']['print'] for m in variants[1:])
result={'cases':variants,'frames_per_case':60,'signature':signatures.pop(),'depth_diagnostics_match':True,'post_change_preserves_scene':True,'non_parity_prints_differ':True,'scope':'Host linking/consistency only; not Mali performance or full appearance qualification'}
(E/'validation.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
