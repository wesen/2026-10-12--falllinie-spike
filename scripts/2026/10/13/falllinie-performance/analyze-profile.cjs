const fs=require('node:fs');
const path=process.argv[2];if(!path)throw Error('Usage: node scripts/analyze-profile.cjs CPU.csv');
function csv(p){const lines=fs.readFileSync(p,'utf8').trim().split('\n'),header=lines.shift().split(',');return lines.filter(Boolean).map(l=>Object.fromEntries(l.split(',').map((v,i)=>[header[i],Number(v)])));}
const all=csv(path),rows=all.filter(r=>r.frame>=60);
function stats(key,data=rows){const v=data.map(r=>r[key]).filter(Number.isFinite).sort((a,b)=>a-b);function q(p){return v[Math.max(0,Math.ceil(p*v.length)-1)]??null;}return {p50:q(.5),p95:q(.95),p99:q(.99),max:v[v.length-1]??null};}
const stages={};for(const k of Object.keys(rows[0]||{}))if(k.endsWith('_ms'))stages[k]=stats(k);
let gpu=[];if(fs.existsSync(path+'.gpu.csv'))gpu=csv(path+'.gpu.csv');
const result={file:path,warmup_frames:60,measured_frames:rows.length,fps:rows.length*1000/rows.reduce((s,r)=>s+r.interval_ms,0),app_frame_budget_misses_60hz:rows.filter(r=>r.interval_ms>1000/60).length,late_over_18_33ms:rows.filter(r=>r.interval_ms>18.333).length,over_33_33ms:rows.filter(r=>r.interval_ms>33.333).length,stages,streaming_frames:rows.filter(r=>r.stream_ms>0),worst_frames:[...rows].sort((a,b)=>b.interval_ms-a.interval_ms).slice(0,5),gpu:{samples:gpu.length,invalid:gpu.filter(r=>r.disjoint).length,top:stats('gpu_ms',gpu.filter(r=>!r.disjoint&&r.view===0&&r.frame>=60)),bottom:stats('gpu_ms',gpu.filter(r=>!r.disjoint&&r.view===1&&r.frame>=60))},limitations:'Application production intervals, not compositor presentation feedback. Submit timing can include driver backpressure. GPU query instrumentation can perturb tile rendering; compare CPU-only control.'};
console.log(JSON.stringify(result,null,2));
