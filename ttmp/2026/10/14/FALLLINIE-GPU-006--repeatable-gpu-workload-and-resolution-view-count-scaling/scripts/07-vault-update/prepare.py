#!/usr/bin/env python3
"""Publish a generated additive checkpoint; preserve historical GPU report bytes."""
import hashlib
import json
import shutil
import subprocess
from pathlib import Path
from PIL import Image
P = Path(__file__).resolve().parent
T = P.parent.parent
R = Path(subprocess.check_output(['git', '-C', str(P), 'rev-parse', '--show-toplevel'], text=True).strip())
V = Path('/home/manuel/code/wesen/go-go-golems/go-go-parc')
NOTE = Path('Projects/2026/10/13/PROJECT REPORT - Falllinie GPU Profiling - Hardware Counters, Shader Compilation and a Rejected Optimization.md')
A = NOTE.parent/'_assets/falllinie-gpu-shader-investigation/replay-scaling'
D = T/'scripts/evidence/target-20261014T180649Z'
BASE = 'd16dc38ce09b79b4b55e95f16cf772a1f2a98afd'

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def historical(text):
    return text.split('## What this investigation established\n', 1)[1].split('\n\n## 10. Follow-up:', 1)[0].split('\n\n## Falllinie optimization report network', 1)[0]

def main():
    (V/A).mkdir(parents=True, exist_ok=True)
    sources = {}
    def copy(source, target):
        sources[str(source.relative_to(R))] = digest(source)
        shutil.copyfile(source, V/A/target)
    copy(D/'summary.json', 'summary.json')
    copy(D/'analysis-provenance.json', 'analysis-provenance.json')
    copy(D/'scaling.svg', 'scaling.svg')
    copy(D/'figure-provenance.json', 'figure-provenance.json')
    copy(T/'scripts/evidence/build/qualification.json', 'build-qualification.json')
    for size, case in [('256x192', '03-256x192-v2-fps0'), ('384x288', '01-384x288-v2-fps0'), ('512x384', '06-512x384-v2-fps0')]:
        source = D/case/'capture/print.bmp'
        sources[str(source.relative_to(R))] = digest(source)
        with Image.open(source) as image:
            image.convert('RGB').save(V/A/f'replay-{size}.png')
        copy(D/case/'receipt.json', f'receipt-{size}.json')
    docs = [T/'design-doc/01-intern-guide-to-repeatable-gpu-benchmarking-architecture-and-implementation.md',
            T/'reference/01-gpu-benchmark-implementation-diary.md',
            T/'analysis/01-measured-replay-resolution-and-view-count-scaling-on-mali-g52.md']
    perf = R/'ttmp/2026/10/13/FALLLINIE-PERF-004--gl-pass-measurement-and-evidence-driven-native-optimization'
    docs += list((perf/'design-doc').glob('*.md'))
    docs += [perf/'reference/04-mali-counter-investigation-diary.md', perf/'reference/05-post-shader-compiler-and-experiment-diary.md']
    docs += [R/'src'/name for name in ['main.cpp', 'runtime.cpp', 'terrain.cpp', 'renderer.cpp']]
    for path in docs:
        sources[str(path.relative_to(R))] = digest(path)
    s = json.loads((D/'summary.json').read_text())
    frames = ['| Case | Size | Views | FPS cap | Mean ms | p95 ms | Loop frames/s |', '|---|---|---:|---:|---:|---:|---:|']
    counters = ['| Case | GPU active Mcycles/s | Fragment active Mcycles/s | External reads MB/s | External writes MB/s |', '|---|---:|---:|---:|---:|']
    labels = ['Opening reference', 'Small single', 'Small dual', 'Middle single', 'Large single', 'Large dual', 'Closing reference', 'Paced reference']
    for label, (name, value) in zip(labels, s['cases'].items()):
        size = '×'.join(map(str, value['render_size'])); views = 2 if value['dual_screen'] else 1; cap = 60 if name.endswith('fps60') else 0
        frames.append(f"| {label} | {size} | {views} | {cap} | {value['mean_ms']:.4f} | {value['p95_ms']:.4f} | {value['loop_frames_per_second']:.2f} |")
        rates = value['rates_per_second']
        counters.append('| '+label+' | '+' | '.join(f'{rates[k]/1e6:.2f}' for k in ['MaliGPUActiveCy','MaliFragActiveCy','MaliExtBusRdBy','MaliExtBusWrBy'])+' |')
    extra = (P/'report-addendum.txt').read_text().replace('{{FRAME_TABLE}}', '\n'.join(frames)).replace('{{COUNTER_TABLE}}', '\n'.join(counters))
    assert '{{' not in extra
    note = (V/NOTE).read_text()
    marker = '\n\n## Falllinie optimization report network'
    assert note.count(marker) == 1, 'Navigation boundary ambiguous'
    before, after = note.split(marker)
    before = before.split('\n\n## 10. Follow-up:', 1)[0]
    updated = before+'\n\n'+extra+marker+after
    original = subprocess.check_output(['git','-C',str(V),'show',f'{BASE}:{NOTE}'],text=True)
    assert historical(original) == historical(updated), 'Historical body would change'
    (V/NOTE).write_text(updated)
    assets = {str(path.relative_to(V)): digest(path) for path in sorted((V/A).iterdir()) if path.name != 'publication-provenance.json'}
    provenance = {'native_evidence_checkpoint': '6c5b231', 'runtime_commit': '32b8bfd640f44c465d3f8b6311400f35eac07e81',
                  'previous_vault_checkpoint': BASE, 'native_sources': sources, 'vault_assets': assets,
                  'historical_body_sha256': hashlib.sha256(historical(original).encode()).hexdigest(),
                  'note_sha256': digest(V/NOTE), 'generator_sha256': digest(Path(__file__)),
                  'addendum_sha256': digest(P/'report-addendum.txt'),
                  'scope': 'Additive report synthesis; no new hardware run. Historical GPU report body and assets preserved.'}
    (V/A/'publication-provenance.json').write_text(json.dumps(provenance,indent=2,sort_keys=True)+'\n')
    print(V/NOTE)

if __name__ == '__main__':
    main()
