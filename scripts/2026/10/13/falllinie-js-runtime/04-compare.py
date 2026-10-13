#!/usr/bin/env python3
"""Exact integers/labels, abs tolerance 1e-9 + relative 1e-12 for doubles."""
import json,math,pathlib
w=pathlib.Path(__file__).resolve().parent/'.work'
expected=json.loads((w/'node-oracle.json').read_text())
counts={}
for name in ('bellard','ng','native'):
 if name=='native': actual=json.loads((w/'native-oracle.json').read_text())
 else: actual=json.loads(next(s[7:] for s in (w/(name+'-results.txt')).read_text().splitlines() if s.startswith('oracle ')))
 count=[0];error=[0.]
 def compare(a,b,path):
  if isinstance(a,dict):
   assert a.keys()==b.keys(),(path,'keys');[compare(a[k],b[k],path+'.'+k) for k in a]
  elif isinstance(a,list):
   assert len(a)==len(b),(path,'length');[compare(x,y,path+f'[{i}]') for i,(x,y) in enumerate(zip(a,b))]
  elif isinstance(a,float):
   assert math.isclose(a,b,rel_tol=1e-12,abs_tol=1e-9),(path,a,b)
   error[0]=max(error[0],abs(a-b));count[0]+=1
  else: assert a==b,(path,a,b);count[0]+=1
 compare(expected,actual,'oracle');counts[name]={'leaves':count[0],'max_absolute_float_error':error[0]}
print(json.dumps({'status':'PASS','comparison':counts},indent=2))
(w/'comparison.json').write_text(json.dumps(counts,indent=2)+'\n')
