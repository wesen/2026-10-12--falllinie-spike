#!/usr/bin/env python3
"""Close the shader experiment from actual images, ABBA frames and counter rows."""
import csv,hashlib,json,math,re
from collections import defaultdict
from pathlib import Path
P=Path(__file__).resolve().parent
D=P/'evidence/target-20261014T032113Z'
inputs={}
def text(p):
    b=p.read_bytes();inputs[str(p.relative_to(P))]=hashlib.sha256(b).hexdigest();return b.decode()
def rows(p): return list(csv.DictReader(text(p).splitlines()))
def quantile(a,q):
    a=sorted(a);x=(len(a)-1)*q; k=int(x);return a[k]+(a[math.ceil(x)]-a[k])*(x-k)
summary={'cases':{},'images':{},'decision':'Do not promote: no convincing improvement in this finite live ABBA experiment; retain diagnostic variants.'}
for kind in ['scene','depth','print']:
    a=(D/'capture-baseline'/f'{kind}.bmp').read_bytes();b=(D/'capture-combined'/f'{kind}.bmp').read_bytes()
    summary['images'][kind]={'baseline_sha256':hashlib.sha256(a).hexdigest(),'candidate_sha256':hashlib.sha256(b).hexdigest(),'byte_identical':a==b}
    inputs[f'{D.relative_to(P)}/capture-baseline/{kind}.bmp']=hashlib.sha256(a).hexdigest();inputs[f'{D.relative_to(P)}/capture-combined/{kind}.bmp']=hashlib.sha256(b).hexdigest()
for case in sorted(D.glob('[0-9][0-9]-*')):
    fr=[r for r in rows(case/'frames.csv') if int(r['frame'])>=60];ms=[float(r['interval_ms']) for r in fr]
    item={'n':len(ms),'mean_ms':sum(ms)/len(ms),'p50_ms':quantile(ms,.5),'p95_ms':quantile(ms,.95),'p99_ms':quantile(ms,.99)}
    pairs=[json.loads(l) for l in text(case/'frames.csv.paired.jsonl').splitlines()]
    final=pairs[-1];assert final['active']==0 and final['dropped']==0 and final['invalid_cpu']==0 and final['invalid_wall']==0
    samples=[r for r in pairs if r['record']=='sample'];assert len(samples)==final['written']==final['attempted']
    item['paired_records']=len(samples)
    active=[r for r in samples if r['frame']>=60];lo=min(r['wall_start_ns'] for r in active);hi=max(r['wall_end_ns'] for r in active)
    groups=defaultdict(list)
    for r in active:
        if r['stage']=='render':groups[f"{r['view']}/{r['pass']}"] .append(((r['wall_end_ns']-r['wall_start_ns'])/1e6,(r['cpu_end_ns']-r['cpu_start_ns'])/1e6))
    item['paired_groups_ms']={k:{'n':len(v),'wall':sum(t[0] for t in v)/len(v),'cpu':sum(t[1] for t in v)/len(v)} for k,v in groups.items()}
    cr=rows(case/'counters.csv');seq=defaultdict(list)
    for r in cr:seq[int(r['sequence'])].append(r)
    assert sorted(seq)==list(range(450)) and all(len(v)==13 for v in seq.values())
    log=text(case/'counters.log');assert f'final rows={len(cr)} errors=0' in log and 'stop=ok' in log
    values=defaultdict(list); prev=None;interval=0;selected=0;nonfinite=0
    for rr in seq.values():
        end=int(rr[0]['complete_ns'])
        if prev is not None and prev>=lo and end<=hi:
            interval+=(end-prev)/1e9;selected+=1
            for r in rr:
                if r['status']=='valid':values[r['counter']].append(float(r['value']))
                else:nonfinite+=1
        prev=end
    item['counter_rows']=len(cr);item['central_intervals']=selected;item['central_nonfinite_rows']=nonfinite;item['host_counter_seconds']=interval
    item['rates']={k:sum(v)/interval for k,v in values.items() if k.endswith('Cy') or k.endswith('By')}
    item['ratio_means']={k:sum(v)/len(v) for k,v in values.items() if not (k.endswith('Cy') or k.endswith('By'))}
    gl=text(case/'game.log');item['health']={k:re.findall(rf'\b{k}=([^\s]+)',gl)[-1] for k in ['GL_error','over_budget','starved_callbacks','missing_frames']}
    summary['cases'][case.name]=item
