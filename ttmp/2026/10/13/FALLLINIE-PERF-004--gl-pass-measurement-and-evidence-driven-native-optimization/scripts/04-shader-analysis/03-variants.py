#!/usr/bin/env python3
"""Diagnostic asset variants; retain canonical source assets and shader formulas."""
import hashlib,json,shutil,subprocess
from pathlib import Path
P=Path(__file__).resolve().parent
R=next(p for p in P.parents if (p/'src/renderer.cpp').exists())
M=P/'.work/Arm_Performance_Studio_2026.5/mali_offline_compiler/malioc'
original=(R/'assets/shaders/post_fs.glsl').read_text()
adapted=(P/'inputs/runtime/3.frag').read_text()
def lazy(s):
    before='float d=texture(uD,uv).r; vec4 s=texture(uC,uv);'
    assert s.count(before)==1
    s=s.replace(before,'float d=texture(uD,uv).r;')
    return s.replace('float lum=s.r;', 'vec4 s=texture(uC,uv); float lum=s.r;')
def fog(s):
    before='float zc=lin(d), zl='
    assert s.count(before)==1
    s=s.replace(before,'float zc=lin(d); float edge=0.;\n    if(zc<uFogP.y){\n    float zl=')
    s=s.replace('float edge=smoothstep(0.018,0.045,lap);','edge=smoothstep(0.018,0.045,lap);')
    s=s.replace('edge*=1.-smoothstep(uFogP.x,uFogP.y,zc);','edge*=1.-smoothstep(uFogP.x,uFogP.y,zc);\n    }')
    return s
manifest={}
for name,transform in [('baseline',lambda s:s),('lazy-color',lazy),('fog-skip',fog),('combined',lambda s:fog(lazy(s)))]:
    asset=P/'inputs/assets'/name; asset.mkdir(parents=True,exist_ok=True)
    for path in (R/'assets/shaders').glob('*.glsl'): shutil.copyfile(path,asset/path.name)
    (asset/'post_fs.glsl').write_text(transform(original))
    src=P/'inputs'/f'{name}.frag'; src.write_text(transform(adapted))
    manifest[name]={'asset_sha256':hashlib.sha256((asset/'post_fs.glsl').read_bytes()).hexdigest(),'adapted_sha256':hashlib.sha256(src.read_bytes()).hexdigest(),'diagnostic_only':name!='baseline'}
    for form,extension in [('text','txt'),('json','json')]:
        argv=[str(M),'--core','Mali-G52','--opengles','--format',form,str(src)]
        result=subprocess.run(argv,check=True,capture_output=True,text=True)
        (P/'evidence/reports'/f'{name}.{extension}').write_text(result.stdout)
(P/'evidence/variant-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
