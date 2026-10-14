#!/usr/bin/env python3
"""Build side-by-side review assets from a PERF-010 campaign.
Usage: 08-review-assets.py CAMPAIGN_DIR OUT_DIR

For every checkpoint and both windows: the baseline (PERF-009 defaults) and each
LOD setting as full 1024x768 window captures, a difference overlay (baseline dimmed,
changed pixels in red) and an enlarged crop of the 256x192 window with the most
changed pixels. Writes review.json with pixel statistics and the campaign timing.
"""
import json,sys
from pathlib import Path
import numpy as np
from PIL import Image
root=Path(sys.argv[1]);out=Path(sys.argv[2]);out.mkdir(parents=True,exist_ok=True)
MODES=['lod120','lod60'];VIEWS={'top':'output.png','lower':'lower/output.png'}
keys=sorted({p.name[len('visual-'):].rsplit('-',1)[0] for p in root.glob('visual-*-baseline')})
review={'checkpoints':[],'timing':{}}
def load(p):return np.asarray(Image.open(p).convert('RGB'),dtype=np.int16)
def best_window(mask,w=256,h=192):
 ii=np.pad(mask.astype(np.int32).cumsum(0).cumsum(1),((1,0),(1,0)))
 H,W=mask.shape;best=(-1,0,0)
 for y in range(0,H-h+1,16):
  for x in range(0,W-w+1,16):
   s=ii[y+h,x+w]-ii[y,x+w]-ii[y+h,x]+ii[y,x]
   if s>best[0]:best=(int(s),x,y)
 return best
for key in keys:
 entry={'key':key,'views':{}}
 for view,rel in VIEWS.items():
  base=load(root/f'visual-{key}-baseline'/'capture'/rel)
  Image.fromarray(base.astype(np.uint8)).save(out/f'{key}-{view}-baseline.png',optimize=True)
  v={}
  for mode in MODES:
   img=load(root/f'visual-{key}-{mode}'/'capture'/rel)
   Image.fromarray(img.astype(np.uint8)).save(out/f'{key}-{view}-{mode}.png',optimize=True)
   delta=np.abs(img-base).max(axis=2);mask=delta>0
   dim=(base*0.35+255*0.65).astype(np.uint8);overlay=dim.copy();overlay[mask]=[220,30,40]
   Image.fromarray(overlay).save(out/f'{key}-{view}-{mode}-diff.png',optimize=True)
   count,x,y=best_window(mask)
   crop=lambda a:Image.fromarray(a[y:y+192,x:x+256].astype(np.uint8)).resize((512,384),Image.NEAREST)
   strip=Image.new('RGB',(512*3+16,384),(255,255,255))
   strip.paste(crop(base),(0,0));strip.paste(crop(img),(520,0));strip.paste(crop(overlay),(1040,0))
   strip.save(out/f'{key}-{view}-{mode}-crop.png',optimize=True)
   v[mode]={'different_pixels':int(mask.sum()),'pixels':int(mask.size),'percent':round(100*float(mask.mean()),4),'max_channel_delta':int(delta.max()),'crop':[x,y,256,192],'crop_changed':count}
  entry['views'][view]=v
 review['checkpoints'].append(entry)
summary=json.loads((root/'summary.json').read_text())
for g,p in summary['pooled'].items():
 f=p['frames'];review['timing'][g]={k:round(f[k],4) for k in ['mean_ms','p95_ms','p99_ms','long_share','missed_refreshes_per_frame']}
 review['timing'][g]['per_frame']={k:round(v,1) for k,v in p['per_frame_mean'].items()}
review['receipt_draws']={n:c['receipt']['geometry_draws'] for n,c in summary['cases'].items()}
(out/'review.json').write_text(json.dumps(review,indent=1)+'\n');print(out,len(keys),'checkpoints')
