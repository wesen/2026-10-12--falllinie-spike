#!/usr/bin/env python3
import csv,hashlib,json,re,statistics
from pathlib import Path
P=Path(__file__).resolve().parent;D=P/'evidence/target-20261014T152820Z'
result={}; hashes={}
def text(p):
 b=p.read_bytes();hashes[str(p.relative_to(P))]=hashlib.sha256(b).hexdigest();return b.decode()
for case in sorted(D.glob('[0-9][0-9]-*')):
 pid=int(text(case/'main-tid.txt'));dso=text(case/'perf-dso.txt');stats=text(case/'perf-statistics.txt');report=text(case/'perf-report.txt')
 pairs=[json.loads(l) for l in text(case/'frames.csv.paired.jsonl').splitlines()];final=pairs[-1]
 assert final['pid']==final['tid']==pid and final['attempted']==final['written']==sum(r['record']=='sample' for r in pairs)
 assert all(final[k]==0 for k in ['dropped','invalid_wall','invalid_cpu','active'])
 assert int(text(case/'perf-status.txt'))==0 and json.loads(text(case/'receipt.json'))['audio_disabled']
 assert 'GL_error=0' in text(case/'game.log')
 lost=int(re.search(r'Total Lost Samples:\s+(\d+)',dso)[1]);assert lost==0
 shares={}
 for pct,tid,name in re.findall(r'^\s*([0-9.]+)%\s+(\d+):\S+\s+(\S+)',dso,re.M):assert int(tid)==pid;shares[name]=float(pct)
 hotspot=re.search(r'^\s*([0-9.]+)%.*\[\.\] 0x000000000042d6dc',report,re.M)
 frames=[float(r['interval_ms']) for r in csv.DictReader(text(case/'frames.csv').splitlines()) if int(r['frame'])>=60]
 mappings=text(case/'process-maps.txt');assert '/usr/lib/libmali.so.1.10.0' in mappings
 result[case.name]={'samples':int(re.search(r'SAMPLE events:\s+(\d+)',stats)[1]),'lost':lost,'dso_weight_percent':shares,'hotspot_42d6dc_percent':float(hotspot[1]) if hotspot else None,'hotspot_missing_means':'Not displayed at 0.5% symbol cutoff, not proven zero.','postwarmup_frame_mean_ms':statistics.mean(frames),'paired_written':final['written'],'config':text(case/'sample-config.txt')}
first=text(D/'01-cpu_cycles/process-maps.txt');base=int(re.search(r'^([0-9a-f]+)-[0-9a-f]+ r-xp 00000000 .* /usr/lib/libmali.so.1.10.0$',first,re.M)[1],16)
callgraph=text(P/'evidence/cycles-callgraph.txt');assert hex(base+0x42d6dc) in callgraph and hex(base+0x4103ef) in callgraph
out={'cases':result,'samples_total':sum(x['samples'] for x in result.values()),'address_mapping':{'base':hex(base),'hotspot':hex(base+0x42d6dc),'caller_perf_address':hex(base+0x4103ef),'caller_note':'perf return-address-minus-one 0x4103ef corresponds to BL at0x4103ec and return at0x4103f0; ELF executable LOAD offset/VA are both0.'}}
(P/'evidence/summary.json').write_text(json.dumps(out,indent=2)+'\n');(P/'evidence/provenance.json').write_text(json.dumps(hashes,indent=2)+'\n');print(json.dumps(out,indent=2))
