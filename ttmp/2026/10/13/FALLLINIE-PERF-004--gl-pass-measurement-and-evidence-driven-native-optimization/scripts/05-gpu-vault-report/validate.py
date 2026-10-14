#!/usr/bin/env python3
"""Publication boundary check: links/assets, evidence values, and report shape."""
import hashlib,json,re,xml.etree.ElementTree as ET
from pathlib import Path
from PIL import Image
P=Path(__file__).resolve().parent
V=Path('/home/manuel/code/wesen/go-go-golems/go-go-parc')
N=V/'Projects/2026/10/13/PROJECT REPORT - Falllinie GPU Profiling - Hardware Counters, Shader Compilation and a Rejected Optimization.md'
A=N.parent/'_assets/falllinie-gpu-shader-investigation'
s=N.read_text();links=re.findall(r'\]\((\./[^)]+)\)',s)
for link in links:assert (N.parent/link).is_file(),link
for path in A.glob('*.svg'):ET.parse(path)
for path in A.glob('*.json'):json.loads(path.read_text())
for path in A.glob('*.png'):
    with Image.open(path) as im:im.verify()
prov=json.loads((A/'report-provenance.json').read_text())
for name,h in prov['assets'].items():assert hashlib.sha256((A/name).read_bytes()).hexdigest()==h,name
shader=json.loads((A/'shader-summary.json').read_text())
assert sum(v['n'] for v in shader['cases'].values())==6308
for k in ['baseline_pooled_mean_ms','candidate_pooled_mean_ms']:assert f'{shader[k]:.6f}' in s
assert all(v['byte_identical'] for v in shader['images'].values())
assert s.count('```mermaid')==2
assert 'diagnostic-only' in s and 'not a proven regression' in s and '1.264146008' in s
assert 'canonical shader is unchanged' in s
prose=re.sub(r'^---\n.*?\n---\n','',s,flags=re.S);prose=re.sub(r'```.*?```','',prose,flags=re.S);prose='\n'.join(l for l in prose.splitlines() if not l.startswith('|'))
result={'note':str(N.relative_to(V)),'note_sha256':hashlib.sha256(N.read_bytes()).hexdigest(),'prose_words':len(re.findall(r"\b[\w'-]+\b",prose)),'mermaid_diagrams':2,'svg_figures':len(list(A.glob('*.svg'))),'png_fixtures':len(list(A.glob('*.png'))),'asset_files':len(list(A.iterdir())),'relative_links_checked':len(links),'checks':'links, SVG XML, JSON parse, PNG integrity, copied hashes, frame total, exact means, fixture equality, claim boundaries','files':[str(N.relative_to(V))]+[str(p.relative_to(V)) for p in sorted(A.iterdir())]}
(P/'validation.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
