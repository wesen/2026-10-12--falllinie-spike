#!/usr/bin/env python3
"""Make matched offline excerpts; never modify production audio or device settings."""
from pathlib import Path
import hashlib,json,subprocess,wave
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[4]
WORK=HERE/'.work';WORK.mkdir(exist_ok=True)
source=(ROOT/'src/audio.cpp').read_text()
needle='vinyl_filter.process(vinyl[vinyl_pos])*0.22'
assert source.count(needle)==1
flags=subprocess.check_output(['pkg-config','--cflags','--libs','sdl2'],text=True).split()
records=[]
for label,text in [('source',source),('without-vinyl',source.replace(needle,'vinyl_filter.process(vinyl[vinyl_pos])*0.0'))]:
 cpp=WORK/(label+'.cpp');cpp.write_text(text)
 binary=WORK/(label+'-probe');output=WORK/(label+'.wav')
 subprocess.run(['g++','-std=c++17','-O2','-pthread','-I'+str(ROOT/'src'),str(ROOT/'tests/test_audio.cpp'),str(cpp),'-o',str(binary),*flags],check=True)
 with (HERE/(label+'-render.log')).open('w') as log:
  subprocess.run([str(binary),'--render',str(output),'30'],stdout=log,stderr=subprocess.STDOUT,check=True)
 records.append({'variant':label,'source_sha256':hashlib.sha256(text.encode()).hexdigest(),'wav_sha256':hashlib.sha256(output.read_bytes()).hexdigest()})
# One listening test: 15 seconds reference, one second silence, same 15 seconds
# with only the vinyl contribution zeroed. Both are precomputed: no live DSP.
with wave.open(str(WORK/'source.wav'),'rb') as a,wave.open(str(WORK/'without-vinyl.wav'),'rb') as b:
 assert a.getparams()==b.getparams()
 params=a.getparams();n=a.getframerate()*15
 left=a.readframes(n);right=b.readframes(n);assert left!=right
 with wave.open(str(WORK/'matched-ab.wav'),'wb') as out:
  out.setparams(params);out.writeframes(left);out.writeframes(bytes(a.getframerate()*a.getnchannels()*a.getsampwidth()));out.writeframes(right)
(HERE/'manifest.json').write_text(json.dumps({'production_modified':False,'difference':'vinyl contribution .22 -> 0; generation/filter/RNG retained','listening_order':'15s reference, 1s silence, 15s no-vinyl; both offline','renders':records},indent=2)+'\n')
print(WORK/'matched-ab.wav')
