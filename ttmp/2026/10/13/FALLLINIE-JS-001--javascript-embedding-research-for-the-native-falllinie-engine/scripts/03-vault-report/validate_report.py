#!/usr/bin/env python3
"""Validate report scope, local assets, source hashes and measured table values."""
import hashlib
import json
import re
import xml.etree.ElementTree as ET
from generate_assets import ASSETS, DATED, ENGINES, P, REV, ROOT, source

NOTE = DATED / 'PROJECT REPORT - Falllinie JavaScript Embedding - ARM64 Qualification, V8 Ownership and Bounded Rules.md'


def main():
    report = NOTE.read_text()
    assert report.startswith('---\n')
    _, metadata, body = report.split('---\n', 2)
    for field in ('title', 'aliases', 'tags', 'status', 'type', 'created', 'report_date',
                  'project_started', 'repo', 'source_revision', 'ticket'):
        assert re.search(r'^' + field + ':', metadata, re.M), field
    assert 'source_revision: 22241ca\n' in metadata and REV in metadata
    assert 'project_started: 2026-10-12\n' in metadata
    assert 'created: 2026-10-13\n' in metadata and 'status: active\n' in metadata
    fences = re.findall(r'^```.*$', body, re.M)
    assert len(fences) % 2 == 0, 'unbalanced fences'
    for i, line in enumerate(fences):
        assert (line == '```') if i % 2 else (line != '```'), 'fence order'
    prose = re.sub(r'```[^\n]*\n.*?\n```', '', body, flags=re.S)
    prose = '\n'.join(line for line in prose.splitlines() if not line.startswith(('|', '#', '![](')))
    words = re.findall(r"\b[\w]+(?:[’'−-][\w]+)*\b", prose)
    all_words = re.findall(r'\b\w+\b', body)
    assert 6000 <= len(words) <= 8000, ('substantive prose words', len(words))
    assert not re.search(r'\b(TODO|TBD|FIXME|PLACEHOLDER)\b', report)
    analogy = r'\b(think of .{0,60} as|like a (kitchen|traffic cop|factory|train)|imagine a (kitchen|factory)|metaphor|analogy)\b'
    assert not re.search(analogy, body, re.I), 'analogy pattern'
    links = re.findall(r'\]\(([^)]+)\)', body)
    for link in links:
        assert not re.match(r'https?://|/|\.\.', link), ('nonportable asset link', link)
        assert (DATED / link).is_file(), ('missing local asset', link)
    assert len([x for x in links if x.endswith('.svg')]) == 5
    expected_assets = {'ownership.svg', 'copy-commit.svg', 'evidence-pipeline.svg', 'latency.svg', 'memory.svg', 'provenance.json'}
    assert {f.name for f in ASSETS.iterdir()} == expected_assets
    for f in sorted(ASSETS.glob('*.svg')):
        xml = ET.parse(f)
        assert xml.getroot().tag == '{http://www.w3.org/2000/svg}svg'
        assert xml.getroot().find('{http://www.w3.org/2000/svg}title') is not None
        for element in xml.iter():
            assert element.tag.split('}')[-1] not in ('script', 'foreignObject', 'image', 'use')
            assert not any('href' in name for name in element.attrib), 'external/link dependency'
        assert not re.search(r'url\(|@import|<!ENTITY|<!DOCTYPE', f.read_text(), re.I)
    provenance = json.loads((ASSETS / 'provenance.json').read_text())
    assert provenance['source_revision'] == REV
    for path, digest in provenance['receipt_sha256'].items():
        assert hashlib.sha256(source(path)).hexdigest() == digest, ('receipt hash', path)
    assert hashlib.sha256(source('fixtures/falllinie.html')).hexdigest() == provenance['fixture_sha256']
    summary = json.loads(source(P + '/evidence/target-corrected-v2/summary.json'))
    checks = {'Row warm p50, ms': ('benches', 'batch', 'p50_us'),
              'Row warm p99, ms': ('benches', 'batch', 'p99_us'),
              'First row wrapper, ms': ('first_calls', 'batch', 'first_call_us'),
              'Rules-batch warm p50, ms': ('benches', 'sourceRules', 'p50_us'),
              'Rules-batch warm p99, ms': ('benches', 'sourceRules', 'p99_us'),
              'First rules wrapper, ms': ('first_calls', 'sourceRules', 'first_call_us'),
              'Heavy block p50, ms': ('benches', 'sustained_block_8row_4rules_4churn', 'p50_us'),
              'Heavy block p99, ms': ('benches', 'sustained_block_8row_4rules_4churn', 'p99_us'),
              'Heavy block maximum, ms': ('benches', 'sustained_block_8row_4rules_4churn', 'max_us'),
              'Explicit requested GC wall, ms': ('explicit_gc_wall_us',),
              'Reported getrusage peak RSS, KiB': ('process', 'peak_rss_kib'),
              'Maximum sampled VmRSS, KiB': ('max_sampled_vm_rss_kib',)}
    for label, keys in checks.items():
        line = next(line for line in body.splitlines() if line.startswith('| ' + label + ' |'))
        values = [float(x.strip()) for x in line.split('|')[2:5]]
        for engine, actual in zip(ENGINES, values):
            expected = summary['engines'][engine]
            for key in keys:
                expected = expected[key]
            if label.endswith(', ms'):
                expected /= 1000
            assert abs(actual - expected) < 1e-6, (label, engine, actual, expected)
    wall_line = next(line for line in body.splitlines() if line.startswith('| External integer process wall, s |'))
    walls = [int(x.strip()) for x in wall_line.split('|')[2:5]]
    for e, wall in zip(ENGINES, walls):
        timing = source(P + '/evidence/target-corrected-v2/' + e + '-time.log').decode()
        assert wall == int(re.search(r'process_elapsed_integer_seconds=(\d+)', timing)[1])
        assert 'exit=0' in timing
    for e in ENGINES:
        for workload in ('batch', 'sourceRules'):
            label = f'{summary["engines"][e]["benches"][workload]["p50_us"] / 1000:.6f}'
            assert label in (ASSETS / 'latency.svg').read_text()
        assert str(int(summary['engines'][e]['process']['peak_rss_kib'])) in (ASSETS / 'memory.svg').read_text()
        assert provenance['target_metrics'][e] == {k: summary['engines'][e][k] for k in provenance['target_metrics'][e]}
        for path, digest in summary['engines'][e]['source_sha256'].items():
            assert hashlib.sha256(source(P + '/evidence/target-corrected-v2/' + path)).hexdigest() == digest
    print(f'VALIDATION PASS substantive_prose_words={len(words)} body_words_including_tables_code={len(all_words)}')
    print(f'5 portable SVGs; 6 local links; {len(provenance["receipt_sha256"])} immutable receipt hashes; 13 metric rows x3 engines')
    print('Frontmatter/fences/XML/external-dependency/placeholder/analogy-pattern checks passed; manual editorial review remains required.')
    print('Changed vault inventory (exactly seven new files):')
    for f in [NOTE, *sorted(ASSETS.iterdir())]:
        print(f.relative_to(DATED.parents[3]))


if __name__ == '__main__':
    main()
