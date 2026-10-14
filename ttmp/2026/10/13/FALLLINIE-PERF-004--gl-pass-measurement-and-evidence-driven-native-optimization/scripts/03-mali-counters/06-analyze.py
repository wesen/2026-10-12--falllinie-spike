#!/usr/bin/env python3
"""Bounded G52 campaign summary; whole-device counters, not per-process GPU timing."""
import csv, hashlib, html, json, math, re
from collections import defaultdict
from pathlib import Path
P=Path(__file__).resolve().parent
D=P/'evidence/target-20261014T024712Z'
inputs={}
def text(p):
    b=p.read_bytes(); inputs[str(p.relative_to(P))]=hashlib.sha256(b).hexdigest(); return b.decode()
def csvrows(p): return list(csv.DictReader(text(p).splitlines()))
def mean(a): return sum(a)/len(a) if a else None
def quantile(a,q):
    a=sorted(a); x=(len(a)-1)*q; k=int(x); return a[k]+(a[math.ceil(x)]-a[k])*(x-k)
summary={}
for case in sorted(D.glob('[0-9][0-9]-*')):
    item={}; summary[case.name]=item
    assert 'probe_status=0' in text(case/'status.txt') or 'probe_status=not-run' in text(case/'status.txt')
    start=0; end=2**64
    if (case/'frames.csv').exists():
        frames=[r for r in csvrows(case/'frames.csv') if int(r['frame'])>=60]
        ms=[float(r['interval_ms']) for r in frames]
        item['frame_ms']={'n':len(ms),'mean':mean(ms),'p50':quantile(ms,.5),'p95':quantile(ms,.95),'p99':quantile(ms,.99)}
        pairs=[json.loads(l) for l in text(case/'frames.csv.paired.jsonl').splitlines()]
        assert pairs[-1]['active']==0 and pairs[-1]['dropped']==0
        active=[p for p in pairs if p['record']=='sample' and p['frame']>=60]
        start=min(p['wall_start_ns'] for p in active); end=max(p['wall_end_ns'] for p in active)
        item['window_mono_ns']=[start,end]
        log=text(case/'game.log')
        item['health']={k:re.findall(rf'\b{k}=([^\s]+)',log)[-1] for k in ['GL_error','over_budget','starved_callbacks','missing_frames']}
    if not (case/'counters.csv').exists(): continue
    rows=csvrows(case/'counters.csv'); log=text(case/'counters.log')
    final=re.search(r'final rows=(\d+) errors=(\d+) nonfinite=(\d+) stop=(\S+)',log)
    assert final and int(final[1])==len(rows) and final[2]=='0' and final[4]=='ok'
    assert int(final[3])==sum(r['status']=='nonfinite' for r in rows)
    item['raw_rows']=len(rows); item['nonfinite_rows']=int(final[3]); item['advertised_counters']=len(re.findall(r'^available=',log,re.M))
    byseq=defaultdict(list)
    for r in rows: byseq[int(r['sequence'])].append(r)
    assert sorted(byseq)==list(range(len(byseq)))
    names={r['counter'] for r in byseq[0]}
    assert len(names)==13
    series=defaultdict(list); durations=[]; selected=[]; prev=None
    offset=[]
    for seq, rr in sorted(byseq.items()):
        assert len(rr)==13 and {r['counter'] for r in rr}==names
        request=int(rr[0]['request_ns']); complete=int(rr[0]['complete_ns'])
        offset.append((int(rr[0]['counter_timestamp_ns'])-complete)/1e9)
        # Skip first interval and require both adjacent polls inside game window.
        if prev is not None and prev>=start and complete<=end:
            dt=(complete-prev)/1e9; durations.append(dt); selected.append(rr)
            for r in rr:
                if r['status']=='valid':
                    value=float(r['value']); assert math.isfinite(value)
                    series[r['counter']].append(value)
        prev=complete
    elapsed=sum(durations)
    item['selected_intervals']=len(durations); item['host_interval_seconds']=elapsed
    item['selected_nonfinite_rows']=sum(r['status']=='nonfinite' for rr in selected for r in rr)
    item['backend_begin_minus_host_complete_s']={'min':min(offset),'max':max(offset)}
    item['poll_wall_us']={'mean':mean([(int(rr[0]['complete_ns'])-int(rr[0]['request_ns']))/1000 for rr in byseq.values()])}
    item['counter_means']={k:mean(v) for k,v in series.items()}
    item['rates_per_second']={k:sum(v)/elapsed for k,v in series.items() if k.endswith('Cy') or k.endswith('By')}
    # Whole-window ratio from cycle counts rather than unweighted per-sample ratios.
    item['fragment_active_over_gpu_active_percent']=100*sum(series['MaliFragActiveCy'])/sum(series['MaliGPUActiveCy']) if sum(series['MaliGPUActiveCy']) else None
    item['gpu_frequency_snapshots_hz']=sorted({int(r['gpu_freq_snapshot_hz']) for rr in selected for r in rr})
