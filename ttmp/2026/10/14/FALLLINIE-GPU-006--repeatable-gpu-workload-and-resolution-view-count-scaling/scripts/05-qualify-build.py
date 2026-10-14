#!/usr/bin/env python3
"""Verify pre-commit source-hash manifest against an implementation commit."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('revision')
args = parser.parse_args()
root = Path(subprocess.check_output(['git', 'rev-parse', '--show-toplevel'], text=True).strip())
build = Path(__file__).resolve().parent/'evidence/build'
revision = subprocess.check_output(['git', 'rev-parse', args.revision], text=True).strip()
count = 0
for line in (build/'source-hashes.txt').read_text().splitlines():
    digest, name = line.split(maxsplit=1)
    path = Path(name)
    relative = path.relative_to(root) if path.is_absolute() else path
    data = subprocess.check_output(['git', 'show', f'{revision}:{relative}'])
    assert hashlib.sha256(data).hexdigest() == digest, relative
    count += 1
result = {'implementation_commit': revision, 'verified_source_files': count,
          'scope': 'Pre-commit build source identity only; target deployment/acceptance is separate.'}
(build/'qualification.json').write_text(json.dumps(result, indent=2)+'\n')
print(json.dumps(result))
