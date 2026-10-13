#!/usr/bin/env python3
"""Validate only the append-only report and intended self-contained figure files."""
from pathlib import Path
import hashlib, json, re, subprocess, xml.etree.ElementTree as ET

VAULT=Path('/home/manuel/code/wesen/go-go-golems/go-go-parc')
NATIVE=Path('/home/manuel/code/wesen/2026-10-12--falllinie-spike')
REL=Path('Projects/2026/10/13/PROJECT REPORT - Falllinie Buffered Audio - PCM Ownership, Callback Deadlines and Measured Producer Scheduling.md')
ASSETS=Path('Projects/2026/10/13/_assets/falllinie-buffered-audio-20261013')
EXPECTED=[str(REL)]+[str(ASSETS/name) for name in ['01-ownership.svg','02-partial-wrap.svg','03-convolution.svg','04-target-timing.svg','05-frame-intervals.svg','provenance.json']]
p=VAULT/REL;raw=p.read_text()
assert raw.startswith('---\n')
front,body=raw[4:].split('\n---\n',1)
for key in ['title:','aliases:','tags:','status:','type: project','created: 2026-10-13','repo:','source_revision: 686872c1d68776924b94dbf9a684eae174c1573e']:
 assert key in front,key
fence=None;fences=0
for lineno,line in enumerate(body.splitlines(),1):
 if line.startswith('```'):
  if fence is None:fence=line[3:];fences+=1
  else:assert line=='```',f'bad closing fence:{lineno}';fence=None
assert fence is None,'unclosed Markdown fence'
assert fences==5,f'expected cpp,pseudocode,bash,2Mermaid fences:{fences}'
plain=re.sub(r'```.*?```','',body,flags=re.S)
words=len(plain.split());assert words>=6000,words
for pattern in [r'\bTODO\b',r'\bTBD\b',r'\bFIXME\b',r'\blorem ipsum\b',r'insert (?:figure|text) here',r'think of .* as',r'like a (?:kitchen|factory|train|traffic cop)',r'analogy']:
 assert not re.search(pattern,body,re.I),pattern
links=re.findall(r'!?\[[^\]]*\]\(([^)]+)\)',body)
assert len(links)==6,links
for link in links:
 target=(p.parent/link).resolve();assert target.is_relative_to(VAULT),link
 assert target.is_file(),link
assert (VAULT/'Projects/2026/10/13/PROJECT REPORT - Falllinie Integrated Native Game - Precision, Real-Time Audio, Developer Telemetry and JavaScript Embedding.md').is_file()
ns='{http://www.w3.org/2000/svg}'
for name in EXPECTED[1:-1]:
 svg=(VAULT/name).read_text();tree=ET.fromstring(svg)
 assert tree.tag==ns+'svg'
 assert tree.find(ns+'title') is not None and tree.find(ns+'desc') is not None
 assert not re.search(r'<(?:script|foreignObject|image)\b|url\(|(?:href|src)=|<!DOCTYPE',svg,re.I),name
 assert tree.attrib['viewBox'].startswith('0 0 1200 ')
meta=json.loads((VAULT/EXPECTED[-1]).read_text())
assert meta['source_revision']=='686872c1d68776924b94dbf9a684eae174c1573e'
for mode in ['direct','buffered']:
 folder=NATIVE/'ttmp/2026/10/13/FALLLINIE-AUDIO-003--buffered-native-audio-worker-and-callback-deadline-qualification/scripts/target'/mode
 for name in ['run.log','frames.csv','frames.csv.gpu.csv']:
  assert hashlib.sha256((folder/name).read_bytes()).hexdigest()==meta['source_artifact_sha256'][mode+'/'+name]
 rec=meta['target_records'][mode]
 assert rec['gpu_rows']==0
 assert rec['csv_rows']==int(rec['frame']['frames'])
 assert rec['csv_measured_rows']==int(rec['frame']['measured'])
a=meta['target_records']['buffered']['audio']
assert int(a['produced'])==int(a['delivered'])+int(a['pcm_depth'])
assert a['starved_callbacks']==a['missing_frames']==a['over_budget']=='0'
assert meta['target_records']['direct']['audio']['over_budget']=='18'
assert a['producer_over_budget']=='16'
# Scope-check both index and worktree; stop rather than stage unrelated state.
staged=subprocess.check_output(['git','diff','--cached','--name-only'],cwd=VAULT,text=True).splitlines()
assert not(set(staged)-set(EXPECTED)),f'unrelated staged paths:{staged}'
status=subprocess.check_output(['git','status','--porcelain=v1','--untracked-files=all'],cwd=VAULT,text=True).splitlines()
for row in status:
 name=row[3:].strip('"')
 assert name in EXPECTED,f'unrelated vault state:{row}'
subprocess.run(['git','diff','--check'],cwd=VAULT,check=True)
subprocess.run(['git','diff','--cached','--check'],cwd=VAULT,check=True)
result={'report':str(REL),'word_count_without_frontmatter_or_fences':words,'word_count_body_total':len(body.split()),'markdown_fence_blocks':fences,'local_asset_links':len(links),'svg_files':5,'gpu_csv_rows':{'direct':0,'buffered':0},'staged_files':staged,'intended_changed_files':EXPECTED,'checks':'PASS: assets/SVG XML/no external references/fences/frontmatter/word count/no placeholders/scope/source hashes/frontier equality'}
print(json.dumps(result,indent=2))
