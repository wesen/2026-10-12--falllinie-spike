#!/usr/bin/env python3
"""Rebuild campaign metrics and SVGs from immutable raw inputs; stdlib only."""
import csv
import hashlib
import html
import importlib.util
import json
import math
import re
from collections import defaultdict
from pathlib import Path

P = Path(__file__).resolve().parent
D = P / 'evidence/target-20261014T020124Z'
OUT = P / 'figures'
OUT.mkdir(exist_ok=True)
WARMUP = 60
COLORS = ['#287bb5', '#e59837', '#458f69', '#ab5f94', '#7e70bc', '#c76657', '#5f8e96', '#757575']
inputs = {}

def text(path):
    raw = path.read_bytes()
    inputs[str(path.relative_to(P))] = hashlib.sha256(raw).hexdigest()
    return raw.decode()

def rows(path):
    return list(csv.DictReader(text(path).splitlines()))

def quantile(a, q):
    a = sorted(a)
    if not a:
        return None
    x = (len(a) - 1) * q
    lo = math.floor(x)
    return a[lo] + (a[math.ceil(x)] - a[lo]) * (x - lo)

def stats(a):
    return {'n': len(a), 'mean': sum(a) / len(a), 'p50': quantile(a, .5), 'p95': quantile(a, .95), 'p99': quantile(a, .99), 'max': max(a)} if a else {'n': 0}

def esc(s):
    return html.escape(str(s))

def start(title, subtitle, height=560):
    return [f'<svg xmlns="http://www.w3.org/2000/svg" width="1120" height="{height}" viewBox="0 0 1120 {height}"><rect width="1120" height="{height}" fill="#faf9f6"/><g font-family="sans-serif" fill="#172a3a"><text x="24" y="34" font-size="24" font-weight="bold">{esc(title)}</text><text x="24" y="61" font-size="14">{esc(subtitle)}</text>']

def finish(name, svg):
    svg.append('</g></svg>')
    (OUT / name).write_text('\n'.join(svg))

def bars(name, title, subtitle, labels, segments, legend, footnote):
    height = max(440, 165 + len(labels) * 46)
    svg = start(title, subtitle, height)
    xmax = max(sum(values) for values in segments) * 1.12 or 1
    left, width = 255, 790
    for i in range(6):
        x = left + width * i / 5
        svg.append(f'<path d="M{x} 90 V{height-70}" stroke="#ddd"/><text x="{x}" y="85" text-anchor="middle" font-size="12">{xmax*i/5:.1f}</text>')
    for i, (label, values) in enumerate(zip(labels, segments)):
        y = 102 + i * 46
        svg.append(f'<text x="245" y="{y+19}" text-anchor="end" font-size="14">{esc(label)}</text>')
        x = left
        for j, value in enumerate(values):
            w = width * value / xmax
            svg.append(f'<rect x="{x}" y="{y}" width="{max(0,w)}" height="28" fill="{COLORS[j]}"/>')
            if w > 45:
                svg.append(f'<text x="{x+w/2}" y="{y+19}" fill="white" text-anchor="middle" font-size="13">{value:.2f}</text>')
            x += w
        svg.append(f'<text x="{x+5}" y="{y+19}" font-size="12">{sum(values):.2f}</text>')
    for j, label in enumerate(legend):
        x = 25 + j * 335
        svg.append(f'<rect x="{x}" y="{height-49}" width="14" height="14" fill="{COLORS[j]}"/><text x="{x+22}" y="{height-37}" font-size="13">{esc(label)}</text>')
    svg.append(f'<text x="24" y="{height-12}" font-size="12">{esc(footnote)}</text>')
    finish(name, svg)

