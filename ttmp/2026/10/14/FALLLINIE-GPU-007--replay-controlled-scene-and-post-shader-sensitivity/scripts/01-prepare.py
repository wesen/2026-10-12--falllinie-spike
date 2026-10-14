#!/usr/bin/env python3
"""Create explicit non-parity shader packages without changing canonical assets."""
import hashlib
import json
import shutil
import subprocess
from pathlib import Path
P = Path(__file__).resolve().parent
R = Path(subprocess.check_output(['git','-C',str(P),'rev-parse','--show-toplevel'],text=True).strip())
I = P/'inputs'; A = I/'assets'; A.mkdir(parents=True,exist_ok=True)
source = R/'assets/shaders'
variants = ['baseline','post-copy','scene-flat','both']
manifest = {'runtime_evidence': '6c5b231', 'binary_sha256': hashlib.sha256((R/'build-device/falllinie-gpu006').read_bytes()).hexdigest(), 'variants':{},'assets':{}}
assert manifest['binary_sha256'] == '90f124a7745a37f76175130bd79ef748b741852bccfe58305548f7946c0d82ed'
for variant in variants:
    out = A/variant;out.mkdir(exist_ok=True)
    for path in source.glob('*.glsl'): shutil.copyfile(path,out/path.name)
    post = variant in ['post-copy','both'];scene = variant in ['scene-flat','both']
    if post:
        path=out/'post_fs.glsl';text=path.read_text();assert text.count('void main(){')==1
        text=text.replace('void main(){','void main(){\n  // NON-PARITY DIAGNOSTIC: retain FBO/draw, replace print processing.\n  o=vec4(vec3(texture(uD,gl_FragCoord.xy/uRes).r>=0.99999 ? 1. : texture(uC,gl_FragCoord.xy/uRes).r),1.); return;\n')
        path.write_text(text)
    if scene:
        path=out/'scene_fs.glsl';text=path.read_text();assert text.count('void main(){')==1
        text=text.replace('void main(){','void main(){\n  // NON-PARITY DIAGNOSTIC: keep material encoding and snow coverage.\n  if(uKind==2 && dot(vU,vU)>1.) discard;\n  o=vec4(0.8,0.,uMat/255.,0.); return;\n')
        path.write_text(text)
    manifest['variants'][variant]={'non_parity':post or scene,'cheap_post':post,'flat_scene':scene}
for filename in ['hud-font.bmp','glyphs.bmp']: shutil.copyfile(R/'assets'/filename,A/filename)
for path in sorted(A.rglob('*')):
    if path.is_file(): manifest['assets'][str(path.relative_to(P))]=hashlib.sha256(path.read_bytes()).hexdigest()
(I/'manifest.json').write_text(json.dumps(manifest,indent=2,sort_keys=True)+'\n')
(I/'manifest.sha256').write_text(''.join(f'{h}  {name}\n' for name,h in manifest['assets'].items()))
print('Generated',len(manifest['assets']),'assets; canonical shaders unchanged')