# Lightweight comparison graph; each metric has its own scale and units.
svg=['<svg xmlns="http://www.w3.org/2000/svg" width="1050" height="620"><rect width="1050" height="620" fill="#faf9f6"/><g font-family="sans-serif" fill="#172a3a">', '<text x="24" y="36" font-size="24">Actual Mali-G52 counters: live versus post bypass</text>', '<text x="24" y="64" font-size="14">Whole-device sampled intervals within gameplay window; bypass is visually non-parity.</text>']
metrics=[('GPU active cycles / s (millions)','rates_per_second','MaliGPUActiveCy',1e6),('Fragment active cycles / s (millions)','rates_per_second','MaliFragActiveCy',1e6),('External GPU reads (MB/s)','rates_per_second','MaliExtBusRdBy',1e6),('External GPU writes (MB/s)','rates_per_second','MaliExtBusWrBy',1e6)]
for i,(label,group,key,scale) in enumerate(metrics):
    vals=[summary[c][group][key]/scale for c in ['03-live','04-bypass']]; maxv=max(vals)*1.1 or 1
    y=108+i*112; svg.append(f'<text x="24" y="{y}" font-size="16">{html.escape(label)}</text>')
    for j,v in enumerate(vals):
        w=v/maxv*790; yy=y+12+j*29
        svg.append(f'<rect x="170" y="{yy}" width="{w}" height="22" fill="{["#287bb5","#e59837"][j]}"/><text x="155" y="{yy+17}" text-anchor="end" font-size="14">{["Live","Bypass"][j]}</text><text x="{178+w}" y="{yy+17}" font-size="13">{v:.2f}</text>')
svg.append('<text x="24" y="589" font-size="13">Rates use adjacent host poll intervals, not uncertain backend clock alignment; compositor is included.</text></g></svg>')
(P/'evidence/counter-comparison.svg').write_text('\n'.join(svg))
svg=['<svg xmlns="http://www.w3.org/2000/svg" width="1050" height="520"><rect width="1050" height="520" fill="#faf9f6"/><g font-family="sans-serif" fill="#172a3a">','<text x="24" y="36" font-size="24">Live post bypass changes the frame-time distribution</text>','<text x="24" y="62" font-size="14">Empirical CDF; frame &gt;= 60; baseline cases have no GPU counter sampler. Bypass is non-parity.</text>']
for tick in range(0,41,10):
    x=80+tick/40*900; svg.append(f'<path d="M{x} 90 V390" stroke="#ddd"/><text x="{x}" y="412" text-anchor="middle">{tick}ms</text>')
for tick in range(0,101,25):
    y=390-tick*3; svg.append(f'<path d="M80 {y} H980" stroke="#ddd"/><text x="65" y="{y+4}" text-anchor="end">{tick}%</text>')
for i,name in enumerate(['02-baseline','03-live','04-bypass','05-baseline']):
    values=sorted(float(r['interval_ms']) for r in csvrows(D/name/'frames.csv') if int(r['frame'])>=60)
    color=['#777','#287bb5','#e59837','#9a68a0'][i]
    points=' '.join(f'{80+min(v,40)/40*900:.2f},{390-(j+1)/len(values)*300:.2f}' for j,v in enumerate(values))
    svg.append(f'<polyline points="{points}" fill="none" stroke="{color}" stroke-width="2"/><text x="{30+i*250}" y="455" fill="{color}" font-size="16">{name}</text>')
svg.append('<text x="24" y="496" font-size="13">Sequential runs, uncontrolled CPU/GPU clocks and live trajectories; not a source-faithful optimization result.</text></g></svg>')
(P/'evidence/frame-comparison.svg').write_text('\n'.join(svg))
(P/'evidence/summary.json').write_text(json.dumps(summary,indent=2)+'\n')
(P/'evidence/analysis-provenance.json').write_text(json.dumps({'script_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'inputs':inputs,'scope':'Whole-device; central intervals inside frame>=60 paired-clock extent; no application-only or per-pass claims'},indent=2)+'\n')
for name,data in summary.items():
    print(name,json.dumps(data,indent=2))
