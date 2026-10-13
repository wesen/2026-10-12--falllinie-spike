#!/usr/bin/env python3
"""Fixed hashes only; extract privately, never install. No device contact."""
import hashlib,json,pathlib,subprocess,urllib.request
p=pathlib.Path(__file__).resolve().parent; w=p/'.work'; w.mkdir(exist_ok=True)
def fetch(url,dest,sha):
 if not dest.exists():
  with urllib.request.urlopen(url,timeout=90) as r, dest.with_suffix('.partial').open('wb') as f:
   while chunk:=r.read(1024*1024): f.write(chunk)
  dest.with_suffix('.partial').rename(dest)
 assert hashlib.sha256(dest.read_bytes()).hexdigest()==sha, f'hash mismatch: {dest}'
 print('verified',dest.name)
archive=w/'quickjs-2026-06-04.tar.xz'
fetch('https://bellard.org/quickjs/quickjs-2026-06-04.tar.xz',archive,'b376e839b322978313d929fd20663b11ba58b75df5a46c126dd19ea2fa70ad2a')
subprocess.run(['tar','-xJf',str(archive),'-C',str(w)],check=True)
ng=w/'quickjs-ng'
if not ng.exists(): subprocess.run(['git','clone','--depth','1','--branch','v0.17.0','https://github.com/quickjs-ng/quickjs.git',str(ng)],check=True)
assert subprocess.check_output(['git','-C',str(ng),'rev-parse','HEAD'],text=True).strip()=='6d46d07d04041b40f4f49eaa7fdebe44c314c699'
pins=json.loads((p/'debian-pins.json').read_text())
for arch,packages in pins['packages'].items():
 root=w/f'debian-{arch}'; root.mkdir(exist_ok=True)
 for name,d in packages.items():
  dest=w/pathlib.Path(d['Filename']).name
  fetch(pins['origin']+'/'+d['Filename'],dest,d['SHA256'])
  subprocess.run(['dpkg-deb','-x',str(dest),str(root)],check=True)
print('FETCH PASS; pinned private roots; no system installation')
