// Additional explicit adapters ONLY; extracted source functions stay unchanged.
const scoringInput={spin:Math.PI*2,flip:-TAU,t:1.3,sw:false,
 grabs:new Map([['Indy',{t:.5,tw:.3}]]),ubers:[{name:'Helikopter',dur:1.5}],uber:null};
function sourceRules(){
 let sum=0;
 for(let i=0;i<128;i++){
  G.recent=[];const first=scoreAir(scoringInput,true),second=scoreAir(scoringInput,true);
  sum+=first.pts+second.pts+hashInt(i)+rng(i)();
 }
 G.recent=[];return sum;
}
// Fixed bounded retained state (~128KiB buffers + small object overhead).
let retained=[];
function retainState(){retained=Array.from({length:128},(_,i)=>({i,data:new Float64Array(128)}));return retained.length;}
function dropState(){retained=[];return 0;}
function churn(){for(let i=0;i<500;i++){const a={data:new Array(16).fill(i)};a.self=a;}return 1;}
function protocolTests(){
 const detached=new Float64Array(3);detachForProbe(detached.buffer);
 let rejected=0;
 for(const a of [new Float32Array(3),new BigInt64Array(3),new Float64Array([NaN]),new Float64Array([Infinity]),new Float64Array(750),new Float64Array(0),detached,{},new DataView(new ArrayBuffer(8))]){
  try{consume(a);}catch(e){rejected++;}
 }
 if(rejected!==9)throw Error('bridge rejection count '+rejected);
 if(consume(new Float64Array([99,1,2,3]).subarray(1))!==6)throw Error('offset');
 const cp=new Float64Array([1,2,3]);if(consume(cp)!==6)throw Error('copy');cp[0]=500;
 if(copiedSum()!==6)throw Error('native copy changed');
 let overflowRejected=false;
 try{consume(new Float64Array([Number.MAX_VALUE,Number.MAX_VALUE]));}catch(e){overflowRejected=true;}
 if(!overflowRejected)throw Error('native sum overflow accepted');
 if(copiedSum()!==6)throw Error('overflow mutated copied sum');
 try{scalar('1');throw Error('accepted string');}catch(e){if(e.message==='accepted string')throw e;}
 rejected=0;
 for(const r of [null,{pts:NaN,name:'x',ubers:1},{pts:1.2,name:'x',ubers:1},{pts:-1,name:'x',ubers:1},{pts:1,name:7,ubers:1},{pts:1,name:'x'.repeat(257),ubers:1},{pts:1,name:'x',ubers:NaN},{pts:1,name:'x',ubers:17},{get pts(){throw Error('getter exception');},name:'x',ubers:1}]){try{acceptScore(r);}catch(e){rejected++;}}
 if(rejected!==9)throw Error('malformed output rejection');
 if(acceptScore({pts:10260,name:'FS Rodeo 360 · Indy · Helikopter',ubers:1})!==10260)throw Error('valid output');
 G.recent=[];if(acceptScore(scoreAir(scoringInput,true))!==10260)throw Error('source output');G.recent=[];
 return rejected+10;
}
let jobCount=0;
function scheduleJob(){Promise.resolve().then(()=>jobCount++);return jobCount;}
function jobsObserved(){return jobCount;}
function intentionalException(){throw Error('intentional qualification exception');}
