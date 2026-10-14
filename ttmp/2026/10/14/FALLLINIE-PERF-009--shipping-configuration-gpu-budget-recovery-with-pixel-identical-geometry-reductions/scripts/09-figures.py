#!/usr/bin/env python3
"""Acceptance figure: live autoplay interval distributions, original vs defaults.
Usage: 09-figures.py ACCEPTANCE_DIR OUT_SVG"""
import csv,sys
from pathlib import Path
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
SURFACE,INK,INK2,GRID,S1,S2='#fcfcfb','#0b0b0b','#52514e','#e4e3df','#2a78d6','#eb6834'
plt.rcParams.update({'font.family':'DejaVu Sans','font.size':10,'axes.facecolor':SURFACE,'figure.facecolor':SURFACE,'savefig.facecolor':SURFACE,'axes.spines.top':False,'axes.spines.right':False,'axes.edgecolor':INK2,'xtick.color':INK2,'ytick.color':INK2,'svg.fonttype':'none'})
root=Path(sys.argv[1])
def load(names):return [float(r['interval_ms']) for n in names for r in csv.DictReader(open(root/n/'frames.csv')) if int(r['frame'])>=60]
panels=[('Live autoplay, previous behaviour (cases 05, 08)',load(['05-original-live','08-original-live']),S2),('Live autoplay, PERF-009 defaults (cases 06, 07)',load(['06-defaults-live','07-defaults-live']),S1)]
fig,axes=plt.subplots(2,1,figsize=(8.6,5.2),sharex=True)
for ax,(title,iv,color) in zip(axes,panels):
 ax.grid(True,color=GRID);ax.set_axisbelow(True)
 bins=[10+.5*i for i in range(61)];ax.hist(iv,bins=bins,weights=[100/len(iv)]*len(iv),color=color,edgecolor=SURFACE,linewidth=.6)
 for k in (1,2):ax.axvline(k*1000/60,color=INK2,linestyle='--',linewidth=1)
 long=sum(x>=25 for x in iv)/len(iv);mean=sum(iv)/len(iv)
 ax.set_title(title,loc='left',fontsize=10,color=INK);ax.set_ylabel('% per 0.5 ms')
 ax.text(.98,.88,f'n={len(iv)}  mean {mean:.2f} ms  ≥25 ms: {100*long:.1f}%',transform=ax.transAxes,ha='right',va='top',fontsize=9,color=INK2)
axes[1].set_xlabel('Frame interval (ms); dashed: 16.67 and 33.33 ms');axes[1].set_xlim(10,40)
fig.suptitle('Live dual-view autoplay with muted audio, 384×288 internal',x=.07,ha='left',color=INK,fontsize=12)
fig.tight_layout();fig.savefig(sys.argv[2])
