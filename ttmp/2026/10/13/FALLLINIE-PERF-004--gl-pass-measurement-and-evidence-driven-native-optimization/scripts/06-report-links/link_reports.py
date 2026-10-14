#!/usr/bin/env python3
"""Add explicitly requested cross-links without rewriting historical report bodies."""
import hashlib,json,re
from pathlib import Path
P=Path(__file__).resolve().parent
V=Path('/home/manuel/code/wesen/go-go-golems/go-go-parc')
H='MOC - Falllinie Optimization Reports'
items=[
('Native GLES Spike','Graphics foundations','Deterministic terrain, encoded scene/print passes and Mali depth-sampler precision; establishes the fidelity constraints for later tuning.','This is the graphics foundation, not the final integrated performance result. Continue with the playable port and integrated game, then the measured rendering and GPU investigations.'),
('Playable Port','Playable port and streaming','Shared dual views, frame pacing and cooperative terrain generation; an earlier, smaller workload.','The earlier operating point applies to this smaller workload. Read the integrated-game report before carrying its frame-rate result forward; later rendering reports qualify the measurement tools.'),
('Integrated Native Game','Integrated workload and precision','Camera-relative projection, source DSP optimizations, telemetry and the initial embedding investigation.','Later buffered-audio and ARM64 JavaScript reports extend this checkpoint. Actual rendering attribution and hardware-counter results are in the rendering and GPU reports.'),
('Buffered Audio','Buffered audio qualification','Producer-owned DSP, SPSC PCM transport, copy-only callbacks and finite target deadline results.','This is implemented audio transport with measured target evidence. The rendering report separates producer CPU work from driver CPU work; the JavaScript report does not integrate JavaScript into this audio callback.'),
('JavaScript Embedding','ARM64 JavaScript experiments','Actual QuickJS, QuickJS-ng and V8 embeddings, ownership contracts and isolated workload measurements.','These are qualified isolated embeddings, not production game integration or demonstrated spare frame budget. Read the integrated workload and rendering results alongside the benchmark numbers.'),
('Tracing','Tracing concepts and investigation plan','Wall versus CPU time, scheduler-state interpretation and proposed perf/ftrace/eBPF investigation.','This is the earlier conceptual/planning checkpoint. The rendering report supplies subsequent actual paired-clock/perf data and kernel capability findings; the GPU report adds real hardware counters. Constructed examples here remain constructed.'),
('Rendering Measurements','Measured CPU and driver attribution','Actual paired clocks and perf samples, driver execution, non-running time and unresolved GPU elapsed-query anomalies.','Continue to GPU Profiling for subsequently implemented counter access and offline compilation. Those tools were future work at this checkpoint; their later availability does not validate the anomalous GL timer values.'),
('GPU Profiling','Hardware counters and rejected shader candidate','Actual G52 counters, exact-input offline compilation, non-parity post bypass and a null-result ABBA shader experiment.','This is the latest GPU experiment in this collection. The candidate stays diagnostic-only: bounded fixture equality did not produce a convincing live speedup. Earlier CPU attribution and shader-fidelity reports explain its constraints.')]
reports=list((V/'Projects').rglob('PROJECT REPORT - Falllinie *.md'))
assert len(reports)==8
resolved=[]
for key,label,description,status in items:
    matches=[p for p in reports if p.name.startswith('PROJECT REPORT - Falllinie '+key+' -')]
    assert len(matches)==1,key
    resolved.append((matches[0],label,description,status))
receipt=[]
for path,label,description,status in resolved:
    old=path.read_text();assert '## Falllinie optimization report network' not in old
    block='\n\n## Falllinie optimization report network\n\n'
    block+=f'Navigation addendum: [[{H}|series index, reading routes and evidence boundaries]]. The original report above remains a historical checkpoint; these cross-links do not revise its measurements.\n\n'
    block+=status+'\n\n'
    for other,title,desc,_ in resolved:
        if other!=path:block+=f'- [[{other.stem}|{title}]] — {desc}\n'
    path.write_text(old+block)
    receipt.append({'path':str(path.relative_to(V)),'before_sha256':hashlib.sha256(old.encode()).hexdigest(),'after_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'original_body_preserved':path.read_text().startswith(old)})
(P/'navigation-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('\n'.join(str(p.relative_to(V)) for p,_,_,_ in resolved))