# Use the independent strict sidecar analyzer before interpreting any paired data.
spec = importlib.util.spec_from_file_location('paired', P / 'paired-clocks/analyze.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
summary = {'warmup_frames_excluded': WARMUP, 'cases': {}, 'limits': ['Sequential single runs; uncontrolled ondemand clocks.', 'Paired render scopes nest; do not add parent and children.', 'wall minus thread CPU is non-running estimate, not proven GPU wait.', 'Perf percentages include startup and all workload threads; clocks graphs omit first 60 frames.']}
frame_sets = {}
for case in sorted(D.glob('[0-9][0-9]-*')):
    fr = [r for r in rows(case / 'frames.csv') if int(r['frame']) >= WARMUP]
    frame_sets[case.name] = fr
    result = {'frame_ms': stats([float(r['interval_ms']) for r in fr])}
    result['sampled_frame_ms'] = stats([float(r['interval_ms']) for r in fr if int(r['frame']) % 60 == 0])
    result['other_frame_ms'] = stats([float(r['interval_ms']) for r in fr if int(r['frame']) % 60 != 0])
    result['after_sample_frame_ms'] = stats([float(r['interval_ms']) for r in fr if int(r['frame']) % 60 == 1])
    pair = case / 'frames.csv.paired.jsonl'
    if pair.exists():
        data = [json.loads(x) for x in text(pair).splitlines()]
        validated = module.analyze(pair)
        if not validated['all_attempted_samples_valid']:
            raise ValueError(f'invalid/incomplete paired campaign: {case.name}')
        (case / 'paired-analysis.json').write_text(json.dumps(validated, indent=2)+'\n')
        result['paired_final'] = data[-1]
        groups = defaultdict(list)
        for r in data:
            if r['record'] != 'sample' or r['frame'] < WARMUP:
                continue
            if r['wall_status'] != 'valid' or r['cpu_status'] != 'valid':
                continue
            wall = (r['wall_end_ns'] - r['wall_start_ns']) / 1e6
            cpu = (r['cpu_end_ns'] - r['cpu_start_ns']) / 1e6
            groups[f"{r['stage']}/{r['view']}/{r['pass']}"] .append((wall, cpu, wall-cpu))
        result['paired_groups_ms'] = {k: {'n': len(v), 'wall': sum(x[0] for x in v)/len(v), 'cpu': sum(x[1] for x in v)/len(v), 'nonrunning': sum(x[2] for x in v)/len(v)} for k,v in groups.items()}
    gpu = rows(case / 'frames.csv.gpu.csv')
    # Keep impossible raw startup readings visible; no silent clamping/relabeling.
    result['gpu_over_1000ms_all_frames'] = [r for r in gpu if r['status'] == 'valid' and float(r['gpu_ms']) > 1000]
    gg = defaultdict(list)
    for r in gpu:
        if int(r['frame']) >= WARMUP and r['status'] == 'valid':
            gg[f"{r['view']}/{r['pass']}"] .append(float(r['gpu_ms']))
    result['gpu_groups_ms'] = {k: stats(v) for k,v in gg.items()}
    log = text(case / 'stdout.log')
    result['health'] = {key: re.findall(rf'\b{key}=([^\s]+)', log)[-1] for key in ['GL_error', 'starved_callbacks', 'missing_frames', 'over_budget']}
    result['before'] = text(case / 'before.txt')
    result['after'] = text(case / 'after.txt')
    text(case / 'argv-lines.txt'); text(case / 'binary.sha256'); text(case / 'receipt.json'); text(case / 'metadata.txt')
    summary['cases'][case.name] = result

# Main performance graphic uses first unqueried clock run and a closing replicate.
labels, values = [], []
for case in ['02-clocks', '07-clocks']:
    groups = summary['cases'][case]['paired_groups_ms']
    for key, label in [('render/0/view','Top view'),('render/1/view','Lower view'),('player/-1/none','Player incl. streaming'),('physics/-1/none','Physics'),('swap/0/none','Top swap'),('swap/1/none','Lower swap')]:
        g = groups[key]
        labels.append(f'{case[:2]} {label}')
        values.append([g['cpu'], g['nonrunning']])
bars('01-cpu-wall.svg', 'Render wall time is not all CPU execution', 'Same-row mean milliseconds; GPU queries OFF; cadence 60; first 60 frames excluded.', labels, values, ['Thread CPU execution', 'Wall minus CPU (not GPU wait)'], 'Each row is an independent scope; player includes streaming. The two clock runs bracket query/perf cases.')

svg = start('Frame interval distributions across the live campaign', 'Empirical CDF, first 60 frames excluded. Sequential 30-second cases; not fixed-clock paired trials.', 600)
for tick in range(0,61,10):
    x = 75+tick/60*990
    svg.append(f'<path d="M{x} 90 V460" stroke="#ddd"/><text x="{x}" y="480" text-anchor="middle" font-size="12">{tick} ms</text>')
for tick in range(0,101,25):
    y = 460-tick/100*370
    svg.append(f'<path d="M75 {y} H1065" stroke="#ddd"/><text x="65" y="{y+4}" text-anchor="end" font-size="12">{tick}%</text>')
for i, (case, fr) in enumerate(frame_sets.items()):
    a = sorted(float(r['interval_ms']) for r in fr)
    points = ' '.join(f'{75+min(v,60)/60*990:.2f},{460-(j+1)/len(a)*370:.2f}' for j,v in enumerate(a))
    svg.append(f'<polyline points="{points}" fill="none" stroke="{COLORS[i]}" stroke-width="2"/>')
    x = 30+(i%4)*270; y = 515+(i//4)*28
    svg.append(f'<rect x="{x}" y="{y-12}" width="15" height="12" fill="{COLORS[i]}"/><text x="{x+22}" y="{y}" font-size="13">{case}</text>')
svg.append('<text x="24" y="582" font-size="12">X axis clips at 60 ms; exact maxima are in summary.json. ~16.7/33.3 ms clusters do not establish scanout timing.</text>')
finish('02-frame-cdf.svg', svg)

labels, vals = [], []
for case, s in summary['cases'].items():
    labels.append(case)
    vals.append([s['sampled_frame_ms']['mean'], s['other_frame_ms']['mean']])
# Grouped rather than stacked: never imply these two populations sum to a frame.
svg = start('Query modes: sampled versus other frame intervals', 'Blue: cadence-selected frames; orange: all other frames. Mean interval, ms; first 60 frames excluded.', 580)
maxv = max(max(v) for v in vals)*1.12
for i,(label,v) in enumerate(zip(labels,vals)):
    y=95+i*48
    svg.append(f'<text x="220" y="{y+18}" text-anchor="end" font-size="14">{label}</text>')
    for j,value in enumerate(v):
        width=value/maxv*780
        svg.append(f'<rect x="240" y="{y+j*18}" width="{width}" height="15" fill="{COLORS[j]}"/><text x="{246+width}" y="{y+j*18+12}" font-size="12">{value:.2f}</text>')
svg.append('<text x="24" y="525" font-size="14">These are two frame populations, not additive components. No significance or isolated shader-cost claim.</text>')
svg.append('<text x="24" y="552" font-size="13">Both CPU-only clock runs are included as checks; the postprocessing interruption separates closing brackets.</text>')
finish('03-query-observer.svg',svg)

gpu_labels, gpu_values = [], []
for case in ['03-view','04-passes']:
    for key, g in summary['cases'][case]['gpu_groups_ms'].items():
        gpu_labels.append(f'{case[3:]} {key}')
        gpu_values.append([g['mean']])
bars('05-gpu-elapsed.svg', 'GPU elapsed readings remain nonadditive', 'Per-query means in ms; frame >= 60. View and pass measurements are from different runs.', gpu_labels, gpu_values, ['Reported query elapsed (ms)'], 'Four startup results across both runs were about 1.84e13 ms despite valid status; preserved, excluded by warmup.')

perf = text(D/'06-perf-record/perf-dso.txt')
entries=[]
for line in perf.splitlines():
    m=re.match(r'\s*([\d.]+)%\s+(\d+):\S+\s+(\S+)\s+(\S+)', line)
    if m: entries.append({'percent':float(m[1]),'tid':int(m[2]),'comm':m[3],'dso':m[4]})
summary['perf_dso_rows'] = entries
main_tid=summary['cases']['06-perf-record']['paired_final']['tid']
main=sum(e['percent'] for e in entries if e['tid']==main_tid)
mali=sum(e['percent'] for e in entries if e['tid']==main_tid and 'libmali' in e['dso'])
summary['perf_main_thread_percent']=main
summary['perf_main_libmali_percent']=mali
summary['perf_main_libmali_fraction']=mali/main
rank=sorted(entries,key=lambda e:e['percent'],reverse=True)[:8]
bars('04-perf-cpu.svg','Where perf sampled executing CPU work','Percent of process-and-inherited-thread cpu-clock samples; includes startup; no blocked-time or GPU samples.',[f"{e['tid']} {e['dso']}" for e in rank],[[e['percent']] for e in rank],['Sampled CPU share (%)'], '3973 samples; reported lost samples 0. Main thread '+str(main_tid)+'. libmali addresses lack function symbols.')
text(D/'06-perf-record/perf-statistics.txt');text(D/'06-perf-record/perf-header.txt');text(D/'06-perf-record/perf-buildids.txt')
summary['source_rev'] = text(D/'campaign.txt')
(P/'evidence/campaign-summary.json').write_text(json.dumps(summary,indent=2)+'\n')
(OUT/'provenance.json').write_text(json.dumps({'script_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'inputs':inputs,'filter':'frame >= 60 for CSV and paired plots; perf aggregates whole recording','figures':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in OUT.glob('*.svg')}},indent=2)+'\n')
for case, s in summary['cases'].items():
    f=s['frame_ms']
    print(case, 'mean/p95/p99', *(round(f[x],3) for x in ['mean','p95','p99']), 'sampled/other',round(s['sampled_frame_ms']['mean'],3),round(s['other_frame_ms']['mean'],3), 'gpu_outliers',len(s['gpu_over_1000ms_all_frames']), 'health',s['health'])
    if 'paired_groups_ms' in s:
        for k in ['render/0/view','render/1/view','render/0/scene','render/0/post','render/0/output']:
            print(' ', k, {key:round(v,3) for key,v in s['paired_groups_ms'][k].items()})
print('perf main',main,'mali',mali,'fraction',mali/main)
