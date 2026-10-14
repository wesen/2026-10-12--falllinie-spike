#!/usr/bin/env python3
import json,os,subprocess
from pathlib import Path
from capture_tools import compress,compare
P=Path(__file__).resolve().parent;R=Path(subprocess.check_output(['git','-C',str(P),'rev-parse','--show-toplevel'],text=True).strip());E=P/'evidence/host';E.mkdir(parents=True,exist_ok=True)
signatures=set()
for mode in ['original','off','bounds']:
 out=E/mode;out.mkdir(exist_ok=True)
 args=['xvfb-run','-a',str(R/'build/integrated-source/falllinie-spike'),'--assets',str(R/'assets/shaders'),'--shadow-mode',mode,'--benchmark-replay','--frames','120','--seed','42','--render-size','512x384','--dual-screen','--no-vsync','--fps','0','--receipt',str(out/'receipt.json'),'--capture-dir',str(out/'capture')]
 (out/'argv.json').write_text(json.dumps(args,indent=2)+'\n');env=os.environ.copy();env['FALLLINIE_SHADER_DUMP_DIR']=str(out/'submitted-shaders')
 with (out/'game.log').open('w') as log:subprocess.run(args,env=env,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=90)
 receipt=json.loads((out/'receipt.json').read_text());assert receipt['frames']==120 and receipt['shadow_mode']==mode and receipt['gl_error']==0;signatures.add(receipt['replay_signature']);compress(out)
assert len(signatures)==1
comparisons={mode:compare(E/'original/capture',E/mode/'capture') for mode in ['bounds','off']}
result={'signature':signatures.pop(),'comparisons':comparisons,'scope':'Host GL qualification; not Mali performance or universal parity'}
(E/'validation.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
assert all(x['different_pixels']==0 for x in comparisons['bounds'].values()),'candidate differs; inspect retained pixels'
assert any(x['different_pixels'] for x in comparisons['off'].values()),'shadow omission not visible at this checkpoint'
