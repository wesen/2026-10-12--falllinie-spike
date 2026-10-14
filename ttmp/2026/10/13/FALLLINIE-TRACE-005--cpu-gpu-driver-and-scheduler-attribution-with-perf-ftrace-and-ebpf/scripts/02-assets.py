#!/usr/bin/env python3
"""Generate original self-contained figures and a bounded source manifest."""
import argparse
import datetime
import hashlib
import html
import json
from pathlib import Path
import subprocess

SCRIPTS = Path(__file__).resolve().parent
REPO = next(p for p in SCRIPTS.parents if (p / 'src/profiler.cpp').exists())
PERF = REPO / 'ttmp/2026/10/13/FALLLINIE-PERF-004--gl-pass-measurement-and-evidence-driven-native-optimization'

class Figure:
    def __init__(self, title, subtitle, height=440):
        self.parts = [f'<svg xmlns="http://www.w3.org/2000/svg" width="1100" height="{height}" viewBox="0 0 1100 {height}" role="img">', f'<title>{html.escape(title)}</title><desc>{html.escape(subtitle)}</desc>', f'<rect width="1100" height="{height}" fill="#f8fafc"/>']
        self.text(30, 38, title, 25, '#0f172a')
        self.text(30, 67, subtitle, 15, '#475569')
    def text(self, x, y, text, size=18, color='#0f172a'):
        self.parts.append(f'<text x="{x}" y="{y}" font-family="sans-serif" font-size="{size}" fill="{color}">{html.escape(text)}</text>')
    def box(self, x, y, w, h, lines, color='#dbeafe'):
        self.parts.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="8" fill="{color}" stroke="#64748b"/>')
        for i, line in enumerate(lines):
            self.text(x+14, y+28+i*26, line, 17)
    def arrow(self, x1, y1, x2, y2, color='#475569'):
        self.parts.append(f'<path d="M{x1} {y1} L{x2} {y2}" stroke="{color}" stroke-width="2" fill="none"/>')
        # This generator uses horizontal and vertical arrows only.
        if x1 == x2:
            d = -1 if y2 > y1 else 1
            points = f'{x2},{y2} {x2-6},{y2+d*10} {x2+6},{y2+d*10}'
        else:
            d = -1 if x2 > x1 else 1
            points = f'{x2},{y2} {x2+d*10},{y2-6} {x2+d*10},{y2+6}'
        self.parts.append(f'<polygon points="{points}" fill="{color}"/>')
    def save(self, dest):
        dest.write_text('\n'.join(self.parts+['</svg>'])+'\n')

