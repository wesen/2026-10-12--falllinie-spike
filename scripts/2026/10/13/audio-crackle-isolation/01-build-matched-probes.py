#!/usr/bin/env python3
"""Reproduce matched offline excerpts via supported API; preserve historical evidence."""
from pathlib import Path
import hashlib,json,subprocess,wave
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[4]
# Historical logs/manifest beside this script are immutable. New API-based
# reproduction belongs to AUDIO-003, never overwrites the original experiment.
EVIDENCE=ROOT/'ttmp/2026/10/13/FALLLINIE-AUDIO-003--buffered-native-audio-worker-and-callback-deadline-qualification/scripts/vinyl-probes'
EVIDENCE.mkdir(parents=True,exist_ok=True)
(EVIDENCE/'.gitignore').write_text('.work/\n')
WORK=EVIDENCE/'.work';WORK.mkdir(exist_ok=True)
flags=subprocess.check_output(['pkg-config','--cflags','--libs','sdl2'],text=True).split()
binary=WORK/'offline-probe'
subprocess.run(['g++','-std=c++17','-O2','-pthread','-I'+str(ROOT/'src'),str(ROOT/'tests/test_audio.cpp'),str(ROOT/'src/audio.cpp'),'-o',str(binary),*flags],check=True)
source_sha=hashlib.sha256((ROOT/'src/audio.cpp').read_bytes()).hexdigest()
records=[]
for label,options in [('source',[]),('without-vinyl',['--no-vinyl'])]:
 output=WORK/(label+'.wav')
 with (EVIDENCE/(label+'-render.log')).open('w') as log:
  subprocess.run([str(binary),'--render',str(output),'30',*options],stdout=log,stderr=subprocess.STDOUT,check=True)
 records.append({'variant':label,'source_vinyl':not options,'source_sha256':source_sha,'wav_sha256':hashlib.sha256(output.read_bytes()).hexdigest()})
with wave.open(str(WORK/'source.wav'),'rb') as a,wave.open(str(WORK/'without-vinyl.wav'),'rb') as b:
 assert a.getparams()==b.getparams()
 params=a.getparams();n=a.getframerate()*15
 left=a.readframes(n);right=b.readframes(n);assert left!=right
 with wave.open(str(WORK/'matched-ab.wav'),'wb') as out:
  out.setparams(params);out.writeframes(left);out.writeframes(bytes(a.getframerate()*a.getnchannels()*a.getsampwidth()));out.writeframes(right)
(EVIDENCE/'manifest.json').write_text(json.dumps({'production_modified':False,'difference':'prepare_offline source_vinyl true vs false; generation/filter/RNG retained','listening_order':'15s reference, 1s silence, 15s no-vinyl; both offline (not played by this script)','renders':records},indent=2)+'\n')
print(WORK/'matched-ab.wav')
