#!/usr/bin/env python3
from pathlib import Path
import hashlib,json,shutil,html
from PIL import Image
P=Path(__file__).resolve().parent
A=Path('/home/manuel/code/wesen/go-go-golems/go-go-parc/Projects/2026/10/14/_assets/falllinie-cpu-pmu/range-draw')
A.mkdir(parents=True,exist_ok=False)
E=P.parent/'13-index-ranges/evidence';F=P.parent/'11-cpu-attribution/evidence'
for src,name in [(E/'summary.json','range-summary.json'),(E/'provenance.json','range-provenance.json'),(F/'summary.json','attribution-summary.json'),(F/'provenance.json','attribution-provenance.json'),(E/'build/qualification.json','build-qualification.json')]:shutil.copyfile(src,A/name)
r=json.loads((E/'summary.json').read_text());f=json.loads((F/'summary.json').read_text())
def start(title,sub,height):return [f'<svg xmlns="http://www.w3.org/2000/svg" width="1080" height="{height}"><rect width="1080" height="{height}" fill="#faf9f6"/><g font-family="sans-serif" fill="#172a3a">',f'<text x="24" y="38" font-size="24">{html.escape(title)}</text>',f'<text x="24" y="65" font-size="14">{html.escape(sub)}</text>']
svg=start('Different CPU events emphasize different executing code','9,895 samples; zero reported lost. Separate runs and event populations, not comparable time fractions.',390)
for i,(name,v) in enumerate(f['cases'].items()):
 y=110+i*45;weight=v['dso_weight_percent']['libmali.so.1.10.0']
 svg.append(f'<text x="24" y="{y+17}" font-size="15">{name}</text><rect x="300" y="{y}" width="{weight*8}" height="24" fill="#277da8"/><text x="{310+weight*8}" y="{y+17}" font-size="15">{weight:.2f}% driver</text>')
svg.append('<text x="24" y="360" font-size="14">Cycle hotspot at 0x42d6dc: 16.84% opening / 15.56% closing; other reports used a 0.5% symbol cutoff.</text></g></svg>');(A/'04-event-attribution.svg').write_text('\n'.join(svg))
svg=start('Known index bounds reduce scene CPU, not demonstrated frame time','Same binary, ordinary/range/range/ordinary order; shaders and index data unchanged; audio disabled.',410)
head=['Case','Top scene CPU','Top output wall','Frame mean','Helper weight']
for x,t in zip([24,225,440,665,855],head):svg.append(f'<text x="{x}" y="105" font-size="16" font-weight="bold">{t}</text>')
for i,(name,v) in enumerate(r['cases'].items()):
 row=[name,f"{v['scopes']['render/0/scene']['cpu_ms']:.3f}ms",f"{v['scopes']['render/0/output']['wall_ms']:.3f}ms",f"{v['frame_mean_ms']:.3f}ms",'no samples' if not v['minmax_helper_sampled_offsets'] else f"{v['minmax_helper_rounded_weight_percent']:.2f}%"]
 for x,t in zip([24,225,440,665,855],row):svg.append(f'<text x="{x}" y="{150+i*45}" font-size="17">{t}</text>')
svg.append('<text x="24" y="355" font-size="14">CPU/wall scope means: 20 selected records per case. Frame mean: whole run after frame 60.</text><text x="24" y="382" font-size="14">Helper absence means zero observed samples in its address range, not a proven zero call count.</text></g></svg>');(A/'05-range-cpu-frame.svg').write_text('\n'.join(svg))
for mode in ['baseline','ranges']:
 src=E/f'target-20261014T154415Z/capture-{mode}/print.bmp';Image.open(src).convert('RGB').save(A/f'fixture-{mode}.png')
manifest={'native_evidence_commit':'44c0c72','implementation_commit':'487492c','assets':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in A.iterdir()},'generator_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
(A/'provenance.json').write_text(json.dumps(manifest,indent=2)+'\n');print(A)
