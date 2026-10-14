#!/usr/bin/env python3
"""Publish measured GPU008 text and actual figures, preserving prior checkpoints."""
import hashlib,json,re,shutil,subprocess
from pathlib import Path
from PIL import Image
import xml.etree.ElementTree as ET
P=Path(__file__).resolve().parent;D=P/'evidence/target-20261014T194947Z'
V=Path('/home/manuel/code/wesen/go-go-golems/go-go-parc');folder=V/'Projects/2026/10/13'
N=folder/'PROJECT REPORT - Falllinie GPU Profiling - Hardware Counters, Shader Compilation and a Rejected Optimization.md'
A=folder/'_assets/falllinie-gpu-shader-investigation/shadow-bounds';A.mkdir(exist_ok=True,parents=True)
sources={p.name:p for p in (D/'figures').iterdir() if p.is_file()}
for name in ['summary.json','analysis-provenance.json','visual-comparisons.json','baseline-continuity.json','campaign.json']:sources[name]=D/name
sources['build-qualification.json']=P/'evidence/build/qualification.json'
for label,key in [('later','visual-f1200-s42-p0'),('xray','visual-f0600-s42-p5')]:
 for mode in ['original','bounds']:sources[f'{label}-{mode}.png']=D/(key+'-'+mode)/'capture/output.png'
for name,path in sources.items():shutil.copyfile(path,A/name)
analysis=(P.parent/'analysis/01-measured-shadow-footprint-optimization-and-screenshot-qualification.md').read_text()
body=analysis.split('# Conservative shadow footprint: actual cost reduction without observed image loss\n',1)[1].strip()
body=re.sub(r'^### (.+)$',r'#### \1',body,flags=re.M);counter=[0]
def heading(match):
 counter[0]+=1;return f'### 18.{counter[0]} '+match[1]
body=re.sub(r'^## (.+)$',heading,body,flags=re.M)
base='./_assets/falllinie-gpu-shader-investigation/shadow-bounds/'
body=body.replace('../scripts/evidence/target-20261014T194947Z/figures/',base)
extra='## 18. Conservative shadow-footprint rejection: measured candidate with identical tested pixels\n\n'+body+'\n\n'
extra+='#### Additional visual checkpoints and publication evidence\n\n'
extra+=f'![Later original view at approximately509m]({base}later-original.png)\n\n'
extra+=f'![X-ray palette checkpoint, original output]({base}xray-original.png)\n\n'
extra+='The corresponding bounds screenshots are retained beside these assets and match pixel-for-pixel. These images extend the documented check beyond the first short trajectory and default palette; they still do not qualify all glyph transitions or motion.\n\n'
extra+=f'Measured native checkpoint: **a9a6c9f**; implementation: **e970ec5**. See the [actual summary]({base}summary.json), [40-pair comparison results]({base}visual-comparisons.json), [input provenance]({base}analysis-provenance.json), [source/build qualification]({base}build-qualification.json) and [publication hashes]({base}publication-provenance.json). The design covers all five proposed alternatives; only shadow omission and conservative bounds were implemented and measured in this checkpoint.\n\n'
old=N.read_text();marker='\n\n## Falllinie optimization report network';assert old.count(marker)==1 and '## 18. Conservative' not in old
new=old.replace(marker,'\n\n'+extra+marker)
new=re.sub(r'latest_native_evidence_commit: .*', 'latest_native_evidence_commit: a9a6c9f',new)
new=re.sub(r'latest_runtime_commit: .*','latest_runtime_commit: e970ec5',new)
new=re.sub(r'> \[!summary\] Latest checkpoint[^\n]*\n> [^\n]*', '> [!summary] Latest checkpoint — conservative shadows, unchanged tested pixels\n> At dual512×384, conservative shadow-footprint rejection reduces mean frame interval from23.575 to20.636ms (12.46%). All40 multi-endpoint original/candidate image pairs match exactly, including both full-window views. Section18 explains the bound, floating-point fallback, actual screenshots and limitations. The candidate remains opt-in; this is not integrated60FPS acceptance.',new,count=1)
new=new.replace('Section 17 records the subsequently completed scene/post experiment; earlier future-work statements remain historical.','Section 17 records the subsequently completed scene/post experiment. Section18 implements and measures the narrower shadow candidate; earlier future-work statements remain historical.')
start='## What this investigation established\n';historical=old.split(start,1)[1].split(marker)[0]
assert historical==new.split(start,1)[1].split('\n\n## 18. Conservative')[0]
N.write_text(new)
M=folder/'MOC - Optimization and Performance Engineering.md';m=M.read_text();needle='These are changed-image sensitivity results, not accepted production optimizations.';assert needle in m
M.write_text(m.replace(needle,needle+' Section18 follows with conservative shadow rejection:12.46% lower mean interval and40 pixel-equal multi-endpoint comparisons, with full-window screenshots; it remains an opt-in candidate pending broader/integrated qualification.'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
prov={'native_evidence_commit':'a9a6c9f','runtime_commit':'e970ec5','note_sha256':sha(N),'historical_sections_1_through_17_sha256':hashlib.sha256(historical.encode()).hexdigest(),'assets':{name:sha(A/name) for name in sources},'generator_sha256':sha(Path(__file__))}
(A/'publication-provenance.json').write_text(json.dumps(prov,indent=2,sort_keys=True)+'\n')
for name,source in sources.items():assert sha(A/name)==sha(source)
for target in re.findall(r'!?\[[^\]]*\]\(([^)]+)\)',new):
 if not target.startswith(('http://','https://')):assert (N.parent/target).exists(),target
for p in A.glob('*.png'):
 with Image.open(p) as im:im.verify()
ET.parse(A/'timing.svg')
(P/'evidence/publication-validation.json').write_text(json.dumps({'historical_sections_1_through_17_preserved':True,'copied_assets':len(sources),'images_links_and_hashes_valid':True,'note_sha256':sha(N)},indent=2)+'\n')
print(N)
