#!/usr/bin/env python3
"""Build 384x288 vs 512x384 review assets from a res512 campaign.
Usage: 09-resolution-assets.py CAMPAIGN_DIR OUT_DIR

Full 1024x768 window captures for both internal sizes at every checkpoint and both
windows, plus 2x nearest-neighbour crops of three fixed regions per window
(left: 384x288, right: 512x384). Writes resolution.json with timing summaries.
"""
import json,sys
from pathlib import Path
from PIL import Image
root=Path(sys.argv[1]);out=Path(sys.argv[2]);out.mkdir(parents=True,exist_ok=True)
VIEWS={'top':'output.png','lower':'lower/output.png'}
REGIONS={'top':{'hud':(0,0,256,192,'HUD text'),'horizon':(400,180,256,192,'Distant terrain and horizon'),'rider':(384,420,256,192,'Rider and near snow')},
         'lower':{'horizon':(384,200,256,192,'Distant terrain and horizon'),'near':(384,480,256,192,'Near terrain'),'edge':(0,300,256,192,'Left edge terrain')}}
keys=sorted({p.name[len('visual-'):].rsplit('-',1)[0] for p in root.glob('visual-*-baseline')})
result={'checkpoints':keys,'regions':{v:{k:list(r) for k,r in rs.items()} for v,rs in REGIONS.items()},'timing':{}}
for key in keys:
 for view,rel in VIEWS.items():
  imgs={'r384':Image.open(root/f'visual-{key}-baseline'/'capture'/rel).convert('RGB'),'r512':Image.open(root/f'visual-{key}-r512'/'capture'/rel).convert('RGB')}
  for name,im in imgs.items():im.save(out/f'{key}-{view}-{name}.png',optimize=True)
  for region,(x,y,w,h,_) in REGIONS[view].items():
   pair=Image.new('RGB',(w*4+16,h*2),(255,255,255))
   for i,name in enumerate(['r384','r512']):pair.paste(imgs[name].crop((x,y,x+w,y+h)).resize((w*2,h*2),Image.NEAREST),(i*(w*2+16),0))
   pair.save(out/f'{key}-{view}-{region}-crop.png',optimize=True)
summary=json.loads((root/'summary.json').read_text())
for g,p in summary['pooled'].items():
 f=p['frames'];pf=p['per_frame_mean'];busy=pf.get('MaliGPUActiveCy',0)*f['loop_fps']/8e8*100
 result['timing'][g]={'mean_ms':round(f['mean_ms'],3),'p95_ms':round(f['p95_ms'],3),'p99_ms':round(f['p99_ms'],3),'long_share':round(f['long_share'],4),'gpu_busy_percent':round(busy,1)}
result['audio_health']={n:c['status'] for n,c in summary['cases'].items()}
(out/'resolution.json').write_text(json.dumps(result,indent=1)+'\n');print(out,len(keys),'checkpoints')
