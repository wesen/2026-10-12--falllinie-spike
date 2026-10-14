#!/usr/bin/env python3
import datetime,hashlib,json,os,subprocess
from pathlib import Path
from capture_tools import compress,compare
P=Path(__file__).resolve().parent;R=Path(subprocess.check_output(['git','-C',str(P),'rev-parse','--show-toplevel'],text=True).strip())
C=next(R.glob('ttmp/2026/10/13/FALLLINIE-PERF-004*/scripts/03-mali-counters'))
stamp=datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ');remote='/storage/apps/falllinie-gpu008-'+stamp;out=P/('evidence/target-'+stamp);out.mkdir(parents=True)
opts=['-o','BatchMode=yes','-o','IdentitiesOnly=yes','-o','StrictHostKeyChecking=yes','-o','ConnectTimeout=10','-i',os.environ.get('SSH_IDENTITY_FILE',str(Path.home()/'.ssh/id_ed25519'))];host='root@192.168.0.202'
def ssh(command):return subprocess.check_output(['ssh','-n',*opts,host,command],text=True,timeout=95)
def scp(*args):subprocess.run(['scp',*opts,*map(str,args)],check=True,timeout=90)
ssh(f"set -e; if pgrep -af '[f]alllinie-(spike|trace005|pmu|ranges|gpu00[678])( |$)|[m]ali-counters( |$)|[p]erf (record|stat)'; then exit 2; fi; mkdir '{remote}'; mkdir '{remote}/evidence'")
scp('-r',R/'build-device/falllinie-gpu008',C/'.work/build-arm/mali-counters',P/'03-target-case.sh',P/'04-capture-case.sh',R/'assets',host+':'+remote+'/')
deployed=ssh(f"chmod +x '{remote}/falllinie-gpu008' '{remote}/mali-counters'; sha256sum '{remote}/falllinie-gpu008' '{remote}/mali-counters'");(out/'deployed.sha256').write_text(deployed)
assert deployed.split()[0]==hashlib.sha256((R/'build-device/falllinie-gpu008').read_bytes()).hexdigest()
assets={str(p.relative_to(R)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted((R/'assets').rglob('*')) if p.is_file()}
manifest=''.join(f'{h}  {p}\n' for p,h in assets.items());(out/'assets.sha256').write_text(manifest);scp(out/'assets.sha256',host+':'+remote+'/assets.sha256')
(out/'assets-before.txt').write_text(ssh(f"cd '{remote}' && sha256sum -c assets.sha256"))
meta={'source_revision':subprocess.check_output(['git','-C',str(R),'rev-parse','HEAD'],text=True).strip(),'remote':remote,'binary_sha256':deployed.split()[0],'assets':assets,'complete':False};(out/'campaign.json').write_text(json.dumps(meta,indent=2)+'\n')
visual={}
for frames,seed,palette in [(120,42,0),(600,42,0),(1200,42,0),(600,7,0),(600,42,5)]:
 key=f'f{frames:04d}-s{seed}-p{palette}'
 for mode in ['original','bounds']:
  name='visual-'+key+'-'+mode;path=remote+'/evidence/'+name
  status=0
  try:ssh(f"timeout --kill-after=5s 85s sh '{remote}/04-capture-case.sh' '{remote}' '{path}' {mode} {frames} {seed} {palette}")
  except subprocess.CalledProcessError as e:status=e.returncode
  scp('-r',host+':'+path,out);compress(out/name);print(name,'status',status,flush=True);assert status==0
 a=out/('visual-'+key+'-original');b=out/('visual-'+key+'-bounds')
 ra=json.loads((a/'receipt.json').read_text());rb=json.loads((b/'receipt.json').read_text());assert ra['replay_signature']==rb['replay_signature'] and ra['placement_ok'] and rb['placement_ok']
 visual[key]=compare(a/'capture',b/'capture');(out/'visual-comparisons.json').write_text(json.dumps(visual,indent=2)+'\n')
 # Preserve differences and continue measurement; they block fidelity acceptance, not honest timing collection.
 print(key,'different pixels',sum(v['different_pixels'] for v in visual[key].values()),flush=True)
for i,mode in enumerate(['original','off','bounds','bounds','off','original'],1):
 name=f'{i:02d}-{mode}';path=remote+'/evidence/'+name;status=0
 try:ssh(f"timeout --kill-after=5s 60s sh '{remote}/03-target-case.sh' '{remote}' '{path}' {mode}")
 except subprocess.CalledProcessError as e:status=e.returncode
 scp('-r',host+':'+path,out);compress(out/name);print(name,'status',status,flush=True);assert status==0
(out/'assets-after.txt').write_text(ssh(f"cd '{remote}' && sha256sum -c assets.sha256"));meta['complete']=True;(out/'campaign.json').write_text(json.dumps(meta,indent=2)+'\n');print(out,flush=True)
