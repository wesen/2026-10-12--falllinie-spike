#!/usr/bin/env python3
"""GPU008 shadow attribution and bounds; GPU006 host-window conventions."""
import argparse,csv,hashlib,json,math,re,statistics
from collections import defaultdict
from pathlib import Path
from capture_tools import compare
P=Path(__file__).resolve().parent
ORDER=['original','off','bounds','bounds','off','original']

def q(values,f):
    a=sorted(values);x=(len(a)-1)*f;k=int(x);return a[k]+(a[math.ceil(x)]-a[k])*(x-k)

def analyze(root):
    hashes={}
    def text(path):
        b=path.read_bytes();hashes[str(path.relative_to(root))]=hashlib.sha256(b).hexdigest();return b.decode()
    def rows(path):return list(csv.DictReader(text(path).splitlines()))
    manifest=json.loads(text(root/'campaign.json'));assert manifest['complete']
    assert text(root/'deployed.sha256').split()[0]==manifest['binary_sha256']
    expected=[f'{name}: OK' for name in manifest['assets']]
    for file in ['assets-before.txt','assets-after.txt']:assert sorted(text(root/file).splitlines())==sorted(expected)
    expected_dirs=[f'{i:02d}-{mode}' for i,mode in enumerate(ORDER,1)]
    assert sorted(p.name for p in root.glob('[0-9][0-9]-*'))==expected_dirs
    signatures=set();images={};groups=defaultdict(list);pooled=defaultdict(list);result={'cases':{},'groups':{}}
    for name,mode in zip(expected_dirs,ORDER):
        d=root/name
        argv=text(d/'argv.txt').splitlines();assert argv[argv.index('--shadow-mode')+1]==mode
        assert argv[argv.index('--fps')+1]=='0' and '--post-bypass' not in argv
        assert text(d/'status.txt').splitlines()==['game_status=0','probe_status=0']
        for f in ['game.log','conditions-before.txt','conditions-after.txt']:text(d/f)
        r=json.loads(text(d/'receipt.json'))
        assert r['shadow_mode']==mode and r['shadow_non_parity']==(mode=='off')
        assert r['benchmark_replay'] and r['audio_disabled'] and not r['draw_ranges']
        assert r['frames']==600 and r['gl_error']==0 and r['placement_ok'] and r['dual_screen'] and r['render_size']==[512,384]
        assert r['seed']==42 and r['debug_view']==0 and r['palette']==0 and r['replay_work_units']==32
        assert abs(r['replay_step_seconds']-1/60)<1e-7 and r['measurement']['gpu_mode']=='off' and not r['measurement']['post_bypass']
        signatures.add(r['replay_signature'])
        frame_rows=rows(d/'frames.csv');assert [int(x['frame']) for x in frame_rows]==list(range(600))
        frames=[float(x['interval_ms']) for x in frame_rows if int(x['frame'])>=60];assert all(math.isfinite(v) and v>0 for v in frames)
        pooled[mode].extend(frames)
        pairs=[json.loads(line) for line in text(d/'frames.csv.paired.jsonl').splitlines()];samples=[p for p in pairs if p['record']=='sample'];final=pairs[-1]
        assert final['attempted']==final['written']==len(samples)
        assert all(final[k]==0 for k in ['active','dropped','invalid_cpu','invalid_wall'])
        assert all(p['pid']==p['tid']==final['pid'] for p in samples)
        active=[p for p in samples if p['frame']>=60];start=min(p['wall_start_ns'] for p in active);end=max(p['wall_end_ns'] for p in active)
        scopes=defaultdict(list)
        for p in active:scopes[f"{p['stage']}/{p['view']}/{p['pass']}"].append(((p['wall_end_ns']-p['wall_start_ns'])/1e6,(p['cpu_end_ns']-p['cpu_start_ns'])/1e6))
        item={'variant':mode,'non_parity':mode=='off','frames':len(frames),'mean_ms':statistics.mean(frames),'p50_ms':q(frames,.5),'p95_ms':q(frames,.95),'p99_ms':q(frames,.99),'loop_frames_per_second':1000/statistics.mean(frames),'rss_kib':r['rss_kib'],'paired_records':len(samples),'scopes':{k:{'n':len(v),'wall_ms':statistics.mean(x[0] for x in v),'cpu_ms':statistics.mean(x[1] for x in v)} for k,v in scopes.items()},'counter_window_mono_ns':[start,end]}
        counter_rows=rows(d/'counters.csv');log=text(d/'counters.log');m=re.search(r'final rows=(\d+) errors=(\d+) nonfinite=(\d+) stop=(\S+)',log)
        assert m and int(m[1])==len(counter_rows) and m[2]=='0' and m[4]=='ok'
        assert int(m[3])==sum(x['status']=='nonfinite' for x in counter_rows)
        seqs=defaultdict(list)
        for row in counter_rows:seqs[int(row['sequence'])].append(row)
        assert sorted(seqs)==list(range(len(seqs)))
        names={row['counter'] for row in seqs[0]};assert len(names)==13
        previous=None;selected=[];durations=[];offsets=[]
        for seq,group in sorted(seqs.items()):
            assert len(group)==13 and {row['counter'] for row in group}==names
            complete=int(group[0]['complete_ns']);offsets.append((int(group[0]['counter_timestamp_ns'])-complete)/1e9)
            if previous is not None:
                assert complete>previous
                if previous>=start and complete<=end:selected.append(group);durations.append((complete-previous)/1e9)
            previous=complete
        assert durations
        series=defaultdict(list)
        for group in selected:
            for row in group:
                if row['status']=='valid':
                    value=float(row['value']);assert math.isfinite(value);series[row['counter']].append(value)
        item.update({'counter_rows':len(counter_rows),'counter_intervals':len(durations),'counter_host_seconds':sum(durations),'raw_nonfinite_rows':int(m[3]),'selected_nonfinite_rows':sum(row['status']=='nonfinite' for group in selected for row in group),'rates_per_second':{k:sum(series[k])/sum(durations) if len(series[k])==len(durations) else None for k in names if k.endswith(('Cy','By'))},'backend_minus_host_seconds':[min(offsets),max(offsets)],'gpu_frequency_snapshots_hz':sorted({int(g[0]['gpu_freq_snapshot_hz']) for g in selected})})
        images[name]={}
        for kind in ['scene','depth','print','output','lower/scene','lower/depth','lower/print','lower/output']:
            path=d/'capture'/f'{kind}.png';h=hashlib.sha256(path.read_bytes()).hexdigest();hashes[str(path.relative_to(root))]=h;images[name][kind]=h
        result['cases'][name]=item;groups[mode].append(name)
    assert len(signatures)==1 and signatures=={'6081555844518115723'}
    assert len({v['depth'] for v in images.values()})==1,'Depth diagnostic mismatch'
    for mode,names in groups.items():
        assert len(names)==2
        assert images[names[0]]==images[names[1]],f'Repeated endpoint mismatch: {mode}'
        values=pooled[mode];result['groups'][mode]={'runs':names,'frames':len(values),'mean_ms':statistics.mean(values),'p95_ms':q(values,.95),'p99_ms':q(values,.99),'non_parity':mode=='off'}
    for mode,names in groups.items():
        cases=[result['cases'][name] for name in names];seconds=sum(c['counter_host_seconds'] for c in cases)
        result['groups'][mode]['rates_per_second']={k:sum(c['rates_per_second'][k]*c['counter_host_seconds'] for c in cases)/seconds if all(c['rates_per_second'][k] is not None for c in cases) else None for k in cases[0]['rates_per_second']}
        result['groups'][mode]['sequential_view_cpu_ms']=statistics.mean(sum(c['scopes'][f'render/{i}/view']['cpu_ms'] for i in [0,1]) for c in cases)
    first={mode:images[names[0]] for mode,names in groups.items()}
    assert first['original']==first['bounds'],'Candidate endpoint mismatch'
    assert first['original']['print']!=first['off']['print'],'Shadow omission not visible'
    visual=json.loads(text(root/'visual-comparisons.json'));assert len(visual)==5
    assert all(v['different_pixels']==0 for case in visual.values() for v in case.values())
    for key,expected in visual.items():
        a=root/f'visual-{key}-original';b=root/f'visual-{key}-bounds'
        assert compare(a/'capture',b/'capture')==expected
        ra=json.loads(text(a/'receipt.json'));rb=json.loads(text(b/'receipt.json'))
        assert ra['replay_signature']==rb['replay_signature'] and ra['placement_ok'] and rb['placement_ok']
        for d in [a,b]:
            assert text(d/'status.txt').strip()=='game_status=0'
            for image in d.rglob('*.png'):hashes[str(image.relative_to(root))]=hashlib.sha256(image.read_bytes()).hexdigest()
    result.update({'signature':signatures.pop(),'endpoint_hashes':images,'candidate_pixel_equal':True,'visual_comparisons':visual,'repeated_endpoint_images_match':True,'interpretation':'Off is non-parity; bounds is opt-in with finite visual qualification, not universal parity or audio-enabled acceptance.'})
    (root/'summary.json').write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
    (root/'analysis-provenance.json').write_text(json.dumps({'script_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'inputs':hashes,'scope':'Whole-device rates over contained host-poll windows, partial state signature and post-loop internal-target image checks. No calibrated utilization or per-pass GPU milliseconds.'},indent=2,sort_keys=True)+'\n')
    for name,item in result['cases'].items():print(name,'mean/p95',round(item['mean_ms'],4),round(item['p95_ms'],4),'fragment Mcy/s',round(item['rates_per_second']['MaliFragActiveCy']/1e6,2))
    print('groups',json.dumps(result['groups'],indent=2))

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('campaign',type=Path);args=parser.parse_args();analyze(args.campaign.resolve())
