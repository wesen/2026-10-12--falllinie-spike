#!/usr/bin/env python3
"""PERF-009 device campaigns. Usage: 06-campaign.py PLAN

Deploys the current build-device/falllinie-perf009, the PERF-009 counter probe and
assets into a fresh /storage/apps directory, sets the CPU governor to
`performance` for the campaign (restoring the recorded value in all cases), runs
the plan's visual and timed cases, and copies every case directory back.
Later flags override earlier ones. Phases a-c ran on binaries built before the
default promotion (a7626fd, aab2163); their plans now spell the old behaviour out.
"""
import datetime,hashlib,json,os,subprocess,sys
from pathlib import Path
from capture_tools import compress,compare
P=Path(__file__).resolve().parent;R=Path(subprocess.check_output(['git','-C',str(P),'rev-parse','--show-toplevel'],text=True).strip())
BOUNDS=['--shadow-mode','bounds'];AUDIO=['--replay-audio']
FOG=['--fog-cells','constant'];CULL=['--view-culling','on'];SHORT=['--short-indices','on'];CLEAR=['--output-clear','on'];NODEPTH=['--window-depth','0']
GEOM=FOG+CULL+SHORT
# Pre-PERF-009 behaviour; explicit because the binary defaults changed in the promotion commit.
NOGEOM=['--view-culling','off','--short-indices','off']
ORIGINAL=['--shadow-mode','original']+NOGEOM
DEFAULTS=[]
CHECKPOINTS=[(120,42,0),(600,42,0),(1200,42,0),(600,7,0),(600,42,5)]
# Each plan: visual candidates (compared with original at every checkpoint) and timed cases
# (name, size, counter set, cadence, flags).
PLANS={
 'phase-a':{'visual':{},'timed':[(n,'384x288','base',60,f) for n,f in [('01-original',ORIGINAL),('02-bounds',NOGEOM+BOUNDS),('03-bounds',NOGEOM+BOUNDS),('04-original',ORIGINAL),('05-original-audio',ORIGINAL+AUDIO),('06-bounds-audio',NOGEOM+BOUNDS+AUDIO),('07-bounds-audio',NOGEOM+BOUNDS+AUDIO),('08-original-audio',ORIGINAL+AUDIO)]]},
 'phase-b':{'visual':{},'timed':[(n,'384x288','geometry',60,f) for n,f in [('01-original',ORIGINAL),('02-geometry',ORIGINAL+GEOM),('03-geometry',ORIGINAL+GEOM),('04-original',ORIGINAL)]]},
 # Visual identity at the shipping size; timing at 512x384 dual where frames exceed the
 # budget, so GPU savings appear as frame-time changes. Shadow bounds is the new baseline.
 'phase-c':{'visual':{'fog':FOG,'cull':CULL,'short':SHORT,'bounds':BOUNDS,'all':BOUNDS+GEOM},'timed':[(n,'512x384','base',60,NOGEOM+BOUNDS+f) for n,f in [('01-bounds',[]),('02-fog',FOG),('03-short',SHORT),('04-cull',CULL),('05-geometry',GEOM),('06-geometry',GEOM),('07-cull',CULL),('08-short',SHORT),('09-fog',FOG),('10-bounds',[])]]},
 # Acquire-scope attribution at the shipping size (every frame sampled), then window
 # depth at 512x384 where a saving would be visible. Promoted defaults are the baseline.
 # Acceptance: shipping replay with audio, then live autoplay with audio (launcher-like).
 'acceptance':{'visual':{},'timed':[(n,'384x288','base',60,f) for n,f in [('01-original-audio',ORIGINAL+AUDIO),('02-defaults-audio',AUDIO),('03-defaults-audio',AUDIO),('04-original-audio',ORIGINAL+AUDIO)]]+[(n,'live','base',60,f) for n,f in [('05-original-live',ORIGINAL),('06-defaults-live',DEFAULTS),('07-defaults-live',DEFAULTS),('08-original-live',ORIGINAL)]]},
 'phase-d':{'visual':{'defaults':DEFAULTS,'clear':CLEAR,'nodepth':NODEPTH},'timed':[(n,s,'base',c,f) for n,s,c,f in [('01-clear-ship',  '384x288',1,CLEAR),('02-clear-ship','384x288',1,CLEAR),('03-defaults-big','512x384',60,[]),('04-nodepth-big','512x384',60,NODEPTH),('05-nodepth-big','512x384',60,NODEPTH),('06-defaults-big','512x384',60,[])]]},
}
plan_name=sys.argv[1];plan=PLANS[plan_name]
stamp=datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ');remote='/storage/apps/falllinie-perf009-'+stamp;out=P/('evidence/'+plan_name+'-'+stamp);out.mkdir(parents=True)
opts=['-o','BatchMode=yes','-o','IdentitiesOnly=yes','-o','StrictHostKeyChecking=yes','-o','ConnectTimeout=10','-i',os.environ.get('SSH_IDENTITY_FILE',str(Path.home()/'.ssh/id_ed25519'))];host='root@192.168.0.202'
def ssh(command,timeout=120):return subprocess.check_output(['ssh','-n',*opts,host,command],text=True,timeout=timeout)
def scp(*args):subprocess.run(['scp','-q',*opts,*map(str,args)],check=True,timeout=120)
GOV='/sys/devices/system/cpu/cpufreq/policy0/scaling_governor'
ssh(f"set -e; if pgrep -af '[f]alllinie-(spike|trace005|pmu|ranges|gpu00[678]|perf009)( |$)|[m]ali-counters( |$)|[p]erf (record|stat)'; then exit 2; fi; mkdir '{remote}'; mkdir '{remote}/evidence'")
binary=R/'build-device/falllinie-perf009';probe=P/'03-counters/.work/build-arm/mali-counters'
scp('-r',binary,probe,P/'04-target-case.sh',P/'05-capture-case.sh',P/'08-live-case.sh',R/'assets',host+':'+remote+'/')
deployed=ssh(f"chmod +x '{remote}/falllinie-perf009' '{remote}/mali-counters'; sha256sum '{remote}/falllinie-perf009' '{remote}/mali-counters'");(out/'deployed.sha256').write_text(deployed)
assert deployed.split()[0]==hashlib.sha256(binary.read_bytes()).hexdigest()
assets={str(p.relative_to(R)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted((R/'assets').rglob('*')) if p.is_file()}
(out/'assets.sha256').write_text(''.join(f'{h}  {p}\n' for p,h in assets.items()));scp(out/'assets.sha256',host+':'+remote+'/assets.sha256')
(out/'assets-before.txt').write_text(ssh(f"cd '{remote}' && sha256sum -c assets.sha256"))
meta={'plan':plan_name,'source_revision':subprocess.check_output(['git','-C',str(R),'rev-parse','HEAD'],text=True).strip(),'dirty':bool(subprocess.check_output(['git','-C',str(R),'status','--porcelain','--','src','assets'],text=True).strip()),'remote':remote,'binary_sha256':deployed.split()[0],'complete':False}
governor=ssh(f"cat {GOV}").strip();meta['governor_before']=governor
(out/'campaign.json').write_text(json.dumps(meta,indent=2)+'\n')
try:
 ssh(f"echo performance > {GOV}");meta['governor_during']=ssh(f"cat {GOV}").strip()
 visual={}
 for key_frames,seed,palette in CHECKPOINTS if plan['visual'] else []:
  key=f'f{key_frames:04d}-s{seed}-p{palette}'
  for name,flags in [('original',ORIGINAL)]+list(plan['visual'].items()):
   case='visual-'+key+'-'+name;path=remote+'/evidence/'+case;status=0
   try:ssh(f"timeout --kill-after=5s 85s sh '{remote}/05-capture-case.sh' '{remote}' '{path}' 384x288 {key_frames} {seed} {palette} "+' '.join(flags),timeout=100)
   except subprocess.CalledProcessError as e:status=e.returncode
   scp('-r',host+':'+path,out);compress(out/case);print(case,'status',status,flush=True);assert status==0
  base=out/('visual-'+key+'-original');sig=json.loads((base/'receipt.json').read_text())['replay_signature']
  for name in plan['visual']:
   other=out/('visual-'+key+'-'+name);assert json.loads((other/'receipt.json').read_text())['replay_signature']==sig
   visual.setdefault(name,{})[key]=compare(base/'capture',other/'capture')
   print(key,name,'different pixels',sum(v['different_pixels'] for v in visual[name][key].values()),flush=True)
  (out/'visual-comparisons.json').write_text(json.dumps(visual,indent=2)+'\n')
 for name,size,counter_set,cadence,flags in plan['timed']:
  path=remote+'/evidence/'+name;status=0
  command=f"timeout --kill-after=5s 60s sh '{remote}/08-live-case.sh' '{remote}' '{path}' " if size=='live' else f"timeout --kill-after=5s 60s sh '{remote}/04-target-case.sh' '{remote}' '{path}' {size} {counter_set} {cadence} "
  try:ssh(command+' '.join(flags),timeout=80)
  except subprocess.CalledProcessError as e:status=e.returncode
  scp('-r',host+':'+path,out);print(name,'status',status,flush=True);assert status==0
 (out/'assets-after.txt').write_text(ssh(f"cd '{remote}' && sha256sum -c assets.sha256"));meta['complete']=True
finally:
 ssh(f"echo {governor} > {GOV}");meta['governor_after']=ssh(f"cat {GOV}").strip()
 (out/'campaign.json').write_text(json.dumps(meta,indent=2)+'\n');print(out,meta.get('complete'),'governor restored to',meta['governor_after'],flush=True)
