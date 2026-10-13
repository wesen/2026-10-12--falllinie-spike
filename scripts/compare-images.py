#!/usr/bin/env python3
"""Bounded browser/native print comparison; no bit-identical cross-GPU claim."""
import argparse, json
from PIL import Image

def compare(reference, candidate, crop):
    a = Image.open(reference).convert('RGB'); b = Image.open(candidate).convert('RGB')
    if a.size != b.size: raise ValueError('image dimensions differ')
    if crop: a = a.crop(crop); b = b.crop(crop)
    total = a.width*a.height; absolute = 0; large = 0; intersection = 0; union = 0; ink_a = 0; ink_b = 0
    for p,q in zip(a.getdata(), b.getdata()):
        errors = [abs(x-y) for x,y in zip(p,q)]
        absolute += sum(errors); large += max(errors)>16
        ia = p[0]<100 and p[1]<100 and p[2]>100
        ib = q[0]<100 and q[1]<100 and q[2]>100
        ink_a += ia; ink_b += ib; intersection += ia and ib; union += ia or ib
    return dict(pixels=total,mean_absolute_channel_error=absolute/(total*3),fraction_max_channel_error_over_16=large/total,ink_iou=intersection/union if union else 1,reference_ink_fraction=ink_a/total,candidate_ink_fraction=ink_b/total)

if __name__ == '__main__':
    p=argparse.ArgumentParser();p.add_argument('reference');p.add_argument('candidate');p.add_argument('--legacy');a=p.parse_args()
    im=Image.open(a.reference);terrain=(0,im.height//2,im.width,im.height)
    result={'reference':a.reference,'candidate':a.candidate,'full_image':compare(a.reference,a.candidate,None),'terrain_lower_half':compare(a.reference,a.candidate,terrain),'caveat':'Different ANGLE/Intel and Mali floating-point sin hashes change sky/noise; terrain ROI is a bounded comparison, not whole-game qualification.'}
    if a.legacy: result['legacy_terrain_lower_half']=compare(a.reference,a.legacy,terrain)
    print(json.dumps(result,indent=2))
