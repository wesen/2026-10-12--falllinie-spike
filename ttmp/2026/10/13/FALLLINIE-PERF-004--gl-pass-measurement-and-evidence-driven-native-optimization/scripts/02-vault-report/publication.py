#!/usr/bin/env python3
"""Prepare self-contained report assets or validate the narrowly scoped publication.
No device access, builds, source edits, git staging, commits or pushes.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import re
import shutil
import statistics
import subprocess
import xml.etree.ElementTree as ET

MAIN = Path('/home/manuel/code/wesen/2026-10-12--falllinie-spike')
VAULT = Path('/home/manuel/code/wesen/go-go-golems/go-go-parc')
REV = 'f400ffa7c658086cf775fd6d27c28403ba09c2bb'
VAULT_BASE = 'da2c32a7a2978e26c875c553652378c4cdbee617'
BINARY = '2872ede7219ccd91890de4c0d47d5af8fab687f2b5b2cf9605e25ecb1dfcb1d2'
PERF = MAIN/'ttmp/2026/10/13/FALLLINIE-PERF-004--gl-pass-measurement-and-evidence-driven-native-optimization'
TRACE = MAIN/'ttmp/2026/10/13/FALLLINIE-TRACE-005--cpu-gpu-driver-and-scheduler-attribution-with-perf-ftrace-and-ebpf'
CAMPAIGN = TRACE/'scripts/evidence/target-20261014T020124Z'
NOTE_REL = Path('Projects/2026/10/13/PROJECT REPORT - Falllinie Rendering Measurements - CPU Execution, Mali Driver Work and GPU Timer Limits.md')
NOTE = VAULT/NOTE_REL
ASSETS = NOTE.parent/'_assets/falllinie-render-measurements'
FIGURES = ('01-cpu-wall.svg','02-frame-cdf.svg','03-query-observer.svg','04-perf-cpu.svg','05-gpu-elapsed.svg')
HERE = Path(__file__).resolve().parent


def git(repo, *args):
    return subprocess.check_output(['git','-C',str(repo),*args],text=True).strip()


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_csv(path):
    with path.open() as source:
        return list(csv.DictReader(source))


def check(ok, message):
    if not ok:
        raise ValueError(message)


def source_evidence():
    check(git(MAIN,'rev-parse','HEAD') == REV, 'immutable evidence checkout revision changed')
    provenance = json.loads((TRACE/'scripts/figures/provenance.json').read_text())
    for rel, digest in provenance['inputs'].items():
        check(sha(TRACE/'scripts'/rel) == digest, f'input hash mismatch: {rel}')
    check(sha(TRACE/'scripts/09-graphs.py') == provenance['script_sha256'], 'figure generator hash mismatch')
    for name in FIGURES:
        check(sha(TRACE/'scripts/figures'/name) == provenance['figures'][name], f'figure hash mismatch: {name}')
    summary = json.loads((TRACE/'scripts/evidence/campaign-summary.json').read_text())
    total = postwarmup = pairs = 0
    for name, case in summary['cases'].items():
        directory = CAMPAIGN/name
        check((directory/'binary.sha256').read_text().split()[0] == BINARY, 'binary mismatch')
        rows = read_csv(directory/'frames.csv')
        warm = [r for r in rows if int(r['frame']) >= 60]
        total += len(rows); postwarmup += len(warm)
        check(len(warm) == case['frame_ms']['n'], f'frame count mismatch: {name}')
        check(abs(statistics.mean(float(r['interval_ms']) for r in warm)-case['frame_ms']['mean']) < 1e-9, f'frame mean mismatch: {name}')
        check(all(v == '0' for v in case['health'].values()), f'nonzero health counter: {name}')
        if 'paired_final' in case:
            sidecar = [json.loads(line) for line in (directory/'frames.csv.paired.jsonl').read_text().splitlines()]
            records = [r for r in sidecar if r['record'] == 'sample']
            final = sidecar[-1]
            check(final == case['paired_final'], f'paired final mismatch: {name}')
            check(final['attempted'] == final['written'] == len(records) == 560, f'paired count mismatch: {name}')
            check(all(final[k] == 0 for k in ('active','dropped','invalid_wall','invalid_cpu')), 'incomplete paired collection')
            check(all(r['wall_status'] == r['cpu_status'] == 'valid' for r in records), 'invalid paired sample')
            pairs += len(records)
            for key, group in case['paired_groups_ms'].items():
                stage, view, scope = key.split('/')
                selected = [r for r in records if r['frame'] >= 60 and r['stage'] == stage and r['view'] == int(view) and r['pass'] == scope]
                check(len(selected) == group['n'], 'paired group count mismatch')
                wall = statistics.mean((r['wall_end_ns']-r['wall_start_ns'])/1e6 for r in selected)
                cpu = statistics.mean((r['cpu_end_ns']-r['cpu_start_ns'])/1e6 for r in selected)
                check(abs(wall-group['wall']) < 1e-9 and abs(cpu-group['cpu']) < 1e-9, 'paired group mean mismatch')
    check((total,postwarmup,pairs) == (13133,12653,3360), 'campaign totals mismatch')
    sample_path = CAMPAIGN/'02-clocks/frames.csv.paired.jsonl'
    sample = next(r for r in map(json.loads,sample_path.read_text().splitlines()) if r.get('frame') == 60 and r.get('pass') == 'output' and r.get('view') == 0)
    w = sample['wall_end_ns']-sample['wall_start_ns']; c = sample['cpu_end_ns']-sample['cpu_start_ns']
    check((w,c,w-c) == (6105253,1080042,5025211), 'representative calculation mismatch')
    initial = []
    extra_hashes = {}
    for d in sorted((PERF/'scripts/evidence/target').iterdir()):
        if not (d/'frames.csv').exists(): continue
        rows = read_csv(d/'frames.csv'); selected = [r for r in rows if int(r['frame']) >= 60]
        initial.append({'case':d.name,'total_frames':len(rows),'postwarmup_frames':len(selected),'mean_interval_ms':statistics.mean(float(r['interval_ms']) for r in selected),'metadata':(d/'metadata.txt').read_text()})
        for f in ('frames.csv','metadata.txt','binary.sha256'):
            extra_hashes[str((d/f).relative_to(MAIN))] = sha(d/f)
    for rel in ('src/main.cpp','src/profiler.cpp','src/paired_clocks.cpp','src/paired_clocks.hpp','src/renderer.cpp','src/game_renderer.cpp'):
        extra_hashes[rel] = sha(MAIN/rel)
    perf_stat = CAMPAIGN/'05-perf-stat/perf-stat.csv'
    extra_hashes[str(perf_stat.relative_to(MAIN))] = sha(perf_stat)
    check('39610.22,msec,cpu-clock' in perf_stat.read_text() and '83050,,context-switches' in perf_stat.read_text() and '18127,,cpu-migrations' in perf_stat.read_text(), 'perf-stat values mismatch')
    check('SAMPLE events:       3973' in (CAMPAIGN/'06-perf-record/perf-statistics.txt').read_text(), 'perf sample count mismatch')
    anomalies = [r for case in summary['cases'].values() for r in case['gpu_over_1000ms_all_frames']]
    check(len(anomalies) == 4 and all(r['frame'] == '0' and r['status'] == 'valid' for r in anomalies), 'startup anomaly population mismatch')
    gpu_view_sum = sum(g['mean'] for g in summary['cases']['03-view']['gpu_groups_ms'].values())
    gpu_pass_sum = sum(g['mean'] for g in summary['cases']['04-passes']['gpu_groups_ms'].values())
    check(round(gpu_view_sum,3) == 27.272 and round(gpu_pass_sum,3) == 42.174, 'GPU aggregate mismatch')
    check(abs(summary['perf_main_libmali_percent']/summary['perf_main_thread_percent']-summary['perf_main_libmali_fraction']) < 1e-12, 'perf denominator mismatch')
    return sample, {'source_revision':REV,'runtime_revision':'209a6d092885ff3f0a1efd1efab4252cb4ad2b56','runtime_binary_sha256':BINARY,'trace_total_frames':total,'trace_postwarmup_frames':postwarmup,'paired_records':pairs,'gpu_view_sum_ms':gpu_view_sum,'gpu_pass_sum_ms':gpu_pass_sum,'initial_perf004':initial,'representative_sample_source':str(sample_path.relative_to(MAIN)),'representative_sample_source_sha256':sha(sample_path),'representative_calculation_ns':{'wall':w,'cpu':c,'residual':w-c},'source_hashes':extra_hashes,'summary_sha256':sha(TRACE/'scripts/evidence/campaign-summary.json'),'limits':['Sequential live runs; ondemand clocks; no randomized replicates.','Paired residual is not proven GPU wait; nested scopes nonadditive.','GPU semantic validity unresolved; four startup anomalies remain raw.','Perf all-thread whole-recording denominator includes startup.','Post-run captures do not prove query-active image equivalence.']}


def prepare():
    check(git(VAULT,'branch','--show-current') == 'main' and git(VAULT,'rev-parse','HEAD') == VAULT_BASE, 'vault branch/base drift')
    check(not ASSETS.exists(), 'append-only asset destination already exists')
    sample, evidence = source_evidence()
    ASSETS.mkdir(parents=True)
    for name in (*FIGURES,'provenance.json'):
        shutil.copyfile(TRACE/'scripts/figures'/name,ASSETS/name)
    shutil.copyfile(TRACE/'scripts/evidence/campaign-summary.json',ASSETS/'campaign-summary.json')
    (ASSETS/'representative-sample.json').write_text(json.dumps(sample,indent=2)+'\n')
    (ASSETS/'report-evidence.json').write_text(json.dumps(evidence,indent=2)+'\n')


def validate():
    sample, evidence = source_evidence()
    check(json.loads((ASSETS/'report-evidence.json').read_text()) == evidence, 'report evidence changed')
    check(json.loads((ASSETS/'representative-sample.json').read_text()) == sample, 'sample changed')
    check(sha(ASSETS/'campaign-summary.json') == evidence['summary_sha256'], 'summary copy changed')
    check(sha(ASSETS/'provenance.json') == sha(TRACE/'scripts/figures/provenance.json'), 'provenance copy changed')
    for name in FIGURES:
        check(sha(ASSETS/name) == sha(TRACE/'scripts/figures'/name), 'copied figure mismatch')
        ET.parse(ASSETS/name)
    text = NOTE.read_text()
    prose = re.sub(r'^---\n.*?\n---\n','',text,flags=re.S)
    prose = re.sub(r'```.*?```','',prose,flags=re.S)
    words = len(re.findall(r"\b[\w]+(?:[-’'][\w]+)*\b",prose))
    check(4500 <= words <= 6500, f'word count outside requested range: {words}')
    check(text.count('```mermaid') >= 2, 'missing Mermaid diagrams')
    links = re.findall(r'\]\(([^)]+)\)',text)
    for link in links:
        check(not link.startswith(('http:','https:','file:','/')), 'unexpected external report link')
        check((NOTE.parent/link).is_file(), f'missing relative asset: {link}')
    for title in re.findall(r'\[\[([^\]]+)\]\]',text):
        check((NOTE.parent/(title+'.md')).exists(), f'missing related note: {title}')
    check(set(FIGURES) <= {Path(x).name for x in links}, 'not all five measured figures embedded')
    summary = json.loads((ASSETS/'campaign-summary.json').read_text())
    for case in summary['cases'].values():
        for key in ('mean','p95','p99'):
            check(f"{case['frame_ms'][key]:.3f}" in text, 'missing rounded frame-table value')
    for row in evidence['initial_perf004']:
        check(f"{row['mean_interval_ms']:.3f}" in text and f"{row['postwarmup_frames']:,}" in text, 'initial table mismatch')
    for value in ('6.105253','1.080042','5.025211','27.272','42.174','71.44','39.28','28.06','13,133','12,653','3,360',BINARY,REV):
        check(value in text, f'missing audited value: {value}')
    for phrase in ('non-running estimate','post-run uninstrumented fixture repeatability','not GPU utilization','Query-active image equivalence remains untested','FTRACE and KPROBES','schema 2','schema 1'):
        check(phrase in text, f'missing scope qualification: {phrase}')
    allowed = {str(NOTE_REL)} | {str(p.relative_to(VAULT)) for p in ASSETS.iterdir()}
    changed = subprocess.check_output(['git','-C',str(VAULT),'ls-files','--others','--exclude-standard'],text=True).splitlines()
    changed += subprocess.check_output(['git','-C',str(VAULT),'diff','--name-only','HEAD'],text=True).splitlines()
    check(set(changed) == allowed, f'vault scope drift: {set(changed)^allowed}')
    check(git(VAULT,'rev-parse','HEAD') == VAULT_BASE and git(VAULT,'branch','--show-current') == 'main','vault base/branch drift')
    return {'status':'passed','prose_word_count':words,'mermaid_diagrams':text.count('```mermaid'),'figures':len(FIGURES),'source_revision':REV,'vault_base':VAULT_BASE,'note':str(NOTE_REL),'files':{str(p.relative_to(VAULT)):sha(p) for p in [NOTE,*sorted(ASSETS.iterdir())]},'checks':['source input/provenance/figure hashes','frame counts and means from raw CSV','same-row paired group/sample calculations','perf denominator and GPU totals/anomaly population','relative assets/XML and related notes','new-note/new-asset scope; unchanged vault base']}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action',choices=('prepare','validate'))
    args = parser.parse_args()
    if args.action == 'prepare':
        prepare()
        print('Prepared five measured figures and self-contained evidence assets.')
    else:
        result = validate()
        (HERE/'validation.json').write_text(json.dumps(result,indent=2)+'\n')
        print(json.dumps(result,indent=2))
