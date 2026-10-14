#!/usr/bin/env python3
"""Figures from actual qualified GPU007 results; no simulated series."""
import hashlib,json
from pathlib import Path
from PIL import Image,ImageDraw
P=Path(__file__).resolve().parent;D=P/'evidence/target-20261014T191037Z'
s=json.loads((D/'summary.json').read_text());modes=['baseline','post-copy','scene-flat','both']
svg=['<svg xmlns="http://www.w3.org/2000/svg" width="1040" height="550"><rect width="1040" height="550" fill="#faf9f6"/><g font-family="sans-serif" fill="#182a37">','<text x="24" y="35" font-size="23">Scene/post sensitivity: deliberately non-parity shader substitutions</text>','<text x="24" y="64" font-size="14">Same binary, 600-frame replay, dual 512×384, no audio; two cases per mode; no production change.</text>']
for i,(title,key,maximum,scale) in enumerate([('Pooled mean / p95 frame interval','mean_ms',36,1),('Fragment activity, host-window rate','MaliFragActiveCy',900,1e6)]):
 x=200+i*440;svg.append(f'<text x="{x}" y="103" font-size="16">{title}</text>')
 for j,mode in enumerate(modes):
  g=s['groups'][mode];y=136+j*76;value=g[key] if key in g else g['rates_per_second'][key]/scale
  if i==0:svg.append(f'<text x="24" y="{y+20}" font-size="16">{mode}</text>')
  svg.append(f'<rect x="{x}" y="{y}" width="{value/maximum*320:.2f}" height="27" fill="{["#777","#c18133","#2984ad","#8b65aa"][j]}"/>')
  if i==0:
   p95x=x+g['p95_ms']/maximum*320;svg.append(f'<path d="M{p95x} {y-4} v35" stroke="#222" stroke-width="3"/>')
   label=f"mean {value:.3f} ms; p95 {g['p95_ms']:.3f} ms"
  else:label=f'{value:.2f} Mcycles/s'
  svg.append(f'<text x="{x}" y="{y+48}" font-size="13">{label}</text>')
svg+=['<text x="24" y="477" font-size="14">Frame savings are not independent pass costs. Image content, linking and driver work can change.</text>','<text x="24" y="505" font-size="14">Combined substitution reduces activity without much further mean-time gain; p95 is not monotonic.</text>','<text x="24" y="530" font-size="13">Whole-device counters include compositor work and use approximate host intervals, not calibrated utilization.</text></g></svg>']
(D/'sensitivity.svg').write_text('\n'.join(svg)+'\n')
panel=Image.new('RGB',(1024,824),'white');draw=ImageDraw.Draw(panel)
for i,mode in enumerate(modes):
 case=s['groups'][mode]['runs'][0];x=(i%2)*512;y=(i//2)*412
 draw.text((x+10,y+8),mode+(' (original)' if mode=='baseline' else ' (NON-PARITY)'),fill='black')
 with Image.open(D/case/'capture/print.bmp') as image:panel.paste(image.convert('RGB'),(x,y+28))
panel.save(D/'endpoint-comparison.png')
(D/'figure-provenance.json').write_text(json.dumps({'summary_sha256':hashlib.sha256((D/'summary.json').read_bytes()).hexdigest(),'script_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'image_scope':'Unscaled 512x384 internal print targets after uninstrumented endpoint redraw; not HUD/scanout'},indent=2)+'\n')
print(D/'sensitivity.svg')
