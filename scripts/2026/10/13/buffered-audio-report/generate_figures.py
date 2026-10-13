#!/usr/bin/env python3
"""Original report SVGs from immutable source evidence; no devices or native edits."""
from pathlib import Path
from xml.sax.saxutils import escape
import csv, hashlib, json, math, subprocess

NATIVE=Path('/home/manuel/code/wesen/2026-10-12--falllinie-spike')
TICKET=NATIVE/'ttmp/2026/10/13/FALLLINIE-AUDIO-003--buffered-native-audio-worker-and-callback-deadline-qualification'
VAULT=Path('/home/manuel/code/wesen/go-go-golems/go-go-parc')
OUT=VAULT/'Projects/2026/10/13/_assets/falllinie-buffered-audio-20261013'
REV='686872c1d68776924b94dbf9a684eae174c1573e'
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=NATIVE,text=True).strip()==REV
OUT.mkdir(parents=True,exist_ok=True)
BLUE='#1769aa';GREEN='#16835c';RED='#b33c32';INK='#172b3a';GRAY='#566675'
def text(x,y,s,size=20,color=INK):return f'<text x="{x}" y="{y}" font-size="{size}" fill="{color}">{escape(str(s))}</text>'
def rect(x,y,w,h,fill='#edf4fa'):return f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="8" fill="{fill}" stroke="#bacbd6"/>'
def line(x,y,xx,yy,color=GRAY,dash=False):return f'<line x1="{x}" y1="{y}" x2="{xx}" y2="{yy}" stroke="{color}" stroke-width="2"'+(' stroke-dasharray="6 5"' if dash else '')+'/>'
def arrow(x,y,xx,yy):return line(x,y,xx,yy)+f'<path d="M {xx-8},{yy-5} L {xx},{yy} L {xx-8},{yy+5}" fill="none" stroke="{GRAY}" stroke-width="2"/>'
def save(name,title,desc,body,height=620):
 data=f'<svg xmlns="http://www.w3.org/2000/svg" width="1200" height="{height}" viewBox="0 0 1200 {height}" role="img"><title>{escape(title)}</title><desc>{escape(desc)}</desc><rect width="1200" height="{height}" fill="white"/><g font-family="sans-serif">{text(32,42,title,26)}{body}</g></svg>\n'
 (OUT/name).write_text(data)

b=''
for x,y,w,h,title,lines in [
 (35,100,265,170,'Main thread',['Input + simulation','Numeric command producer','SDL lifetime; video + GL']),
 (410,100,310,170,'DSP worker',['Command consumer','Sole live musical state','Render 1024-frame blocks']),
 (820,100,340,170,'SDL callback',['PCM consumer','Copy + silence + counters','No DSP or producer wake']),
 (410,345,310,155,'PCM frame ring',['Capacity 8192 stereo frames','Target 2048 at request 1024','Release/acquire cursors'])]:
 b+=rect(x,y,w,h)+text(x+15,y+32,title,23)
 for i,s in enumerate(lines):b+=text(x+15,y+67+i*29,s,18)
 b+=('')
b+=arrow(300,170,410,170)+text(307,147,'commands',15)
b+=line(565,270,565,345)+text(575,305,'publish',17)
b+=arrow(720,415,990,415)+line(990,415,990,270)+text(825,402,'retire after copy',17)
b+=text(35,552,'Buffered ownership shown. Direct mode excludes the worker; offline excludes both live owners.',20)
b+=text(35,584,'A thread owns state, not a physical core. No affinity or real-time scheduling policy was changed.',18)
save('01-ownership.svg','One live DSP owner; one PCM producer and consumer','Current implementation ownership. SDL and GL video calls stay on main. Direct and offline modes are mutually exclusive with the buffered worker.',b)

b=text(35,83,'Illustrative capacity 8; values label logical stereo frames, not measured playback samples.',18)
for row,(label,items) in enumerate([
 ('Publish frames A0..A7',['A0','A1','A2','A3','A4','A5','A6','A7']),
 ('Retire first 3 frames',['free','free','free','A3','A4','A5','A6','A7']),
 ('Refill 3; physical wrap',['B0','B1','B2','A3','A4','A5','A6','A7'])]):
 y=120+row*105;b+=text(35,y+35,label,20)
 for j,item in enumerate(items):
  x=350+j*98;b+=rect(x,y,88,65,'#e8f5ed' if item.startswith('B') else '#edf4fa')+text(x+12,y+40,item,20)
