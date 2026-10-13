#!/usr/bin/env python3
"""Compile exact base and candidate, compare 90 seconds of float PCM and traces."""
from pathlib import Path
import hashlib, json, subprocess
HERE=Path(__file__).resolve().parent
ROOT=Path(subprocess.check_output(['git','rev-parse','--show-toplevel'],cwd=HERE,text=True).strip())
BASE='0ff0876ef3e0bafbad9a04c3c34b08f76aacc1f8'
WORK=HERE/'.work/source-identity';WORK.mkdir(parents=True,exist_ok=True)
flags=subprocess.check_output(['pkg-config','--cflags','--libs','sdl2'],text=True).split()
records=[]
for label in ['base','candidate']:
 folder=WORK/label;folder.mkdir(exist_ok=True)
 if label=='base':
  for name in ['audio.cpp','audio.hpp','audio_dsp.hpp']:
   (folder/name).write_bytes(subprocess.check_output(['git','show',BASE+':src/'+name],cwd=ROOT))
  cpp=folder/'audio.cpp';includes=['-I'+str(folder),'-I'+str(ROOT/'src')]
 else:
  cpp=ROOT/'src/audio.cpp';includes=['-I'+str(ROOT/'src')]
 binary=folder/'render';pcm=folder/'source.f32';trace=folder/'trace.csv'
 subprocess.run(['g++','-std=c++17','-O2','-pthread',*includes,str(HERE/'render-source-reference.cpp'),str(cpp),'-o',str(binary),*flags],check=True)
 with trace.open('w') as log:subprocess.run([str(binary),str(pcm)],stdout=log,check=True,timeout=60)
 records.append({'variant':label,'float_bytes':pcm.stat().st_size,'pcm_sha256':hashlib.sha256(pcm.read_bytes()).hexdigest(),'trace_sha256':hashlib.sha256(trace.read_bytes()).hexdigest()})
assert records[0]['pcm_sha256']==records[1]['pcm_sha256'],'source float PCM differs from base'
assert records[0]['trace_sha256']==records[1]['trace_sha256'],'source sequence differs from base'
manifest={'base':BASE,'candidate_source_commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),'rate':48000,'seed':19,'seconds':90,'source_vinyl':True,'bit_identical':True,'renders':records}
(HERE/'source-identity.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps(manifest,indent=2))
