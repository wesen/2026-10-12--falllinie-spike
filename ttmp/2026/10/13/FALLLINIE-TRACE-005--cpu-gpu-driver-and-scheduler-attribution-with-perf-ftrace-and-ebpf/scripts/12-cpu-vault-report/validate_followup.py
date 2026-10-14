#!/usr/bin/env python3
from pathlib import Path
import hashlib,json,re,xml.etree.ElementTree as ET
from PIL import Image
P=Path(__file__).resolve().parent;V=Path('/home/manuel/code/wesen/go-go-golems/go-go-parc')
N=V/'Projects/2026/10/14/PROJECT REPORT - Falllinie CPU PMU - Cache Events, Driver Cycles and an Audio-Disabled Baseline.md'
M=V/'Projects/2026/10/13/MOC - Optimization and Performance Engineering.md'
A=N.parent/'_assets/falllinie-cpu-pmu/range-draw';s=N.read_text();names={p.stem for p in V.rglob('*.md')}
for doc in [N,M]:
 for link in re.findall(r'\[\[([^]]+)\]\]',doc.read_text()):assert link.split('|')[0] in names
for link in re.findall(r'\]\((\./[^)]+)\)',s):assert (N.parent/link).is_file()
for p in A.glob('*.svg'):ET.parse(p)
for p in A.glob('*.json'):json.loads(p.read_text())
for p in A.glob('*.png'):
 with Image.open(p) as im:im.verify()
for name,h in json.loads((A/'provenance.json').read_text())['assets'].items():assert hashlib.sha256((A/name).read_bytes()).hexdigest()==h
r=json.loads((A/'range-summary.json').read_text());f=json.loads((A/'attribution-summary.json').read_text())
assert f['samples_total']==9895 and sum(v['samples'] for v in r['cases'].values())==7336
for v in r['cases'].values():
 for value in [v['frame_mean_ms'],v['frame_p95_ms'],v['scopes']['render/0/scene']['cpu_ms'],v['scopes']['render/0/view']['cpu_ms']]:assert f'{value:.6f}' in s
 assert bool(v['minmax_helper_sampled_offsets'])!=v['draw_ranges']
assert all(x['equal'] for x in r['fixture'].values())
assert s.count('```mermaid')==2
body=re.sub(r'^---\n.*?\n---\n','',s,flags=re.S);body=re.sub(r'```.*?```','',body,flags=re.S);body='\n'.join(l for l in body.splitlines() if not l.startswith('|'))
result={'native_evidence':'44c0c72','note_sha256':hashlib.sha256(N.read_bytes()).hexdigest(),'prose_words':len(re.findall(r"\b[\w'-]+\b",body)),'new_svg_figures':2,'fixture_pngs':2,'mermaid_diagrams':2,'files':[str(p.relative_to(V)) for p in [N,M,*sorted(A.iterdir())]],'checks':'links, figures/JSON/images, asset hashes, exact table values, sample totals and observed helper absence; interpretation manually reviewed'}
(P/'followup-validation.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
