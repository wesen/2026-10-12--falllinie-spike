const fs=require('node:fs'),path=require('node:path');
const root=path.resolve(__dirname,'..');const base=path.join(root,'evidence/qualification');
const cases=fs.readdirSync(base,{withFileTypes:true}).filter(x=>x.isDirectory()).map(x=>({name:x.name,...JSON.parse(fs.readFileSync(path.join(base,x.name,'receipt.json')))}));
for(const c of cases){if(c.gl_error!==0||!c.placement_ok||c.measured_frames<100)throw new Error('Unqualified run '+c.name);}
const summary={scope:'Native fixed-patch graphics qualification only; CPU submission plus swap, not GPU time',timing_target_ms:33.3,cases:cases.map(c=>({name:c.name,size:c.render_size,cells:c.cells,p95_ms:c.frame_ms.p95,p99_ms:c.frame_ms.p99,build_ms:c.terrain_build_ms,peak_rss_kib:c.rss_kib.peak,end_rss_kib:c.rss_kib.end,frames:c.frames,timing_target_met:c.timing_target_met,gl_error:c.gl_error,depth_sampler_highp:c.depth_sampler_highp,rss_samples:c.rss_samples})),all_normal_cases_within_timing_target:cases.filter(c=>c.name!=='resize-stress'&&c.name!=='legacy-depth').every(c=>c.timing_target_met)};
fs.writeFileSync(path.join(root,'evidence/qualification-summary.json'),JSON.stringify(summary,null,2)+'\n');
console.log(JSON.stringify(summary,null,2));
