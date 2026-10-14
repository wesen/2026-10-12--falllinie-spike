#!/usr/bin/env python3
"""Summarise a PERF-009 campaign directory. Usage: 07-analyze.py CAMPAIGN_DIR [OUTPUT_JSON]

Per timed case and per pooled mode (case name without its numeric prefix):
frame-interval statistics for frames >= 60 (mean, p50/p95/p99, share >= 25 ms,
missed refreshes per frame), counter rates over host poll intervals contained in
the paired-clock window, counter values per loop frame (rate / loop FPS, an
estimate), paired scope means, and receipt health. Writes CAMPAIGN_DIR/summary.json
unless OUTPUT_JSON is given (use that for directories owned by other tickets).
"""
import csv,gzip,json,math,statistics,sys
from collections import defaultdict
from pathlib import Path
PERIOD=1000/60
def pct(xs,q):
 s=sorted(xs);k=(len(s)-1)*q;f=math.floor(k);c=math.ceil(k);return s[f]+(s[c]-s[f])*(k-f)
def frame_stats(iv):
 mean=statistics.fmean(iv)
 return {'n':len(iv),'mean_ms':mean,'p50_ms':pct(iv,.5),'p95_ms':pct(iv,.95),'p99_ms':pct(iv,.99),'long_share':sum(x>=25 for x in iv)/len(iv),'missed_refreshes_per_frame':mean/PERIOD-1,'loop_fps':1000/mean}
def case_summary(d):
 rows=[r for r in csv.DictReader(open(d/'frames.csv')) if int(r['frame'])>=60]
 iv=[float(r['interval_ms']) for r in rows]
 out={'frames':frame_stats(iv),'intervals':iv}
 receipt=json.loads((d/'receipt.json').read_text())
 out['receipt']={k:receipt.get(k) for k in ['replay_signature','gl_error','shadow_mode','replay_audio','audio_disabled','fog_cells','view_culling','short_indices','output_clear','window_depth','geometry_draws','placement_ok']}
 # Paired scopes.
 window=[None,None];scopes=defaultdict(lambda:{'wall':[],'cpu':[]});final=None
 paired=d/'frames.csv.paired.jsonl'
 for line in (gzip.open(str(paired)+'.gz','rt') if not paired.exists() else open(paired)):
  r=json.loads(line)
  if r['record']=='final':final=r
  if r['record']!='sample' or r['frame']<60 or r['wall_status']!='valid':continue
  key=f"{r['stage']}/{r['view']}/{r['pass']}";scopes[key]['wall'].append((r['wall_end_ns']-r['wall_start_ns'])/1e6)
  if r['cpu_status']=='valid':scopes[key]['cpu'].append((r['cpu_end_ns']-r['cpu_start_ns'])/1e6)
  window[0]=r['wall_start_ns'] if window[0] is None else min(window[0],r['wall_start_ns']);window[1]=r['wall_end_ns'] if window[1] is None else max(window[1],r['wall_end_ns'])
 out['paired_final']=final
 out['scopes']={k:{'n':len(v['wall']),'wall_ms':statistics.fmean(v['wall']),'cpu_ms':statistics.fmean(v['cpu']) if v['cpu'] else None} for k,v in sorted(scopes.items())}
 # Counters over contained host intervals.
 by_seq=defaultdict(dict);complete={};errors=0
 for r in csv.DictReader(open(d/'counters.csv')):
  s=int(r['sequence']);complete[s]=int(r['complete_ns'])
  if r['status']=='valid':by_seq[s][r['counter']]=float(r['value'])
  else:errors+=1
 seqs=sorted(complete);sums=defaultdict(float);seconds=0;used=0
 for a,b in zip(seqs,seqs[1:]):
  if b!=a+1 or complete[a]<window[0] or complete[b]>window[1]:continue
  seconds+=(complete[b]-complete[a])/1e9;used+=1
  for k,v in by_seq[b].items():sums[k]+=v
 rates={k:v/seconds for k,v in sums.items()} if seconds else {}
 fps=out['frames']['loop_fps']
 out['counters']={'intervals':used,'seconds':seconds,'nonvalid_rows':errors,'rates_per_second':rates,'per_frame':{k:v/fps for k,v in rates.items()}}
 out['status']=(d/'status.txt').read_text()
 return out
def main():
 root=Path(sys.argv[1]);cases={}
 for d in sorted(p for p in root.iterdir() if p.is_dir() and (p/'frames.csv').exists()):cases[d.name]=case_summary(d)
 groups=defaultdict(list)
 for name in cases:groups[name.split('-',1)[1]].append(name)
 pooled={}
 for g,names in groups.items():
  iv=[x for n in names for x in cases[n]['intervals']]
  per=defaultdict(list)
  for n in names:
   for k,v in cases[n]['counters']['per_frame'].items():per[k].append(v)
  pooled[g]={'cases':names,'frames':frame_stats(iv),'per_frame_mean':{k:statistics.fmean(v) for k,v in per.items()},'signatures':sorted({str(cases[n]['receipt']['replay_signature']) for n in names})}
 for c in cases.values():del c['intervals']
 summary={'campaign':root.name,'cases':cases,'pooled':pooled,'signatures':sorted({str(c['receipt']['replay_signature']) for c in cases.values()})}
 (Path(sys.argv[2]) if len(sys.argv)>2 else root/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
 print(f"{'case':28s} {'mean':>8s} {'p95':>8s} {'p99':>8s} {'long':>6s} {'miss/f':>7s} {'GPUact M/f':>10s}")
 for n,c in cases.items():
  f=c['frames'];g=c['counters']['per_frame'].get('MaliGPUActiveCy',float('nan'))/1e6
  print(f"{n:28s} {f['mean_ms']:8.3f} {f['p95_ms']:8.3f} {f['p99_ms']:8.3f} {f['long_share']:6.3f} {f['missed_refreshes_per_frame']:7.3f} {g:10.2f}")
 print('pooled:')
 for g,p in pooled.items():
  f=p['frames'];a=p['per_frame_mean'].get('MaliGPUActiveCy',float('nan'))/1e6
  print(f"{g:28s} {f['mean_ms']:8.3f} {f['p95_ms']:8.3f} {f['p99_ms']:8.3f} {f['long_share']:6.3f} {f['missed_refreshes_per_frame']:7.3f} {a:10.2f}")
 print('signatures',summary['signatures'])
if __name__=='__main__':main()
