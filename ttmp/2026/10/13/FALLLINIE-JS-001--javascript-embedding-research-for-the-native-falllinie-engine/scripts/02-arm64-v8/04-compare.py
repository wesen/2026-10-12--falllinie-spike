#!/usr/bin/env python3
"""85 original leaves, exact labels/integers, abs1e-9 + rel1e-12 doubles.
Also validates deterministic workload checksums and complete probe success.
"""
import argparse,json,math,pathlib,re

def compare(a,b,path='oracle'):
 if isinstance(a,dict):
  assert isinstance(b,dict) and a.keys()==b.keys(),(path,'keys')
  children=[compare(a[k],b[k],path+'.'+k) for k in a]
 elif isinstance(a,list):
  assert isinstance(b,list) and len(a)==len(b),(path,'length')
  children=[compare(x,y,path+f'[{i}]') for i,(x,y) in enumerate(zip(a,b))]
 elif isinstance(a,float):
  assert type(b) in (int,float) and math.isfinite(b) and math.isclose(a,b,rel_tol=1e-12,abs_tol=1e-9),(path,a,b)
  return 1,abs(a-b)
 else:
  assert type(a) is type(b) and a==b,(path,a,b)
  return 1,0
 return sum(x[0] for x in children),max((x[1] for x in children),default=0)

def check_probe_contract(text,engine):
 assert re.search(r'^timing_contract=operation_end_before_logging_v2$',text,re.M),(engine,'missing corrected timing contract')
 assert re.search(r'^protocol PASS copy/offset/type/size/nonfinite/malformed-output/exception/jobs/detach/overflow-state-preserved checks=19 copied_count=3 copied_sum=6 copied_values_preserved=true$',text,re.M),(engine,'missing overflow/state-preservation regression')

def validate(directory,engines):
 expected=json.loads((directory/'node-oracle.json').read_text());work=json.loads((directory/'workload-expected.json').read_text());results={}
 native=directory/'native-oracle.json'
 if native.exists():
  count,error=compare(expected,json.loads(native.read_text()));assert count==85
  results['native']={'leaves':count,'max_absolute_float_error':error}
 for engine in engines:
  text=(directory/f'{engine}.log').read_text()
  assert text.rstrip().endswith('PROBE PASS'),(engine,'missing complete status')
  check_probe_contract(text,engine)
  assert len(re.findall(r'^lifecycle cycle=',text,re.M))==8
  oracle=json.loads(next(s[7:] for s in text.splitlines() if s.startswith('oracle ')))
  count,error=compare(expected,oracle);assert count==85
  checks=[]
  for name,n in work.items():
   cold=re.search(r'^cold '+re.escape(name)+r' first_call_us=(\S+) checksum=(\S+)$',text,re.M)
   assert cold,(engine,name,'cold absent');compare(float(n),float(cold[2]),name+'.cold')
   bench=re.search(r'^bench '+re.escape(name)+r' n=(\d+) .* checksum=(\S+)$',text,re.M)
   assert bench,(engine,name,'bench absent');compare(float(n)*int(bench[1]),float(bench[2]),name+'.warm');checks.append(name)
  config=re.search(r'sustained_blocks=(\d+)',text);assert config
  sustained=re.search(r'^bench sustained_block_8row_4rules_4churn n=(\d+) .* checksum=(\S+)$',text,re.M);assert sustained
  assert int(sustained[1])==int(config[1])
  compare((8*work['batch']+4*work['sourceRules']+4*work['churn'])*int(config[1]),float(sustained[2]),'sustained')
  peak=int(re.search(r'peak_rss_kib=(\d+)',text)[1]);assert peak<192*1024,(engine,'RSS qualification threshold',peak)
  results[engine]={'leaves':count,'max_absolute_float_error':error,'workload_checks':checks+['sustained'],'peak_rss_kib':peak}
 return {'status':'PASS','comparison':results,'tolerance':'abs1e-9 + rel1e-12; labels/integers exact','target_execution':'not implied; directory origin must be recorded separately'}

if __name__=='__main__':
 parser=argparse.ArgumentParser();parser.add_argument('directory',type=pathlib.Path);parser.add_argument('--engines',nargs='+',default=['bellard','ng','v8'],choices=['bellard','ng','v8']);args=parser.parse_args()
 print(json.dumps(validate(args.directory,args.engines),indent=2))
