#!/usr/bin/env python3
"""Analyze a completed GPU006 matrix. Never infer success from missing cases."""
import argparse
import csv
import hashlib
import json
import math
import re
import statistics
from collections import defaultdict
from pathlib import Path

P = Path(__file__).resolve().parent
EXPECTED = ['01-384x288-v2-fps0', '02-256x192-v1-fps0',
            '03-256x192-v2-fps0', '04-384x288-v1-fps0',
            '05-512x384-v1-fps0', '06-512x384-v2-fps0',
            '07-384x288-v2-fps0', '08-384x288-v2-fps60']

def quantile(values, fraction):
    values = sorted(values)
    x = (len(values)-1)*fraction
    return values[int(x)] + (values[math.ceil(x)]-values[int(x)])*(x-int(x))

def analyze(root):
    hashes = {}
    def read(path):
        data = path.read_bytes()
        hashes[str(path.relative_to(root))] = hashlib.sha256(data).hexdigest()
        return data.decode()
    def rows(path):
        return list(csv.DictReader(read(path).splitlines()))
    if 'complete=true' not in read(root/'campaign.txt'):
        raise ValueError('Campaign is incomplete; no scaling result can be inferred')
    assert read(root/'local-binary.sha256').split()[0] == read(root/'deployed.sha256').split()[0]
    assert sorted(p.name for p in root.glob('[0-9][0-9]-*')) == EXPECTED
    result = {'cases': {}, 'interpretation': 'Whole-device counter rates on host-poll windows; loop throughput is not scanout FPS.'}
    signatures = set()
    images = defaultdict(list)
    for name in EXPECTED:
        case = root/name
        for filename in ['argv.txt', 'game.log', 'conditions-before.txt', 'conditions-after.txt']:
            read(case/filename)
        assert read(case/'status.txt').splitlines() == ['game_status=0', 'probe_status=0']
        r = json.loads(read(case/'receipt.json'))
        assert r['benchmark_replay'] and r['audio_disabled'] and not r['draw_ranges']
        assert r['replay_work_units'] == 32 and abs(r['replay_step_seconds']-1/60) < 1e-7
        assert r['frames'] == 600 and r['gl_error'] == 0 and r['placement_ok']
        assert r['measurement']['gpu_mode'] == 'off' and not r['measurement']['post_bypass']
        assert r['seed'] == 42 and r['debug_view'] == 0 and r['palette'] == 0
        signatures.add(r['replay_signature'])
        frame_rows = rows(case/'frames.csv')
        assert [int(f['frame']) for f in frame_rows] == list(range(600))
        values = [float(f['interval_ms']) for f in frame_rows if int(f['frame']) >= 60]
        assert all(math.isfinite(v) and v > 0 for v in values)
        pairs = [json.loads(line) for line in read(case/'frames.csv.paired.jsonl').splitlines()]
        samples = [p for p in pairs if p['record'] == 'sample']
        final = pairs[-1]
        assert final['attempted'] == final['written'] == len(samples)
        assert all(final[k] == 0 for k in ['active', 'dropped', 'invalid_cpu', 'invalid_wall'])
        assert all(p['pid'] == p['tid'] == final['pid'] for p in samples)
        samples = [p for p in samples if p['frame'] >= 60]
        start = min(p['wall_start_ns'] for p in samples)
        end = max(p['wall_end_ns'] for p in samples)
        scopes = defaultdict(list)
        for p in samples:
            scopes[f"{p['stage']}/{p['view']}/{p['pass']}"].append(
                ((p['wall_end_ns']-p['wall_start_ns'])/1e6,
                 (p['cpu_end_ns']-p['cpu_start_ns'])/1e6))
        item = {'render_size': r['render_size'], 'dual_screen': r['dual_screen'],
                'frames': len(values), 'mean_ms': statistics.mean(values),
                'p50_ms': quantile(values, .5), 'p95_ms': quantile(values, .95),
                'p99_ms': quantile(values, .99), 'loop_frames_per_second': 1000/statistics.mean(values),
                'replay_signature': r['replay_signature'], 'rss_kib': r['rss_kib'],
                'scopes': {k: {'n': len(v), 'wall_ms': statistics.mean(x[0] for x in v),
                              'cpu_ms': statistics.mean(x[1] for x in v)} for k, v in scopes.items()},
                'counter_window_mono_ns': [start, end]}
        counter_rows = rows(case/'counters.csv')
        log = read(case/'counters.log')
        match = re.search(r'final rows=(\d+) errors=(\d+) nonfinite=(\d+) stop=(\S+)', log)
        assert match and int(match[1]) == len(counter_rows) and match[2] == '0' and match[4] == 'ok'
        assert int(match[3]) == sum(row['status'] == 'nonfinite' for row in counter_rows)
        seqs = defaultdict(list)
        for row in counter_rows:
            seqs[int(row['sequence'])].append(row)
        assert sorted(seqs) == list(range(len(seqs)))
        names = {row['counter'] for row in seqs[0]}
        assert len(names) == 13
        previous = None
        durations, selected = [], []
        offsets = []
        for seq, group in sorted(seqs.items()):
            assert len(group) == 13 and {row['counter'] for row in group} == names
            complete = int(group[0]['complete_ns'])
            offsets.append((int(group[0]['counter_timestamp_ns'])-complete)/1e9)
            if previous is not None:
                assert complete > previous
                if previous >= start and complete <= end:
                    durations.append((complete-previous)/1e9)
                    selected.append(group)
            previous = complete
        assert durations, 'No fully contained host counter intervals'
        series = defaultdict(list)
        for group in selected:
            for row in group:
                if row['status'] == 'valid':
                    v = float(row['value']); assert math.isfinite(v)
                    series[row['counter']].append(v)
        rates = {k: sum(series[k])/sum(durations) if len(series[k]) == len(durations) else None
                 for k in names if k.endswith(('Cy', 'By'))}
        item.update({'counter_intervals': len(durations), 'counter_host_seconds': sum(durations),
                     'raw_nonfinite_rows': int(match[3]),
                     'selected_nonfinite_rows': sum(row['status'] == 'nonfinite' for g in selected for row in g),
                     'rates_per_second': rates,
                     'backend_minus_host_seconds': [min(offsets), max(offsets)],
                     'gpu_frequency_snapshots_hz': sorted({int(g[0]['gpu_freq_snapshot_hz']) for g in selected})})
        for kind in ['scene', 'depth', 'print']:
            path = case/'capture'/f'{kind}.bmp'
            digest = hashlib.sha256(path.read_bytes()).hexdigest()
            hashes[str(path.relative_to(root))] = digest
            images[(tuple(r['render_size']), kind)].append(digest)
        result['cases'][name] = item
    assert len(signatures) == 1, 'Different tracked trajectories: do not treat as equivalent workload'
    assert all(len(set(v)) == 1 for v in images.values()), 'Same-resolution endpoint image mismatch'
    result['same_resolution_endpoint_images_match'] = True
    result['signature'] = signatures.pop()
    (root/'summary.json').write_text(json.dumps(result, indent=2)+'\n')
    (root/'analysis-provenance.json').write_text(json.dumps({
        'script_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), 'inputs': hashes,
        'limits': ['Partial selected-state signature, not every game field',
                   'Frames >=60; counter window is the smaller sampled paired-clock extent',
                   'No exact backend-clock join, app-only attribution, pass GPU timing or calibrated utilization',
                   'No full observer-overhead qualification; single matrix with reference bracket']}, indent=2)+'\n')
    for name, value in result['cases'].items():
        print(name, 'mean_ms', round(value['mean_ms'], 4), 'p95_ms', round(value['p95_ms'], 4),
              'loop_fps', round(value['loop_frames_per_second'], 2), 'rates/s', value['rates_per_second'])

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('campaign', type=Path)
    args = parser.parse_args()
    analyze(args.campaign.resolve())
