#!/usr/bin/env python3
"""Copy only new report assets; no measurements, network, staging or push."""
import csv,hashlib,json,shutil
from collections import defaultdict
from pathlib import Path
from PIL import Image
P=Path(__file__).resolve().parent
T=P.parent.parent
R=next(x for x in P.parents if (x/'src/renderer.cpp').exists())
V=Path('/home/manuel/code/wesen/go-go-golems/go-go-parc')
A=V/'Projects/2026/10/13/_assets/falllinie-gpu-shader-investigation'
A.mkdir(parents=True,exist_ok=False)
G=T/'scripts/03-mali-counters/evidence'
S=T/'scripts/04-shader-analysis/evidence'
manifest={'native_revision':'70c164a18ee10a6c321c767ea00e64e3faa8dc7e','inputs':{},'assets':{}}
def copy(src,name):
    shutil.copyfile(src,A/name)
    manifest['inputs'][str(src.relative_to(R))]=hashlib.sha256(src.read_bytes()).hexdigest()
for src,name in [(G/'counter-comparison.svg','01-hardware-counters.svg'),(G/'frame-comparison.svg','02-post-bypass-frames.svg'),(S/'compiler-comparison.svg','03-compiler-tradeoffs.svg'),(S/'frame-abba.svg','04-candidate-abba.svg'),(G/'summary.json','hardware-summary.json'),(S/'experiment-summary.json','shader-summary.json'),(G/'analysis-provenance.json','hardware-provenance.json'),(S/'experiment-provenance.json','shader-provenance.json')]:copy(src,name)
for variant in ['baseline','combined']:
    src=S/f'target-20261014T032113Z/capture-{variant}/print.bmp'
    Image.open(src).convert('RGB').save(A/f'fixture-{variant}.png')
    manifest['inputs'][str(src.relative_to(R))]=hashlib.sha256(src.read_bytes()).hexdigest()
# One real central hardware interval, not a synthetic worked example.
case=G/'target-20261014T024712Z/03-live'
summary=json.loads((G/'summary.json').read_text())['03-live']
byseq=defaultdict(list)
for row in csv.DictReader((case/'counters.csv').open()):byseq[int(row['sequence'])].append(row)
prev=None
for seq,rr in sorted(byseq.items()):
    end=int(rr[0]['complete_ns'])
    if prev is not None and prev>=summary['window_mono_ns'][0]:
        example={'case':'03-live','sequence':seq,'previous_host_complete_ns':prev,'host_request_ns':int(rr[0]['request_ns']),'host_complete_ns':end,'backend_begin_ns':int(rr[0]['counter_timestamp_ns']),'host_interval_ns':end-prev,'counter_values':{r['counter']:float(r['value']) if r['status']=='valid' else None for r in rr},'frequency_snapshot_hz':int(rr[0]['gpu_freq_snapshot_hz'])}
        break
    prev=end
(A/'actual-counter-example.json').write_text(json.dumps(example,indent=2)+'\n')
manifest['inputs'][str((case/'counters.csv').relative_to(R))]=hashlib.sha256((case/'counters.csv').read_bytes()).hexdigest()
for path in A.iterdir():manifest['assets'][path.name]=hashlib.sha256(path.read_bytes()).hexdigest()
(A/'report-provenance.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps(example,indent=2))
