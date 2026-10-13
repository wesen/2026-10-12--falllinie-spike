// Explicit adapters; original pure bodies are generated separately unchanged.
SEED=42; PC.clear(); FC.clear(); TC.clear(); RC.clear(); CUM.length=1; CUM[0]=0;
function oracle(){
 const r=rng(12345),random=Array.from({length:12},()=>r());
 const coords=[[0,4],[12,150],[-25,365],[60,900],[0,-10]];
 const terrain=coords.map(([x,z])=>{const p={lip:0,stripe:0};return {x,z,h:height(x,z,p),n:normalAt(x,z),paint:p};});
 const A={spin:Math.PI*2,flip:-TAU,t:1.3,sw:false,grabs:new Map([['Indy',{t:.5,tw:.3}]]),ubers:[{name:'Helikopter',dur:1.5}],uber:null};
 G.recent=[]; const scores=[scoreAir(A,true),scoreAir(A,true)];
 return {hashes:[0,1,0xffffffff,0x80000000,0x12345678].map(hashInt),random,terrain,scores,trees:treesOf(3).slice(0,2)};
}
function rowBatch(out){
 const [P,O]=terrainRow(3,4);let k=0;
 for(let i=0;i<P.length;i++){out[k++]=P[i][0];out[k++]=P[i][1];out[k++]=P[i][2];out[k++]=O[i].lip;out[k++]=O[i].stripe;out[k++]=O[i].rel;out[k++]=O[i].u;}
 return out;
}
const rowBuffer=new Float64Array(107*7);
function batch(){return consume(rowBatch(rowBuffer));}
function scalarBridge(){let s=0;for(let i=0;i<749;i++)s+=scalar(i);return s;}
function batchedBridge(){for(let i=0;i<749;i++)rowBuffer[i]=i;return consume(rowBuffer);}
function churn(){for(let i=0;i<1000;i++){let a={};a.self=a;}return 1;}
function noop(){return 1;}
