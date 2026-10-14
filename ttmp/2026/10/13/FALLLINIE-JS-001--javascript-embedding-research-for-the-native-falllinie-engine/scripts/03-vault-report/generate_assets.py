#!/usr/bin/env python3
"""Derive portable original report figures from immutable committed receipts."""
import hashlib
import html
import json
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[7]
REV = '22241cab96cf61928bde3eec61c88071e8eda402'
TICKET = 'ttmp/2026/10/13/FALLLINIE-JS-001--javascript-embedding-research-for-the-native-falllinie-engine'
P = TICKET + '/scripts/02-arm64-v8'
VAULT = pathlib.Path('/home/manuel/code/wesen/go-go-golems/go-go-parc')
DATED = VAULT / 'Projects/2026/10/13'
ASSETS = DATED / '_assets/falllinie-js-arm64-22241ca'
ENGINES = ('bellard', 'ng', 'v8')
NAMES = {'bellard': 'Bellard', 'ng': 'QuickJS-ng', 'v8': 'V8 / libnode108'}
COLORS = {'bellard': '#246c9c', 'ng': '#487a35', 'v8': '#975124'}


def source(path):
    return subprocess.check_output(['git', '-C', str(ROOT), 'show', REV + ':' + path])


def text(x, y, label, size=17, color='#182b3c'):
    return f'<text x="{x}" y="{y}" font-size="{size}" fill="{color}">{html.escape(str(label))}</text>'


def rect(x, y, w, h, color='#edf3f8'):
    return f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="6" fill="{color}"/>'


def svg(name, title, desc, parts, height=500):
    out = ('<svg xmlns="http://www.w3.org/2000/svg" width="1100" height="' + str(height) +
           '" viewBox="0 0 1100 ' + str(height) + '" role="img" aria-labelledby="title desc">' +
           '<title id="title">' + html.escape(title) + '</title><desc id="desc">' +
           html.escape(desc) + '</desc><rect width="1100" height="' + str(height) +
           '" fill="#ffffff"/><g font-family="sans-serif">' + text(30, 38, title, 24) +
           ''.join(parts) + '</g></svg>\n')
    (ASSETS / name).write_text(out)


def flow(name, title, rows, footer):
    parts = []
    for j, (label, detail) in enumerate(rows):
        y = 70 + j * 75
        parts += [rect(35, y, 1030, 55), text(55, y + 22, label, 18), text(55, y + 44, detail, 15)]
        if j < len(rows) - 1:
            parts += [text(540, y + 73, '↓', 23)]
    parts += [text(35, 85 + len(rows) * 75, footer, 15)]
    svg(name, title, footer, parts, 115 + len(rows) * 75)


