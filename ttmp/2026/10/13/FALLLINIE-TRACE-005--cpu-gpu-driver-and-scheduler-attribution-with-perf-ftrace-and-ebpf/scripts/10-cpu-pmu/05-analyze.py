#!/usr/bin/env python3
"""Validate real PMU intervals and summarize separately scoped quantities."""
import csv,hashlib,json,math,re,sys
from collections import defaultdict
from pathlib import Path
P=Path(__file__).resolve().parent
D=Path(sys.argv[1]) if len(sys.argv)>1 else P/'evidence/target-20261014T144623Z'
hashes={}
def read(p):
 b=p.read_bytes();hashes[str(p.relative_to(P))]=hashlib.sha256(b).hexdigest();return b.decode()
def q(a,v):
 a=sorted(a);x=(len(a)-1)*v;k=int(x);return a[k]+(a[math.ceil(x)]-a[k])*(x-k)
def parse_counts(text):
 groups=defaultdict(list)
 for r in csv.reader(text.splitlines()):
  if not r or r[0].startswith('#'):continue
  assert len(r)>=6,r
  stamp,count,event,runtime,coverage=float(r[0]),int(r[1]),r[3],int(r[4]),float(r[5])
  assert count>=0 and runtime>0 and coverage>=99.99,(event,coverage)
  groups[event].append((stamp,count,runtime,coverage))
 assert len(groups)==2,groups
 a,b=groups.values();assert len(a)>=19 and [x[0] for x in a]==[x[0] for x in b]
 assert [x[2:] for x in a]==[x[2:] for x in b]
 assert all(a[i][0]>a[i-1][0] for i in range(1,len(a)))
 return groups
result={'scope':'Main TID only; no inheritance; audio disabled; kernel and user CPU events included. Separate runs per pair.','cases':{}}
for case in sorted(D.glob('[0-9][0-9]-*')):
 pid=int(read(case/'main-tid.txt'));receipt=json.loads(read(case/'receipt.json'));assert receipt['audio_disabled'] is True
 log=read(case/'game.log');assert 'GL_error=0' in log and 'AUDIO_POLICY disabled=1' in log and 'audio unavailable' not in log
 assert int(read(case/'perf-status.txt'))==0
 pairs=[json.loads(l) for l in read(case/'frames.csv.paired.jsonl').splitlines()]
 final=pairs[-1];samples=[r for r in pairs if r['record']=='sample']
 assert final['pid']==final['tid']==pid
 assert final['written']==final['attempted']==len(samples) and all(final[k]==0 for k in ['dropped','invalid_cpu','invalid_wall','active'])
 lo=int(read(case/'collection-start-ns.txt'));hi=int(read(case/'collection-end-ns.txt'))
 selected=[r for r in samples if r['wall_start_ns']>=lo and r['wall_end_ns']<=hi]
 assert selected and all(r['pid']==r['tid']==pid for r in samples)
 scope=defaultdict(list)
 for r in selected:
  scope[f"{r['stage']}/{r['view']}/{r['pass']}"] .append(((r['wall_end_ns']-r['wall_start_ns'])/1e6,(r['cpu_end_ns']-r['cpu_start_ns'])/1e6))
 frames=[float(r['interval_ms']) for r in csv.DictReader(read(case/'frames.csv').splitlines()) if int(r['frame'])>=60]
 threads=read(case/'threads.txt').splitlines()[1::2]
 assert threads.count('falllinie-pmu')==1 and all(t=='falllinie-pmu' or t.startswith('mali-') for t in threads),threads
 item={'main_tid':pid,'thread_names':threads,'paired_samples':len(samples),'outer_collection_envelope_s':(hi-lo)/1e9,'postwarmup_frames':len(frames),'frame_mean_ms':sum(frames)/len(frames),'frame_p95_ms':q(frames,.95),'frame_p99_ms':q(frames,.99),'paired_envelope_scopes':{k:{'n':len(v),'wall_ms':sum(t[0] for t in v)/len(v),'cpu_ms':sum(t[1] for t in v)/len(v)} for k,v in scope.items()}}
 if (case/'perf-stat.csv').exists():
  groups=parse_counts(read(case/'perf-stat.csv'));events=list(groups)
  totals={k:sum(r[1] for r in v) for k,v in groups.items()}
  assert totals[events[0]]>0
  item['pmu']={'event_totals':totals,'intervals':len(groups[events[0]]),'perf_elapsed_s':groups[events[0]][-1][0],'time_running_s':sum(r[2] for r in groups[events[0]])/1e9,'min_running_percent':min(r[3] for v in groups.values() for r in v),'ratio_second_over_first':totals[events[1]]/totals[events[0]]}
 if (case/'perf-statistics.txt').exists():
  stats=read(case/'perf-statistics.txt');report=read(case/'perf-dso.txt');item['samples']=int(re.search(r'SAMPLE events:\s+(\d+)',stats)[1]);item['reported_lost']=int(re.search(r'Total Lost Samples:\s+(\d+)',report)[1])
  dso={}
  for pct,tid,name in re.findall(r'^\s*([0-9.]+)%\s+(\d+):\S+\s+(\S+)',report,re.M):assert int(tid)==pid;dso[name]=float(pct)
  item['cycle_weighted_dso_percent']=dso
 result['cases'][case.name]=item
# Tiny negative parser checks at the analysis boundary: never silently accept bad coverage or missing data.
fixture=''.join(f'{i},100,,a,10,100.00,,\n{i},50,,b,10,100.00,,\n' for i in range(1,21))
assert len(parse_counts(fixture))==2
for bad in [fixture.replace('100.00','50.00'),fixture.replace('100,,a','<not supported>,,a')]:
 try:parse_counts(bad)
 except (AssertionError,ValueError):pass
 else:raise AssertionError('invalid PMU data accepted')
(P/'evidence/summary.json').write_text(json.dumps(result,indent=2)+'\n')
(P/'evidence/analysis-provenance.json').write_text(json.dumps({'script_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'inputs':hashes},indent=2)+'\n')
for name,item in result['cases'].items():
 print(name,'frames',item['postwarmup_frames'],'mean_ms',round(item['frame_mean_ms'],6),'p95_ms',round(item['frame_p95_ms'],6))
 if 'pmu' in item:print(json.dumps(item['pmu'],indent=2))
 if 'samples' in item:print('samples',item['samples'],'lost',item['reported_lost'],'dso',item['cycle_weighted_dso_percent'])
