#!/usr/bin/env python3
"""Offline publication validation; never invokes target/profiling commands."""
import argparse
from decimal import Decimal, ROUND_HALF_UP
import hashlib
import json
from pathlib import Path
import re
import xml.etree.ElementTree as ET
import yaml


def validate(report):
    text=report.read_text()
    assert text.startswith('---\n'), 'frontmatter must be first'
    parts=text.split('---\n',2)
    metadata=yaml.safe_load(parts[1])
    for key in ['title','aliases','tags','status','type','created','repo']:
        assert key in metadata,key
    assert metadata['status']=='research-plan'
    assert text.count('```') % 2 == 0,'unbalanced fences'
    body=parts[2]
    words=len(re.findall(r'\S+',body))
    assert 5000 <= words <= 7000,words
    assert not re.search(r'\b(TODO|TBD|FIXME|PLACEHOLDER)\b',body),'unfinished text'
    assert not re.search(r'\b(like a kitchen|traffic cop|think of .* as a|imagine a factory)\b',body,re.I),'analogy cue'
    links=re.findall(r'!?\[[^\]]*\]\(([^)]+)\)',text)
    local=[p for p in links if not p.startswith(('https://','http://','#'))]
    for target in local:
        assert (report.parent/target).is_file(),target
    assets=report.parent/'_assets/falllinie-trace-005'
    svgs=list(assets.glob('*.svg'))
    assert len(svgs)==5,len(svgs)
    for path in svgs:
        raw=path.read_text();root=ET.fromstring(raw)
        assert root.tag=='{http://www.w3.org/2000/svg}svg'
        assert not re.search(r'<(?:script|image|foreignObject)\b|(?:href|src)=|url\(',raw,re.I),path
        assert root.find('{http://www.w3.org/2000/svg}title') is not None
        assert root.find('{http://www.w3.org/2000/svg}desc') is not None
    manifest=json.loads((assets/'provenance.json').read_text())
    repo=Path(manifest['repository'])
    for item in manifest['files']:
        raw=(repo/item['path']).read_bytes()
        assert hashlib.sha256(raw).hexdigest()==item['sha256'],item['path']
    for item in manifest['figures']:
        assert hashlib.sha256((assets/item['file']).read_bytes()).hexdigest()==item['sha256']
    excerpt=manifest['illustrative_excerpt']
    assert excerpt['interval']['n']==1034
    assert f"{excerpt['interval']['mean_ms']:.6f}" in text
    for group in ['top_cpu_wall','gpu_elapsed']:
        for value in excerpt[group].values():
            assert value['n']==18
            rounded=str(Decimal(str(value['mean_ms'])).quantize(Decimal('0.000001'),rounding=ROUND_HALF_UP))
            assert rounded in text,value
    assert excerpt['query_stats']['schema_version']=='2'
    assert excerpt['query_stats']['valid']=='114'
    assert all(not item['tracked_at_read'] for item in manifest['files'] if '/evidence/target/' in item['path'])
    result={'report_sha256':hashlib.sha256(report.read_bytes()).hexdigest(),'body_words':words,'local_links':len(local),'svg_figures':len(svgs),'input_hashes':len(manifest['files']),'numeric_excerpt':'matched','frontmatter':'valid','fences':'balanced','target_commands':'none'}
    print(json.dumps(result,indent=2))
    return result

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('report',type=Path)
    validate(parser.parse_args().report)
