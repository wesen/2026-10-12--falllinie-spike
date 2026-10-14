#!/usr/bin/env python3
"""Render the qualified GPU006 scaling summary without plotting dependencies."""
import hashlib
import html
import json
from pathlib import Path
P = Path(__file__).resolve().parent
D = P/'evidence/target-20261014T180649Z'
s = json.loads((D/'summary.json').read_text())
svg = ['<svg xmlns="http://www.w3.org/2000/svg" width="1100" height="780" viewBox="0 0 1100 780">',
       '<rect width="1100" height="780" fill="#faf9f6"/>',
       '<g font-family="sans-serif" fill="#182a37">',
       '<text x="24" y="34" font-size="24">Deterministic live replay: resolution and view-count scaling</text>',
       '<text x="24" y="61" font-size="14">600 frames/case; frames 60–599 summarized; real HUD/rider/particles; audio off; ordinary indexed draws.</text>']
for i, (label, metric, maximum, unit) in enumerate([
    ('Mean completed-frame interval', 'mean_ms', 35, 'ms'),
    ('Frame interval p95', 'p95_ms', 35, 'ms'),
    ('Whole-device fragment activity rate', 'MaliFragActiveCy', 900, 'Mcycles/s')]):
    x = 220+i*290
    svg.append(f'<text x="{x}" y="104" font-size="14">{html.escape(label)}</text>')
    for j, (name, case) in enumerate(s['cases'].items()):
        y = 134+j*65
        value = case[metric] if metric in case else case['rates_per_second'][metric]/1e6
        color = '#b26d28' if case['dual_screen'] else '#287bb5'
        if i == 0:
            caption = name[3:]
            if name.startswith('01'): caption += ' opening'
            if name.startswith('07'): caption += ' closing'
            svg.append(f'<text x="16" y="{y+18}" font-size="13">{html.escape(caption)}</text>')
        width = value/maximum*230
        svg.append(f'<rect x="{x}" y="{y}" width="{width:.2f}" height="26" fill="{color}"/>')
        svg.append(f'<text x="{x}" y="{y+45}" font-size="13">{value:.2f} {unit}</text>')
svg.extend(['<text x="24" y="692" font-size="14">Single-view loop rates remain near 60/s despite higher GPU activity; presentation may cap throughput.</text>',
            '<text x="24" y="718" font-size="14">Dual-view 512×384 slows markedly. Activity is not calibrated utilization; compositor work is included.</text>',
            '<text x="24" y="744" font-size="14">No GPU pass-time or bandwidth-bottleneck claim. One run per nonreference case; fixed clocks were not imposed.</text>',
            '</g></svg>'])
(D/'scaling.svg').write_text('\n'.join(svg)+'\n')
(D/'figure-provenance.json').write_text(json.dumps({
    'script_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    'summary_sha256': hashlib.sha256((D/'summary.json').read_bytes()).hexdigest()}, indent=2)+'\n')
print(D/'scaling.svg')
