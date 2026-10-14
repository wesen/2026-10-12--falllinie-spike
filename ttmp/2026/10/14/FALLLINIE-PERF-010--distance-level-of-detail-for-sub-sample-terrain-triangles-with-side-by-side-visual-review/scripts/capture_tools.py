import hashlib,json
from pathlib import Path
from PIL import Image,ImageChops

def compress(root):
    records=[]
    for p in sorted(Path(root).rglob('*.bmp')):
        with Image.open(p) as im:
            im=im.convert('RGBA');target=p.with_suffix('.png');im.save(target)
            with Image.open(target) as saved:assert saved.convert('RGBA').tobytes()==im.tobytes()
            records.append({'bmp':str(p.relative_to(root)),'bmp_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'png_sha256':hashlib.sha256(target.read_bytes()).hexdigest(),'rgba_sha256':hashlib.sha256(im.tobytes()).hexdigest(),'size':im.size})
        p.unlink()
    if records:(Path(root)/'lossless-conversion.json').write_text(json.dumps(records,indent=2)+'\n')

def compare(a,b):
    result={}
    for p in sorted(Path(a).rglob('*.png')):
        rel=p.relative_to(a)
        with Image.open(p) as ia,Image.open(Path(b)/rel) as ib:
            ia=ia.convert('RGBA');ib=ib.convert('RGBA')
            # Different internal sizes are intentional in resolution comparisons; record, don't compare.
            if ia.size!=ib.size:result[str(rel)]={'size_mismatch':[list(ia.size),list(ib.size)]};continue
            # Count differences across all channels; alpha-only differences included.
            changes=[max(abs(x-y) for x,y in zip(pa,pb)) for pa,pb in zip(ia.getdata(),ib.getdata())]
            result[str(rel)]={'different_pixels':sum(v!=0 for v in changes),'max_channel_delta':max(changes),'pixels':len(changes)}
    return result
