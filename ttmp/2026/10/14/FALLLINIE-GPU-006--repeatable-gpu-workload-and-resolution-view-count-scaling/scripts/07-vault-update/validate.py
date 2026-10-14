#!/usr/bin/env python3
"""Validate publication inputs/assets/history, not GPU counter semantics."""
import hashlib
import json
import re
import subprocess
import xml.etree.ElementTree as ET
from pathlib import Path
from PIL import Image
from prepare import P,T,R,V,NOTE,A,D,BASE,digest,historical

note = (V/NOTE).read_text()
moc_path = V/'Projects/2026/10/13/MOC - Optimization and Performance Engineering.md'
moc = moc_path.read_text()
original = subprocess.check_output(['git','-C',str(V),'show',f'{BASE}:{NOTE}'],text=True)
assert historical(note) == historical(original)
old_dir = NOTE.parent/'_assets/falllinie-gpu-shader-investigation'
old_files = subprocess.check_output(['git','-C',str(V),'ls-tree','-r','--name-only',BASE,'--',str(old_dir)],text=True).splitlines()
for name in old_files:
    assert (V/name).read_bytes() == subprocess.check_output(['git','-C',str(V),'show',f'{BASE}:{name}']), name
provenance = json.loads((V/A/'publication-provenance.json').read_text())
assert provenance['note_sha256'] == digest(V/NOTE)
for name, expected in provenance['vault_assets'].items():
    assert digest(V/name) == expected, name
for name, expected in provenance['native_sources'].items():
    assert digest(R/name) == expected, name
s = json.loads((V/A/'summary.json').read_text())
assert len(s['cases']) == 8 and s['same_resolution_endpoint_images_match']
assert s['signature'] == '6081555844518115723'
assert sum(v['frames'] for v in s['cases'].values()) == 4320
assert sum(v['counter_intervals'] for v in s['cases'].values()) == 674
for value in s['cases'].values():
    assert f"{value['mean_ms']:.4f}" in note and f"{value['p95_ms']:.4f}" in note
    for name in ['MaliGPUActiveCy','MaliFragActiveCy','MaliExtBusRdBy','MaliExtBusWrBy']:
        assert f"{value['rates_per_second'][name]/1e6:.2f}" in note
for size, case in [('256x192','03-256x192-v2-fps0'),('384x288','01-384x288-v2-fps0'),('512x384','06-512x384-v2-fps0')]:
    with Image.open(V/A/f'replay-{size}.png') as png, Image.open(D/case/'capture/print.bmp') as bmp:
        assert png.size == tuple(map(int,size.split('x')))
        assert png.convert('RGB').tobytes() == bmp.convert('RGB').tobytes()
for path in (V/A).glob('*.json'): json.loads(path.read_text())
ET.parse(V/A/'scaling.svg')
links = re.findall(r'!?\[[^\]]*\]\(([^)]+)\)',note)
relative = [link for link in links if not re.match(r'https?://',link)]
for link in relative: assert ((V/NOTE).parent/link.split('#')[0]).exists(),link
tracked = subprocess.check_output(['git','-C',str(V),'ls-files','-z','*.md'],text=True).split('\0')
names = {Path(name).stem for name in tracked}
wikilinks = re.findall(r'\[\[([^]|]+)(?:\|[^]]*)?\]\]',note+'\n'+moc)
for link in wikilinks: assert Path(link.split('#')[0]).name in names,link
assert '{{FRAME_TABLE}}' not in note and '{{COUNTER_TABLE}}' not in note
prose = re.sub(r'```.*?```','',note,flags=re.S)
prose = '\n'.join(line for line in prose.splitlines() if not line.startswith('|'))
result = {'note':str(NOTE), 'note_sha256':digest(V/NOTE), 'historical_body_unchanged':True,
          'historical_assets_unchanged':len(old_files), 'new_assets':len(list((V/A).iterdir())),
          'relative_links_valid':len(relative), 'wikilinks_valid':len(wikilinks),
          'mermaid_diagrams':note.count('```mermaid'), 'approx_words_excluding_code_tables':len(prose.split()),
          'measurement_cases':8,'postwarmup_frames':4320,'counter_intervals':674,
          'new_hardware_experiment':False,'scope':'Publication validation; measurement limitations remain in prose.'}
(P/'validation.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
