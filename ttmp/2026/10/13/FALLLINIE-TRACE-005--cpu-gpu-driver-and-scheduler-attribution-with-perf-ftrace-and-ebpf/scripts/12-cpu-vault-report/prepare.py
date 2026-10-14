#!/usr/bin/env python3
"""Publish existing CPU PMU evidence only; no new target experiments."""
from pathlib import Path
import hashlib,json,shutil,html
P=Path(__file__).resolve().parent
E=P.parent/'10-cpu-pmu/evidence'
A=Path('/home/manuel/code/wesen/go-go-golems/go-go-parc/Projects/2026/10/14/_assets/falllinie-cpu-pmu')
A.mkdir(parents=True,exist_ok=False)
s=json.loads((E/'summary.json').read_text())
for name in ['summary.json','analysis-provenance.json']:shutil.copyfile(E/name,A/name)
shutil.copyfile(E/'build/qualification.json',A/'build-qualification.json')
example={'source':'target-20261014T144623Z/02-cpu_cycles-inst_retired/perf-stat.csv','relative_interval_end_s':1.001154278,'cycles':664337540,'instructions':112020043,'time_running_ns':489699742,'running_percent':100.0,'note':'First interval; paired main-thread events. Time running is not wall elapsed.'}
(A/'actual-interval.json').write_text(json.dumps(example,indent=2)+'\n')
def begin(title,subtitle,height):return [f'<svg xmlns="http://www.w3.org/2000/svg" width="1080" height="{height}"><rect width="1080" height="{height}" fill="#faf9f6"/><g font-family="sans-serif" fill="#172a3a">',f'<text x="24" y="38" font-size="24">{html.escape(title)}</text>',f'<text x="24" y="67" font-size="14">{html.escape(subtitle)}</text>']
svg=begin('Main-thread CPU PMU: counted events, not inferred latency','Five separate 20-second grouped runs; audio disabled; all groups report 100% scheduling coverage.',380)
rows=[('Cycles / instructions','Instructions per cycle',None),('L1 data cache','Refill / access',None),('L2 data cache','Refill / access',None),('Retired branches','Mispredicted / retired',None),('L1 data TLB','Refill / access',None)]
for i,((name,desc,_),case) in enumerate(zip(rows,list(s['cases'].values())[1:6])):
 ratio=case['pmu']['ratio_second_over_first'];value=f'{ratio:.5f} IPC' if i==0 else f'{ratio*100:.4f}%';y=118+i*43
 for x,txt in [(25,name),(330,desc),(770,value)]:svg.append(f'<text x="{x}" y="{y}" font-size="18">{html.escape(txt)}</text>')
svg.append('<text x="24" y="350" font-size="14">Different event domains and separate runs: do not add these ratios or infer a fraction of stalled CPU time.</text></g></svg>')
(A/'01-counter-ratios.svg').write_text('\n'.join(svg))
dso=s['cases']['07-sample']['cycle_weighted_dso_percent'];svg=begin('Where main-thread cycle weight was attributed','1,873 hardware-cycle samples; 0 reported lost. Period-weighted percentages, not GPU utilization.',470)
for i,(name,pct) in enumerate(dso.items()):
 y=100+i*35;svg.append(f'<text x="24" y="{y+17}" font-size="15">{html.escape(name)}</text><rect x="300" y="{y}" width="{pct*8}" height="22" fill="#277da8"/><text x="{310+pct*8}" y="{y+17}" font-size="15">{pct:.2f}%</text>')
svg.append('<text x="24" y="450" font-size="14">Main TID only, no inheritance; kernel and userspace included. Driver symbols remain largely unavailable.</text></g></svg>')
(A/'02-cycle-attribution.svg').write_text('\n'.join(svg))
svg=begin('Frame-time context across the finite campaign','Whole-run frame >= 60, not the shorter PMU interval; these are descriptive sequential runs.',480)
for tick in [18,20,24,28,30]:
 x=350+(tick-16)*45;svg.append(f'<path d="M{x} 90 V395" stroke="#ddd"/><text x="{x}" y="420" text-anchor="middle">{tick}ms</text>')
for i,(name,item) in enumerate(s['cases'].items()):
 y=110+i*35;svg.append(f'<text x="24" y="{y+5}" font-size="13">{html.escape(name)}</text>')
 for key,color in [('frame_mean_ms','#277da8'),('frame_p95_ms','#d97b24')]:svg.append(f'<circle cx="{350+(item[key]-16)*45}" cy="{y}" r="5" fill="{color}"/>')
svg.append('<text x="24" y="458" font-size="14">Blue: mean. Orange: p95. Baseline brackets do not prove zero observer overhead or an audio-off speedup.</text></g></svg>')
(A/'03-frame-context.svg').write_text('\n'.join(svg))
manifest={'native_evidence_commit':'f8f6632','source_summary_sha256':hashlib.sha256((E/'summary.json').read_bytes()).hexdigest(),'generator_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'assets':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in A.iterdir()}}
(A/'report-provenance.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(A)