def generate(dest):
    dest.mkdir(parents=True, exist_ok=True)
    f=Figure('Measurement domains', 'Conceptual architecture; not a captured trace')
    f.box(30,110,315,125,['Application scope','Wall duration W','frame / view / pass'])
    f.box(390,110,315,125,['Render thread','Charged CPU duration C','Running / blocked / runnable'],'#dcfce7')
    f.box(750,110,320,125,['GPU workload','Elapsed query result','Busy counters: separate gate'],'#fef3c7')
    f.arrow(345,172,390,172); f.arrow(705,172,750,172)
    f.box(170,290,760,100,['Correlate identities; measure CPU-clock alignment.','CPU entry and exit do not identify GPU execution boundaries.'],'#e2e8f0')
    f.arrow(548,235,548,290)
    f.save(dest/'01-measurement-domains.svg')

    f=Figure('One output scope: 10 ms wall, 3 ms CPU', 'Constructed teaching example; no target scheduler trace was collected',470)
    segments=[(0,1,'CPU','#86efac'),(1,5,'Blocked: 4 ms','#fcd34d'),(5,6.5,'Runnable','#fda4af'),(6.5,8.5,'CPU: 2 ms','#86efac'),(8.5,10,'Runnable','#fda4af')]
    for a,b,label,color in segments:
        f.box(50+a*100,125,(b-a)*100,80,[label],color)
    for n in [0,1,5,6.5,8.5,10]:
        f.text(45+n*100,235,str(n),16)
    f.text(45,265,'Milliseconds from scope entry',16)
    f.box(130,295,640,65,['ioctl 0.8–7.2 ms: CPU + blocking + runnable delay'],'#dbeafe')
    f.text(50,405,'Off-CPU = 4 ms blocked + 3 ms runnable; not automatically 7 ms GPU wait.',19)
    f.save(dest/'02-example-timeline.svg')

    f=Figure('Scheduler state reconstruction', 'Proposed analysis; actual payload encodings and clock ordering must be verified',480)
    f.box(35,125,260,85,['RUNNING','Scheduled on one CPU'],'#dcfce7')
    f.box(420,125,260,85,['BLOCKED','Not eligible to execute'],'#fef3c7')
    f.box(800,125,265,85,['RUNNABLE','Eligible, not executing'],'#ffe4e6')
    f.arrow(295,166,420,166);f.text(305,112,'block',15)
    f.arrow(680,166,800,166);f.text(697,112,'wakeup',15)
    f.parts.append('<path d="M165 210 V270 H932 V210" stroke="#475569" stroke-width="2" fill="none"/>')
    f.arrow(932,235,932,210)
    f.text(365,258,'Runnable switch-out → RUNNABLE',17)
    f.parts.append('<path d="M932 125 V90 H165 V125" stroke="#475569" stroke-width="2" fill="none"/>')
    f.arrow(165,100,165,125)
    f.text(412,85,'Switch-in → RUNNING',16)
    f.box(135,315,830,105,['Exit: close and retire this task lifetime.','Missing events or initial state: retain UNKNOWN, do not invent a wait.'],'#e2e8f0')
    f.save(dest/'03-scheduler-states.svg')

    f=Figure('Correlation records and clock domains', 'Proposed fields; elapsed GPU duration is not a CPU-aligned timestamp pair',465)
    f.box(30,110,480,125,['CPU scope: run, task lifetime, frame, view, pass','Wall / thread-CPU entry and exit','Clock identity, validity, marker uncertainty'])
    f.box(590,110,480,125,['Scheduler: task lifetime, CPU, event','Timestamp, state, waker when available','Loss counters and cross-CPU clock'],'#dcfce7')
    f.arrow(510,173,590,173)
    f.box(30,290,480,120,['GPU query: originating frame, view, pass','Elapsed duration; valid/disjoint/pending','Result may arrive in a later CPU frame'],'#fef3c7')
    f.box(590,290,480,120,['Driver join, if available','Verified context / submission / fence identity','Do not infer dependency from time overlap'],'#e2e8f0')
    f.arrow(270,235,270,290);f.arrow(830,235,830,290);f.arrow(510,350,590,350)
    f.save(dest/'04-correlation-records.svg')

    f=Figure('Finite experiment gates', 'Each next step requires authorization, bounded overhead, complete provenance and a remaining question',550)
    rows=[('A  Paired clocks','How much charged main-thread execution?','#dbeafe'),('B  perf samples','Which application / driver code executes?','#dcfce7'),('C  Scheduler trace','Blocked or runnable delay?','#fef3c7'),('D  Targeted driver / BPF','Which verified dependency remains unresolved?','#ffe4e6')]
    for i,(name,question,color) in enumerate(rows):
        y=105+i*88
        f.box(35,y,330,66,[name],color);f.box(435,y,625,66,[question],color);f.arrow(365,y+33,435,y+33)
        if i<3:f.arrow(195,y+66,195,y+88)
    f.box(100,470,900,55,['GPU hardware counters and vendor tools require a separate capability gate.'],'#e2e8f0')
    f.save(dest/'05-experiment-gates.svg')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('assets',type=Path)
    args=parser.parse_args()
    generate(args.assets)
    records=[]
    files=[REPO/'src'/p for p in ['profiler.cpp','profiler.hpp','main.cpp','renderer.cpp','game_renderer.cpp','audio_timing.hpp']]
    for case in ['07-live-off','08-live-passes']:
        files += [PERF/'scripts/evidence/target'/case/p for p in ['metadata.txt','receipt.json','analysis.json','stdout.log','frames.csv','frames.csv.passes.csv','frames.csv.gpu.csv','frames.csv.stats.csv','binary.sha256']]
    for path in files:
        body=path.read_bytes()
        rel=str(path.relative_to(REPO))
        tracked=subprocess.run(['git','ls-files','--error-unmatch','--',rel],cwd=REPO,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL).returncode==0
        records.append({'path':rel,'bytes':len(body),'sha256':hashlib.sha256(body).hexdigest(),'tracked_at_read':tracked})
    analysis=json.loads((PERF/'scripts/evidence/target/08-live-passes/analysis.json').read_text())
    manifest={
      'created_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
      'repository':str(REPO),
      'runtime_source_revision':'532081682d36f1eaf5eb0daf9a45b17f4b60f136',
      'ticket_base_revision':'7ad1c7d6595afde830434d597d337605c6652e5b',
      'limits':'Parent-collected PERF004 target evidence was untracked at read time. Hashes identify bytes, not immutable committed publication. No device, perf, ftrace or BPF command was run for this report.',
      'files':records,
      'illustrative_excerpt':{
        'case':'08-live-passes','warmup_frames':60,
        'interval':analysis['cpu_intervals_independent_not_additive']['interval_ms'],
        'top_cpu_wall':{p:analysis['cpu_scopes_nested_not_additive'][f'0/{p} (parent=view)'] for p in ['scene','post','output']},
        'gpu_elapsed':analysis['gpu_scopes_no_cross_mode_sum'],
        'query_stats':analysis['whole_run_query_stats'],
      },
      'primary_sources':json.loads((SCRIPTS/'research-provenance.json').read_text()),
      'figures':[{'file':p.name,'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'origin':'Original SVG generated by TRACE005/scripts/02-assets.py; conceptual except explicitly constructed 10ms teaching example.'} for p in sorted(args.assets.glob('*.svg'))],
    }
    (args.assets/'provenance.json').write_text(json.dumps(manifest,indent=2)+'\n')
    (SCRIPTS/'evidence-provenance.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(f'Generated 5 SVG figures and provenance; hashed {len(records)} local inputs.')

if __name__=='__main__':main()
