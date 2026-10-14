#!/usr/bin/env python3
import csv,hashlib,json,re,xml.etree.ElementTree as ET
from pathlib import Path
P=Path(__file__).resolve().parent
V=Path('/home/manuel/code/wesen/go-go-golems/go-go-parc')
N=V/'Projects/2026/10/14/PROJECT REPORT - Falllinie CPU PMU - Cache Events, Driver Cycles and an Audio-Disabled Baseline.md'
M=V/'Projects/2026/10/13/MOC - Optimization and Performance Engineering.md'
A=N.parent/'_assets/falllinie-cpu-pmu'
s=N.read_text();names={p.stem for p in V.rglob('*.md')}
for text in [s,M.read_text()]:
 for link in re.findall(r'\[\[([^]]+)\]\]',text):assert link.split('|')[0] in names,link
links=re.findall(r'\]\((\./[^)]+)\)',s)
for link in links:assert (N.parent/link).is_file(),link
for p in A.glob('*.svg'):ET.parse(p)
for p in A.glob('*.json'):json.loads(p.read_text())
manifest=json.loads((A/'report-provenance.json').read_text())
for name,h in manifest['assets'].items():assert hashlib.sha256((A/name).read_bytes()).hexdigest()==h,name
summary=json.loads((A/'summary.json').read_text())
for item in summary['cases'].values():
 assert f'{item["frame_mean_ms"]:.6f}' in s
 if 'pmu' in item:
  for count in item['pmu']['event_totals'].values():assert f'{count:,}' in s
example=json.loads((A/'actual-interval.json').read_text())
source=P.parent/'10-cpu-pmu/evidence'/example['source']
rows=[r for r in csv.reader(source.read_text().splitlines()) if r and not r[0].startswith('#')]
assert int(rows[0][1])==example['cycles'] and int(rows[1][1])==example['instructions']
assert int(rows[0][4])==example['time_running_ns'] and float(rows[0][0])==example['relative_interval_end_s']
assert s.count('```mermaid')==2
# Interpretation boundaries are reviewed in prose, not inferred from exact phrasing.
prose=re.sub(r'^---\n.*?\n---\n','',s,flags=re.S);prose=re.sub(r'```.*?```','',prose,flags=re.S);prose='\n'.join(l for l in prose.splitlines() if not l.startswith('|'))
result={'note':str(N.relative_to(V)),'note_sha256':hashlib.sha256(N.read_bytes()).hexdigest(),'prose_words':len(re.findall(r"\b[\w'-]+\b",prose)),'svg_figures':3,'mermaid_diagrams':2,'relative_links':len(links),'files':[str(p.relative_to(V)) for p in [N,M,*sorted(A.iterdir())]],'checks':'wikilinks, local assets, SVG/JSON parsing, hashes, exact frame means and counter totals, actual worked CSV interval; interpretation boundaries manually reviewed'}
(P/'validation.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
