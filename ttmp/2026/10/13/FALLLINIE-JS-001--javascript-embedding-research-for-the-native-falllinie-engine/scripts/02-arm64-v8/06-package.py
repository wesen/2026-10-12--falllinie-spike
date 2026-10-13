#!/usr/bin/env python3
"""Per-engine private artifacts and readelf dependency closure; NEVER execute ARM."""
import hashlib,json,pathlib,re,shutil,subprocess,tarfile
p=pathlib.Path(__file__).resolve().parent;w=p/'.work';root=w/'debian-arm64';out=w/'packages';out.mkdir(exist_ok=True)
def needed(file):
 text=subprocess.check_output(['readelf','-d',str(file)],text=True)
 return re.findall(r'\(NEEDED\).*\[(.*?)\]',text)
def version_requirements(text):
 requirements={};library=None
 for line in text.split('Version needs section')[-1].splitlines():
  match=re.search(r'File: (\S+)',line)
  if match: library=match[1];requirements[library]=[]
  match=re.search(r'Name: (\S+)',line)
  if match and library: requirements[library].append(match[1])
 return requirements
system={'libc.so.6','libm.so.6','ld-linux-aarch64.so.1'}
lookup={file.name:file for file in root.rglob('*.so*') if file.is_file()}
report={'target_execution':'PENDING: cross-linked, NEVER run by worker','system_dependencies':sorted(system),'engines':{}}
for engine in ['bellard','ng','v8']:
 dest=out/engine
 if dest.exists(): shutil.rmtree(dest) # private local generated package only
 dest.mkdir();(dest/'lib').mkdir();(dest/'licenses').mkdir()
 exe=w/'arm64'/f'{engine}-probe';shutil.copy2(exe,dest/'probe')
 pending=needed(exe);bundled=set();audit=[]
 while pending:
  soname=pending.pop()
  if soname in system or soname in bundled: continue
  assert soname in lookup, f'unresolved {engine}: {soname}'
  source=lookup[soname].resolve();shutil.copy2(source,dest/'lib'/soname);bundled.add(soname)
  pending.extend(needed(source))
 for file in [dest/'probe',*sorted((dest/'lib').iterdir())]:
  head=subprocess.check_output(['readelf','-h',str(file)],text=True);assert 'AArch64' in head
  versions=subprocess.check_output(['readelf','--version-info',str(file)],text=True)
  glibc=sorted(set(re.findall(r'Name: (GLIBC_\d+\.\d+)',versions)))
  assert all(tuple(map(int,v[6:].split('.')))<=(2,44) for v in glibc),(file,glibc)
  required=version_requirements(versions)
  for library,names in required.items():
   if library in system: continue # actual system loader/libc acceptance is a device gate
   provider=dest/'lib'/library;assert provider.exists(),(file,library)
   provider_info=subprocess.check_output(['readelf','--version-info',str(provider)],text=True).split('Version needs section')[0]
   definitions=set(re.findall(r'Name: (\S+)',provider_info))
   assert set(names)<=definitions,(file,library,'missing versions',set(names)-definitions)
  audit.append({'file':str(file.relative_to(dest)),'needed':needed(file),'glibc_requirements':glibc,'version_requirements':required})
 for name in ['pure.js','probe.js','node-oracle.json','workload-expected.json','fixture-sha256.txt']:shutil.copy2(w/name,dest/name)
 shutil.copy2(p/'run-target.sh',dest/'run-target.sh');shutil.copy2(p/'debian-pins.json',dest/'debian-pins.json')
 # Notices remain in private deliverable; no third-party source tree is committed.
 for directory in (root/'usr/share/doc').iterdir():
  copyright=directory/'copyright'
  if copyright.exists():shutil.copy2(copyright,dest/'licenses'/f'{directory.name}-copyright')
 q=w/('quickjs-ng' if engine=='ng' else 'quickjs-2026-06-04')
 if engine!='v8':
  license_file=next(q.glob('LICENSE*'));shutil.copy2(license_file,dest/'licenses'/f'{engine}-LICENSE')
 source_hashes={name:hashlib.sha256((p/name).read_bytes()).hexdigest() for name in ['probe-common.hpp','quickjs-probe.cpp','v8-probe.cpp','workloads.js','01-extract.cjs']}
 manifest={'engine':engine,'harness_source_sha256':source_hashes,'target_execution':'PENDING','v8_provenance':'raw V8 C++ API via Debian libnode108 Node18.20.4+dfsg-1~deb12u2; NOT latest standalone V8','dependency_audit':audit,'system_loader':'/lib/ld-linux-aarch64.so.1','bundle_never_contains_libc_or_loader':True}
 (dest/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
 checks=[]
 for file in sorted(dest.rglob('*')):
  if file.is_file():checks.append(hashlib.sha256(file.read_bytes()).hexdigest()+'  '+str(file.relative_to(dest)))
 (dest/'SHA256SUMS').write_text('\n'.join(checks)+'\n')
 archive=out/f'{engine}-arm64.tar.gz'
 with tarfile.open(archive,'w:gz') as tar:tar.add(dest,arcname=engine)
 report['engines'][engine]={'archive':str(archive.relative_to(p)),'sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'archive_bytes':archive.stat().st_size,'dependency_audit':audit}
(w/'package-audit.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({e:{k:v for k,v in d.items() if k!='dependency_audit'} for e,d in report['engines'].items()},indent=2))
print('PACKAGE PASS; readelf closure resolved; ARM execution PENDING')
