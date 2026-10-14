#!/usr/bin/env python3
import hashlib,json
from pathlib import Path
from PIL import Image,ImageDraw,ImageChops
P=Path(__file__).resolve().parent;D=P/'evidence/target-20261014T194947Z';s=json.loads((D/'summary.json').read_text())
fig=D/'figures';fig.mkdir(exist_ok=True)
panel=Image.new('RGB',(2048,1600),'white');draw=ImageDraw.Draw(panel)
for row,view in enumerate(['','lower/']):
 for col,(case,label) in enumerate([('01-original','Original'),('03-bounds','Bounded shadows: pixel-identical at this checkpoint')]):
  draw.text((col*1024+12,row*800+10),label+(' — top with HUD' if not view else ' — lower first-person'),fill='black')
  with Image.open(D/case/'capture'/view/'output.png') as im:panel.paste(im.convert('RGB'),(col*1024,row*800+32))
panel.save(fig/'full-window-comparison.png')
boxes={}
for view in ['','lower/']:
 ims=[Image.open(D/case/'capture'/view/'output.png').convert('RGB') for case in ['01-original','03-bounds','02-off']]
 bbox=ImageChops.difference(ims[0],ims[2]).getbbox();assert bbox
 box=(max(0,bbox[0]-12),max(0,bbox[1]-12),min(1024,bbox[2]+12),min(768,bbox[3]+12));w=(box[2]-box[0])*3;h=(box[3]-box[1])*3
 detail=Image.new('RGB',(3*w,h+36),'white');label=ImageDraw.Draw(detail)
 for i,(im,title) in enumerate(zip(ims,['Original','Bounds (same pixels)','Shadows OFF (non-parity)'])):
  label.text((i*w+8,10),title,fill='black');detail.paste(im.crop(box).resize((w,h),Image.Resampling.NEAREST),(i*w,36))
 name='lower' if view else 'top';detail.save(fig/(name+'-shadow-detail.png'));boxes[name]={'source_crop':box,'scale':3,'resampling':'nearest; full difference bounding rectangle plus12px margin'}
svg=['<svg xmlns="http://www.w3.org/2000/svg" width="1000" height="430"><rect width="1000" height="430" fill="#faf9f6"/><g font-family="sans-serif" fill="#172c3b">','<text x="24" y="36" font-size="24">Original shadows retained: conservative footprint rejection</text>','<text x="24" y="65" font-size="14">Dual 512×384; same binary/replay; two measured cases per mode; no audio.</text>']
for i,(mode,label,color) in enumerate([('original','Original','#777'),('bounds','Bounds candidate','#287fa5'),('off','Shadow OFF diagnostic','#bd7930')]):
 g=s['groups'][mode];y=105+i*82;x=240;scale=17
 svg.extend([f'<text x="24" y="{y+20}" font-size="16">{label}</text>',f'<rect x="{x}" y="{y}" width="{g["mean_ms"]*scale:.2f}" height="26" fill="{color}"/>',f'<path d="M{x+g["p95_ms"]*scale} {y-4} v34" stroke="#222" stroke-width="3"/>',f'<text x="{x}" y="{y+48}" font-size="14">Mean {g["mean_ms"]:.3f} ms; p95 {g["p95_ms"]:.3f}; p99 {g["p99_ms"]:.3f}</text>'])
svg+=['<text x="24" y="375" font-size="15">Bounds: 12.46% lower mean; 40/40 multi-endpoint image pairs pixel-identical.</text>','<text x="24" y="404" font-size="13">Finite visual qualification, not universal parity, isolated shadow cost, or integrated 60 FPS acceptance.</text></g></svg>']
(fig/'timing.svg').write_text('\n'.join(svg)+'\n')
(fig/'provenance.json').write_text(json.dumps({'summary_sha256':hashlib.sha256((D/'summary.json').read_bytes()).hexdigest(),'generator_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'crops':boxes,'scope':'Actual target full-window backbuffers including applicable HUD; not compositor/physical scanout; no fabricated frames'},indent=2)+'\n')
print(json.dumps(boxes))
