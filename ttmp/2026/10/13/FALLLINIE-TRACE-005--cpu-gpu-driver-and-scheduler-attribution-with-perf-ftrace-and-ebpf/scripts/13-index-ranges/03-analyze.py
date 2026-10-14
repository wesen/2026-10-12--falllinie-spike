#!/usr/bin/env python3
import csv,hashlib,json,math,re,statistics
from collections import defaultdict
from pathlib import Path
P=Path(__file__).resolve().parent;D=P/'evidence/target-20261014T154415Z';hashes={}
def text(p):
 b=p.read_bytes();hashes[str(p.relative_to(P))]=hashlib.sha256(b).hexdigest();return b.decode()
def q(a,v):
 a=sorted(a);x=(len(a)-1)*v;k=int(x);return a[k]+(a[math.ceil(x)]-a[k])*(x-k)
result={'cases':{},'fixture':{},'decision':'Keep opt-in: bounded CPU benefit, no demonstrated end-to-end frame gain; audio remains disabled.'}
for kind in ['scene','depth','print']:
 a=(D/'capture-baseline'/f'{kind}.bmp').read_bytes();b=(D/'capture-ranges'/f'{kind}.bmp').read_bytes();assert a==b
 result['fixture'][kind]={'equal':True,'sha256':hashlib.sha256(a).hexdigest()}
for case in sorted(D.glob('[0-9][0-9]-*')):
 pid=int(text(case/'main-tid.txt'));receipt=json.loads(text(case/'receipt.json'));assert receipt['audio_disabled'];assert receipt['draw_ranges']==case.name.endswith('range1')
 assert 'GL_error=0' in text(case/'game.log') and int(text(case/'perf-status.txt'))==0
 pairs=[json.loads(l) for l in text(case/'frames.csv.paired.jsonl').splitlines()];final=pairs[-1]
 samples=[r for r in pairs if r['record']=='sample'];assert final['pid']==final['tid']==pid and final['written']==final['attempted']==len(samples)
 assert all(final[k]==0 for k in ['dropped','invalid_cpu','invalid_wall','active'])
 lo=int(text(case/'collection-start-ns.txt'));hi=int(text(case/'collection-end-ns.txt'));groups=defaultdict(list)
 for r in samples:
  assert r['pid']==r['tid']==pid
  if lo<=r['wall_start_ns'] and r['wall_end_ns']<=hi:
   groups[f"{r['stage']}/{r['view']}/{r['pass']}"] .append(((r['wall_end_ns']-r['wall_start_ns'])/1e6,(r['cpu_end_ns']-r['cpu_start_ns'])/1e6))
 frames=[float(r['interval_ms']) for r in csv.DictReader(text(case/'frames.csv').splitlines()) if int(r['frame'])>=60]
 dso=text(case/'perf-dso.txt');stats=text(case/'perf-statistics.txt');report=text(case/'perf-report.txt')
 assert int(re.search(r'Total Lost Samples:\s+(\d+)',dso)[1])==0
 hotspot=re.search(r'^\s*([0-9.]+)%.*\[\.\] 0x000000000042d6dc',report,re.M)
 item={'draw_ranges':receipt['draw_ranges'],'frame_n':len(frames),'frame_mean_ms':statistics.mean(frames),'frame_p95_ms':q(frames,.95),'frame_p99_ms':q(frames,.99),'paired_samples':len(samples),'scopes':{k:{'n':len(v),'wall_ms':statistics.mean(t[0] for t in v),'cpu_ms':statistics.mean(t[1] for t in v)} for k,v in groups.items()},'samples':int(re.search(r'SAMPLE events:\s+(\d+)',stats)[1]),'lost':0,'sampled_event_count_approx':int(re.search(r'Event count \(approx.\):\s+(\d+)',dso)[1]),'driver_weight_percent':float(re.search(r'^\s*([0-9.]+)%.*libmali.so.1.10.0',dso,re.M)[1]),'hotspot_weight_percent':float(hotspot[1]) if hotspot else None,'hotspot_null_means':'Not displayed at 0.5% cutoff; not proof of zero.'}
 full=text(case/'perf-symbols-all.txt')
 helper=[]
 for line in full.splitlines():
  m=re.search(r'^\s*([0-9.]+)%.*libmali.so.1.10.0\s+\[\.\]\s+0x([0-9a-f]+)',line)
  if m and 0x42d6c4<=int(m[2],16)<0x42d758:helper.append({'offset':hex(int(m[2],16)),'weight_percent':float(m[1])})
 item['minmax_helper_sampled_offsets']=helper
 item['minmax_helper_rounded_weight_percent']=sum(x['weight_percent'] for x in helper)
 result['cases'][case.name]=item
for mode in [False,True]:
 cases=[v for v in result['cases'].values() if v['draw_ranges']==mode]
 result['range' if mode else 'baseline']={'frame_pooled_mean_ms':sum(v['frame_mean_ms']*v['frame_n'] for v in cases)/sum(v['frame_n'] for v in cases)}
(P/'evidence/summary.json').write_text(json.dumps(result,indent=2)+'\n');(P/'evidence/provenance.json').write_text(json.dumps(hashes,indent=2)+'\n')
for name,v in result['cases'].items():
 print(name,'mean/p95',v['frame_mean_ms'],v['frame_p95_ms'],'driver%',v['driver_weight_percent'],'hotspot%',v['hotspot_weight_percent'])
 for k,s in v['scopes'].items():
  if k.startswith('render/') and k.endswith(('/view','/scene','/output')):print(k,s)
print('pooled means',result['baseline'],result['range'])
for name,item in result['cases'].items():print('helper',name,item['minmax_helper_sampled_offsets'])
