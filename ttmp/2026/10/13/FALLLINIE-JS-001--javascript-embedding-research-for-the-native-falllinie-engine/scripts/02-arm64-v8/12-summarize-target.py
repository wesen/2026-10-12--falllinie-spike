#!/usr/bin/env python3
"""Summarize actual retained device receipts; does not execute or extrapolate probes."""
import hashlib
import json
import pathlib
import re

root = pathlib.Path(__file__).resolve().parent / 'evidence/target-corrected-v2'
comparison = json.loads((root / 'comparison.json').read_text())
assert comparison['status'] == 'PASS'
summary = {'execution': 'actual parent-invoked AArch64, one sequential run per engine',
           'reference': 'retained host Node/native oracle, not an ARM64 native benchmark',
           'limits': ['fixed workload, not thermal or full-game acceptance',
                      'V8 raw C++ API through pinned libnode108, not latest standalone V8',
                      'automatic GC pause causality unavailable; heap cap is not RSS cap',
                      'cold function calls follow eight lifecycle cycles, not process-cold'],
           'engines': {}}
for engine in ('bellard', 'ng', 'v8'):
    text = (root / f'{engine}.log').read_text()
    preflight = (root / f'{engine}-preflight.log').read_text()
    assert 'target_execution=ACTUAL' in preflight and '\nPROBE PASS\n' in text
    def fields(line):
        return {key: float(value) for key, value in
                re.findall(r'([A-Za-z0-9_]+)=(-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)', line)}
    benches = {line.split()[1]: fields(line) for line in text.splitlines() if line.startswith('bench ')}
    cold = {line.split()[1]: fields(line) for line in text.splitlines() if line.startswith('cold ')}
    item = {'version': next(line[7:] for line in text.splitlines() if line.startswith('engine ')),
            'benches': benches, 'first_calls': cold,
            'lifecycle': [fields(line) for line in text.splitlines() if line.startswith('lifecycle ')],
            'explicit_gc_wall_us': float(re.search(r'^explicit_gc_wall_us=(\S+)', text, re.M)[1]),
            'process': fields(next(line for line in text.splitlines() if line.startswith('process '))),
            'max_sampled_vm_rss_kib': max(map(int, re.findall(r'(?<![A-Za-z_])rss_kib=(\d+)', text))),
            'comparison': comparison['comparison'][engine],
            'source_sha256': {name: hashlib.sha256((root / name).read_bytes()).hexdigest()
                              for name in (f'{engine}.log', f'{engine}-preflight.log', f'{engine}-time.log')}}
    summary['engines'][engine] = item
    print(engine, 'row p50 ms=', benches['batch']['p50_us'] / 1000,
          '128 scoring pairs p50 ms=', benches['sourceRules']['p50_us'] / 1000,
          'reported peak RSS KiB=', item['process']['peak_rss_kib'])
(root / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