def main():
    ASSETS.mkdir(parents=True, exist_ok=True)
    s = json.loads(source(P + '/evidence/target-corrected-v2/summary.json'))
    flow('ownership.svg', 'Proposed rules ownership — not implemented in the game', [
        ('Native Game / main thread', 'Owns air state, recent history, combo, meter and persistence; captures immutable input.'),
        ('One VM owner', 'Receives copied data; temporary G.recent is reset for each call; returns pts/name/ubers.'),
        ('Native validation and single commit', 'Reject malformed, stale or failed output before effects; native appends accepted history once.'),
        ('Existing native consumers', 'Terrain + World remain collision/render authority; GL main thread; audio receives numeric commands.')],
        'No JS handles cross worker boundaries. No JavaScript, allocation or GC in the audio callback.')
    flow('copy-commit.svg', 'Measured copy protocol — corrected-v2 validation and commit', [
        ('Root exact Float64Array and its backing owner', 'Reject wrong type, empty, detached, oversized; validate offset and byte length without overflow.'),
        ('Borrow bytes only during synchronous copy', 'No JavaScript calls while borrowing; memcpy into candidate[749]; native storage is independent.'),
        ('Validate all elements and accumulated sum', 'Two finite Number.MAX_VALUE values overflow the sum; reject before committing any native slot.'),
        ('Commit complete candidate or preserve previous state', 'Regression checks all 749 slots, count=3 and sum=6 after rejected overflow input.')],
        'Implemented in probe-common.hpp and both native adapters; not a live game snapshot API.')
    flow('evidence-pipeline.svg', 'Measured qualification pipeline — host and actual ARM64 are separate', [
        ('Frozen HTML / source extraction', 'Fixture hash enforced; unchanged selected bodies + explicit adapters; HOST Node oracle retained.'),
        ('Corrected host qualification', 'Three runs per engine; native 12/12; comparator 11/11; bounded adapter sanitizers.'),
        ('Pinned ARM64 cross-link and private packages', 'Matching headers/libraries; ELF + provider versions + checksums; no libc/loader replacement.'),
        ('Actual handheld execution / parent collection', 'One sequential run per engine, 300 blocks; aarch64 A55; glibc 2.44; ondemand governor.'),
        ('Independent comparison of collected logs', 'Each engine: 85 selected leaves, seven checksum workload families, 19 protocol checks.')],
        'Cross-link alone is not execution; selected source equality is not full simulation parity.')
    parts = [text(30, 68, 'Measured on target: warm p50 and first wrapper call (ms); 30 warmups + 120 samples', 17)]
    scale = 24
    for index, workload in enumerate(('batch', 'sourceRules')):
        y = 104 + index * 240
        parts += [text(30, y, 'Row: 107 columns / 749 doubles' if workload == 'batch' else 'Rules: 128 pairs / 256 scoreAir + hash/rng', 19)]
        for j, e in enumerate(ENGINES):
            item = s['engines'][e]
            warm = item['benches'][workload]['p50_us'] / 1000
            first = item['first_calls'][workload]['first_call_us'] / 1000
            yy = y + 35 + j * 60
            parts += [text(30, yy + 14, NAMES[e], 16), rect(215, yy, warm * scale, 17, COLORS[e]),
                      text(835, yy + 14, f'warm {warm:.6f}', 15),
                      rect(215, yy + 22, first * scale, 14, '#b6c2cc'), text(835, yy + 35, f'first {first:.6f}', 15)]
    heavy = s['engines']['v8']['benches']['sustained_block_8row_4rules_4churn']
    heavy_label = ' / '.join(f'{key} {heavy[key + "_us"] / 1000:.6f}' for key in ('p50', 'p99', 'max'))
    parts += [text(30, 585, 'V8 heavy combined block: ' + heavy_label + ' ms', 17),
              text(30, 612, 'First calls follow eight lifecycle cycles. Heavy block is NOT a game frame. No density inferred.', 15)]
    svg('latency.svg', 'ARM64 latency — labeled summary statistics, not invented distributions',
        'Warm p50 versus first wrapper call; gray bars are first calls. One run per engine.', parts, 635)
    parts = [text(30, 72, 'Measured process memory in KiB. Heap constraints do not cap total RSS.', 17)]
    for j, e in enumerate(ENGINES):
        item = s['engines'][e]
        peak = int(item['process']['peak_rss_kib'])
        sampled = item['max_sampled_vm_rss_kib']
        y = 110 + j * 100
        parts += [text(30, y + 17, NAMES[e], 17), rect(220, y, peak / 60, 24, COLORS[e]),
                  text(930, y + 18, f'{peak} peak', 16), rect(220, y + 33, sampled / 60, 20, '#b6c2cc'),
                  text(930, y + 48, f'{sampled} sampled', 15)]
    parts += [text(30, 435, 'Colored: reported getrusage peak. Gray: maximum sampled /proc VmRSS; distinct observations.', 15),
              text(30, 463, 'Cooperative 192 MiB guard + 180 s timeout: finite trusted probe, not a hardened sandbox.', 15)]
    svg('memory.svg', 'ARM64 memory — reported peak and sampled RSS kept separate',
        'Separate observations with different values. No artificial unified RSS estimate.', parts, 490)
    paths = [P + '/' + n for n in ['probe-common.hpp', 'quickjs-probe.cpp', 'v8-probe.cpp', 'workloads.js',
            '01-extract.cjs', '04-compare.py', 'debian-pins.json', '02-build-host.sh', '03-build-arm64.sh',
            '06-package.py', 'run-target.sh', 'evidence/parent-initial-review.txt', 'evidence/parent-corrections-review.txt',
            'evidence/review-corrections-v2/package-audit.json', 'evidence/review-corrections-v2/results-summary.json',
            'evidence/review-corrections-v2/validation.log', 'evidence/target-corrected-v2/comparison.json',
            'evidence/target-corrected-v2/summary.json']]
    paths += [P + '/evidence/target-corrected-v2/' + e + suffix for e in ENGINES
              for suffix in ('.log', '-preflight.log', '-time.log')]
    provenance = {'schema_version': 1, 'source_repository': str(ROOT), 'source_revision': REV,
                  'report_date_local': '2026-10-13', 'ticket': TICKET,
                  'fixture_sha256': hashlib.sha256(source('fixtures/falllinie.html')).hexdigest(),
                  'qualification': s['execution'], 'reference': s['reference'], 'limits': s['limits'],
                  'receipt_sha256': {path: hashlib.sha256(source(path)).hexdigest() for path in paths},
                  'target_metrics': {e: {k: s['engines'][e][k] for k in
                      ('version', 'benches', 'first_calls', 'explicit_gc_wall_us', 'process',
                       'max_sampled_vm_rss_kib', 'comparison')} for e in ENGINES}}
    (ASSETS / 'provenance.json').write_text(json.dumps(provenance, indent=2) + '\n')
    print('GENERATED 5 original SVGs + bounded provenance JSON:', ASSETS)


if __name__ == '__main__':
    main()