for group,names in [('baseline',['01-baseline','04-baseline']),('candidate',['02-combined','03-combined'])]:
    s=[summary['cases'][n] for n in names]
    summary[group+'_pooled_mean_ms']=sum(r['mean_ms']*r['n'] for r in s)/sum(r['n'] for r in s)
# Actual frame CDF; no simulated timing series.
svg=['<svg xmlns="http://www.w3.org/2000/svg" width="1080" height="510"><rect width="1080" height="510" fill="#faf9f6"/><g font-family="sans-serif" fill="#172a3a">','<text x="24" y="36" font-size="24">Compiler-guided candidate: no clear live frame-time gain</text>','<text x="24" y="62" font-size="14">Four 30-second ABBA runs; counters enabled; frame &gt;= 60; unchanged runtime binary.</text>']
for tick in range(0,41,10):
    x=80+tick/40*910;svg.append(f'<path d="M{x} 90 V380" stroke="#ddd"/><text x="{x}" y="402" text-anchor="middle">{tick}ms</text>')
for tick in range(0,101,25):
    y=380-tick*2.9;svg.append(f'<path d="M80 {y} H990" stroke="#ddd"/><text x="65" y="{y+4}" text-anchor="end">{tick}%</text>')
for i,name in enumerate(summary['cases']):
    a=sorted(float(r['interval_ms']) for r in rows(D/name/'frames.csv') if int(r['frame'])>=60)
    color=['#287bb5','#e59837','#a96514','#174b73'][i]
    points=' '.join(f'{80+min(v,40)/40*910:.2f},{380-(j+1)/len(a)*290:.2f}' for j,v in enumerate(a))
    svg.append(f'<polyline points="{points}" fill="none" stroke="{color}" stroke-width="2"/><text x="{25+i*260}" y="445" fill="{color}" font-size="16">{name}</text>')
svg.append('<text x="24" y="485" font-size="13">Finite sequential live runs; uncontrolled clocks/trajectories. Equal fixture images do not prove universal parity.</text></g></svg>')
(P/'evidence/frame-abba.svg').write_text('\n'.join(svg))
# Compiler table as self-contained figure; keep static estimates separate from timings.
svg=['<svg xmlns="http://www.w3.org/2000/svg" width="1080" height="380"><rect width="1080" height="380" fill="#faf9f6"/><g font-family="sans-serif" fill="#172a3a">','<text x="24" y="36" font-size="24">Offline compiler: local changes interact through register allocation</text>','<text x="24" y="64" font-size="14">Mali-G52 r1p0; compiler 2026.5, r51p0 backend. Static estimates, not measured shader milliseconds.</text>']
for i,row in enumerate([['Variant','Work regs','Occupancy','Total A','Total T','Shortest A'],['Baseline','32','100%','10.63','3.50','3.53'],['Lazy color','32','100%','10.75','5.00','3.67'],['Fog skip','60','50%','10.67','2.50','2.67'],['Combined','32','100%','10.80','3.00','2.70']]):
    y=110+i*42
    for x,value in zip([25,290,465,635,775,915],row):svg.append(f'<text x="{x}" y="{y}" font-size="17" font-weight="{ "bold" if i==0 else "normal"}">{value}</text>')
svg.append('<text x="24" y="337" font-size="14">All four report no stack use. Combined avoids fog-only occupancy loss, but is not a predicted universal win.</text></g></svg>')
(P/'evidence/compiler-comparison.svg').write_text('\n'.join(svg))
for name in ['baseline','lazy-color','fog-skip','combined']:text(P/'evidence/reports'/f'{name}.txt')
(P/'evidence/experiment-summary.json').write_text(json.dumps(summary,indent=2)+'\n')
(P/'evidence/experiment-provenance.json').write_text(json.dumps({'script_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'inputs':inputs},indent=2)+'\n')
print(json.dumps(summary,indent=2))