b+=text(35,470,'Next consumption order: A3, A4, A5, A6, A7, B0, B1, B2.',24)
b+=text(35,514,'The consumer retires 3 copied frames, not the whole original 8-frame publication.',20)
b+=text(35,550,'Acquire observes completed writes; release retirement permits overwrite only after copying.',19)
save('02-partial-wrap.svg','Frame retirement preserves unread PCM across wrap','A deterministic eight-frame example matching the transport test. A partial callback keeps the five unread A frames intact while the producer refills the retired prefix.',b,590)

b=text(35,85,'Per channel: 2.6-second response = 124800 taps at 48000 Hz = 122 partitions.',20)
for j in range(5):
 x=35+j*220;b+=rect(x,115,200,70)+text(x+12,155,'h['+str(j)+'] : 1024 taps',18)
b+=text(40,221,'Each partition: zero-pad to 2048 → FFT → retain 1025 independent real-signal bins.',20)
b+=rect(35,250,510,108)+text(50,284,'Completed-history partitions 1..121',23)+text(50,320,'Multiply-accumulate across 64-sample groups',19)
b+=rect(630,250,530,108)+text(645,284,'At input-block completion',23)+text(645,320,'Current FFT + partition 0 + inverse FFT',19)
b+=line(290,358,290,398)+line(890,358,890,398)+line(290,398,890,398)
b+=rect(200,430,790,100)+text(218,465,'Reconstruct conjugate spectrum → inverse FFT → overlap-add',22)+text(218,503,'Emit stored wet output with one explicit 1024-sample partition delay',18)
b+=text(35,580,'Numerical path is unchanged by buffering; this is two independent stereo convolutions.',20)
save('03-convolution.svg','Full stereo partitioned convolution, not a reduced response','Conceptual computation diagram from Reverb::init/process. The first five of 122 partitions are illustrated; all partitions are retained. Completion-dependent FFT work still occurs at block boundaries.',b)

records={};hashes={};intervals={}
for mode in ['direct','buffered']:
 folder=TICKET/'scripts/target'/mode
 log=(folder/'run.log').read_text()
 audio=next(s for s in log.splitlines() if s.startswith('AUDIO '))
 frame=next(s for s in log.splitlines() if s.startswith('frames='))
 records[mode]={'audio':dict(v.split('=',1) for v in audio.split()[1:]),'frame':dict(v.split('=',1) for v in frame.split())}
 rows=list(csv.DictReader((folder/'frames.csv').open()))
 intervals[mode]=sorted(float(r['interval_ms']) for r in rows if int(r['frame'])>=60)
 records[mode]['csv_rows']=len(rows);records[mode]['csv_measured_rows']=len(intervals[mode])
 records[mode]['gpu_rows']=len(list(csv.DictReader((folder/'frames.csv.gpu.csv').open())))
 assert records[mode]['gpu_rows']==0
 for name in ['run.log','frames.csv','frames.csv.gpu.csv']:
  hashes[f'{mode}/{name}']=hashlib.sha256((folder/name).read_bytes()).hexdigest()

b=text(35,85,'PulseAudio backend; 48000 Hz / 1024 frames; 120 seconds per mode, direct first.',19)
b+=text(35,112,'Matched configuration, not identical live inputs. Max wall and max CPU are independent aggregates.',17)
x0=365;scale=700/32
for n,label in enumerate(['Direct callback','Buffered callback','Buffered producer']):
 a=records['direct' if n==0 else 'buffered']['audio'];prefix='producer_' if n==2 else ''
 wall=[float(a[prefix+'mean_ms']),float(a[prefix+'max_ms'])]
 cp=[float(a['producer_cpu_mean_ms' if n==2 else 'callback_cpu_mean_ms']),float(a['producer_cpu_max_ms' if n==2 else 'callback_cpu_max_ms'])]
 y=165+n*110;b+=text(35,y+19,label,20)
 for offset,values,col,lab in [(0,wall,BLUE,'wall'),(36,cp,GREEN,'CPU')]:
  b+=text(250,y+offset+19,lab,18,col)
  b+=f'<rect x="{x0}" y="{y+offset}" width="{values[0]*scale}" height="20" fill="{col}"/>'
  b+=line(x0+values[1]*scale,y+offset-4,x0+values[1]*scale,y+offset+25,col)
  b+=text(1080,y+offset+17,f'{values[0]:.3f}',17,col)
 b+=text(365,y+85,f'max wall {wall[1]:.3f} ms; max CPU {cp[1]:.3f} ms',16)
