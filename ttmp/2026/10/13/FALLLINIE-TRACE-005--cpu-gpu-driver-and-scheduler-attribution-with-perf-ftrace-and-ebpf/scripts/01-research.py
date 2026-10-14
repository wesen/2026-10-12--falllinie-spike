#!/usr/bin/env python3
"""Bounded HTTPS documentation snapshots; no device or profiling operations."""
import datetime
import hashlib
import json
from pathlib import Path
import urllib.request

ROOT = Path(__file__).resolve().parent
SOURCES = {
    'ftrace': 'https://docs.kernel.org/trace/ftrace.html',
    'events': 'https://docs.kernel.org/trace/events.html',
    'perf-security': 'https://docs.kernel.org/admin-guide/perf-security.html',
    'bpf-verifier': 'https://docs.kernel.org/bpf/verifier.html',
    'libbpf': 'https://docs.kernel.org/bpf/libbpf/libbpf_overview.html',
    'bpf-license': 'https://docs.kernel.org/bpf/bpf_licensing.html',
    'perf-record-failed': 'https://raw.githubusercontent.com/torvalds/linux/master/tools/perf/Documentation/perf-record.adoc',
    'perf-record': 'https://raw.githubusercontent.com/torvalds/linux/master/tools/perf/Documentation/perf-record.txt',
    'gpu-query-failed': 'https://registry.khronos.org/OpenGL/extensions/EXT/EXT_disjoint_timer_query.txt',
    'gpu-query': 'https://raw.githubusercontent.com/KhronosGroup/OpenGL-Registry/main/extensions/EXT/EXT_disjoint_timer_query.txt',
}

def main():
    dest = ROOT / '.work' / 'sources'
    dest.mkdir(parents=True, exist_ok=True)
    records = []
    for name, url in SOURCES.items():
        record = {'name': name, 'url': url, 'retrieved_utc': datetime.datetime.now(datetime.timezone.utc).isoformat()}
        try:
            with urllib.request.urlopen(url, timeout=20) as response:
                body = response.read(2_000_001)
                if len(body) > 2_000_000:
                    raise ValueError('source exceeds 2MB bound')
            (dest / (name + '.txt')).write_bytes(body)
            record.update(bytes=len(body), sha256=hashlib.sha256(body).hexdigest(), status='retrieved')
        except Exception as error:
            record.update(status='failed', error=str(error))
        records.append(record)
    (ROOT / 'research-provenance.json').write_text(json.dumps(records, indent=2) + '\n')
    print(json.dumps(records, indent=2))

if __name__ == '__main__':
    main()
