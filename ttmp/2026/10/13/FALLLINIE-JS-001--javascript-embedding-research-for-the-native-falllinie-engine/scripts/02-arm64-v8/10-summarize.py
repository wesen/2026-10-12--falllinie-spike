#!/usr/bin/env python3
"""Derive bounded summary from real receipts; never invent target execution."""
import json,pathlib,re
p=pathlib.Path(__file__).resolve().parent;w=p/'.work'
summary={'target_execution':'PENDING','timing_contract':'operation_end_before_logging_v2','protocol_checks':19,'native_overflow_state_preservation':'passed','historical_receipts':'evidence/host and original summary preserved; not rewritten','host_runs':{},'packages':{}}
for run in ['corrected-v2-1','corrected-v2-2','corrected-v2-3']:
 d=w/run;comparison=json.loads((d/'comparison.json').read_text());assert comparison['status']=='PASS'
 engines={}
 for engine in ['bellard','ng','v8']:
  text=(d/f'{engine}.log').read_text();assert text.rstrip().endswith('PROBE PASS')
  assert 'timing_contract=operation_end_before_logging_v2' in text
  assert 'overflow-state-preserved checks=19 copied_count=3 copied_sum=6 copied_values_preserved=true' in text
  cold={name:float(value) for name,value in re.findall(r'^cold (\S+) first_call_us=(\S+) checksum=',text,re.M)}
  benches={}
  for line in text.splitlines():
   if line.startswith('bench '):
    name=line.split()[1];benches[name]={k:float(v) for k,v in re.findall(r'(p50_us|p95_us|p99_us|max_us|calls_per_second|checksum)=(\S+)',line)}
  time=(d/f'{engine}-time.log').read_text()
  engines[engine]={'engine':next(s[7:] for s in text.splitlines() if s.startswith('engine ')),
   'source_read_us':float(re.search(r'^source_read_us=(\S+)',text,re.M)[1]),'cold_first_call_us':cold,
   'internal_elapsed_definition':re.search(r'elapsed_definition=(\S+)',text)[1],
   'process_elapsed_seconds':float(re.search(r'process_elapsed_s=(\S+)',time)[1]),
   'process_max_rss_kib':int(re.search(r'max_rss_kib=(\d+)',time)[1]),'benches':benches,
   'explicit_gc_wall_us':float(re.search(r'explicit_gc_wall_us=(\S+)',text)[1])}
 summary['host_runs'][run]={'started_at':(d/'environment.log').read_text().splitlines()[0],'engines':engines,'comparison':comparison}
packages=json.loads((w/'package-audit.json').read_text())
for engine,d in packages['engines'].items():
 summary['packages'][engine]={k:d[k] for k in ['archive','sha256','archive_bytes']}
summary['limits']={'target_rss_threshold_kib':196608,'target_current_memavailable_min_kib':262144,'target_timeout_seconds':180,'default_sustained_blocks':300,'automatic_quickjs_gc_count':'unavailable','gc_pause_causality':'unavailable','prebuilt_v8_library_compiler_and_flags':'unavailable'}
(w/'results-summary.json').write_text(json.dumps(summary,indent=2)+'\n')
print('SUMMARY PASS; derived host measurements, target explicitly PENDING')