for v in [0,5,10,15,20,25,30]:
 b+=line(x0+v*scale,492,x0+v*scale,502)+text(x0+v*scale-8,526,v,16)
b+=line(x0+21.333333*scale,135,x0+21.333333*scale,492,RED,True)
b+=text(665,554,'Dashed: nominal 21.333 ms callback/block budget',18,RED)
b+=text(35,588,'Bars = means; ticks = maxima. Buffered producer exceedances 16; PCM shortage 0.',20)
save('04-target-timing.svg','Callback computation moved to the producer','Grouped horizontal timing chart in milliseconds. Wall and calling-thread CPU are separately recorded means and maxima. The producer retains substantial computation; buffered callback cost is small. No paired per-block scheduling trace is inferred.',b,620)

b=text(35,83,'Empirical CDF of recorded interval_ms, excluding frame indices 0..59; GPU CSVs contain no samples.',18)
x0=100;y0=440;w=1000;h=300
for p in [0,.25,.5,.75,1]:
 y=y0-p*h;b+=line(x0,y,x0+w,y,'#d9e2e8')+text(42,y+6,f'{p*100:.0f}%',16)
for v in [10,15,20,25,30,35,40]:
 x=x0+(v-10)/30*w;b+=line(x,y0,x,y0+8)+text(x-12,y0+30,v,17)
for mode,col in [('direct',BLUE),('buffered',GREEN)]:
 a=intervals[mode];points=[]
 for i,v in enumerate(a):
  if 10<=v<=40 and (i%8==0 or i==len(a)-1):points.append(f'{x0+(v-10)/30*w:.2f},{y0-(i+1)/len(a)*h:.2f}')
 b+=f'<polyline points="{" ".join(points)}" fill="none" stroke="{col}" stroke-width="2"/>'
 b+=text(110 if mode=='direct' else 590,512,f'{mode}: {len(a)} intervals',20,col)
x=x0+(1000/60-10)/30*w;b+=line(x,125,x,440,RED,True)+text(x+8,132,'16.667 ms (60 Hz)',17,RED)
b+=text(35,552,'Horizontal range 10..40 ms; rare values outside this range remain in the CDF denominator.',18)
b+=text(35,586,'CPU loop intervals are not physical scanout. Similar distributions do not establish causality.',19)
save('05-frame-intervals.svg','Both target configurations remain near 50 frames per second','Empirical cumulative distribution from the two original target CPU CSVs. The plotted quantity is completed-frame interval, distinct from the terminal frame_start-to-end cost. No GPU durations exist in the header-only GPU CSVs.',b)
provenance={'source_revision':REV,'ticket':'FALLLINIE-AUDIO-003','source_repository':str(NATIVE),'figures_generated_by':'scripts/2026/10/13/buffered-audio-report/generate_figures.py','notes':['Schematics01..03 are explanatory, not measured traces.','Target runs: direct first then buffered,120seconds each; matched configuration,not identical live inputs.','Figure05 uses interval_ms and excludes first60 frame indices; terminal run.log percentiles use frame_start-to-end cost.','GPU CSVs are header-only; no GPU elapsed data or utilization is inferred.'],'target_records':records,'source_artifact_sha256':hashes}
(OUT/'provenance.json').write_text(json.dumps(provenance,indent=2)+'\n')
print(json.dumps({'assets':[str(p.relative_to(VAULT)) for p in sorted(OUT.iterdir())],'target_records':records},indent=2))
