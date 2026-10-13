// Bake the ORIGINAL base pose, two-bone IK and drawRider primitives, not a replacement model.
const fs=require('node:fs'),vm=require('node:vm'),path=require('node:path');
const root=path.resolve(__dirname,'..'),html=fs.readFileSync(path.join(root,'fixtures/falllinie.html'),'utf8'),ctx=vm.createContext({});
vm.runInContext(html.slice(html.indexOf('const TAU ='),html.indexOf('// WebGL 2 renderer:')),ctx);
vm.runInContext(html.slice(html.indexOf('class MB {'),html.indexOf('function upload('))+'\nfunction upload(m){return m;}\n'+html.slice(html.indexOf('function meshCylinder('),html.indexOf('const GRAV =')),ctx);
vm.runInContext(html.slice(html.indexOf('const GRAV ='),html.indexOf('// sound: a lofi')),ctx);
vm.runInContext('const CYL=meshCylinder(10),SPH=meshSphere(14,9),BOX=meshBox(),BOARD=meshBoard();\n'+html.slice(html.indexOf('function drawSeg('),html.indexOf('function buildParticles(')),ctx);
vm.runInContext(`let baked;function drawMesh(m,M,alb,mat){const offset=baked.v.length/11,first=baked.i.length,N=nMat(M);for(let j=0;j<m.v.length;j+=11){const a=m.v.slice(j,j+11),p=a.slice(0,3),n=a.slice(3,6);const P=[0,1,2].map(r=>M[r]*p[0]+M[4+r]*p[1]+M[8+r]*p[2]+M[12+r]);const NN=norm([0,1,2].map(r=>N[r]*n[0]+N[3+r]*n[1]+N[6+r]*n[2]));baked.v.push(...P,...NN,alb>=0?alb:a[6],...a.slice(7));}baked.i.push(...m.i.map(i=>i+offset));baked.groups.push([first,m.i.length,mat]);}`,ctx);
const poses=vm.runInContext(`[0,.5,1].map(c=>{R.p=[0,0,0];R.qVis=QI();R.q=QI();R.mode='ground';R.air=null;R.crouch=c;R.lean=0;R.steerVis=0;R.grabAmt=0;computeRig();baked={v:[],i:[],groups:[]};drawRider();return baked;})`,ctx);
const first=poses[0],header=new Uint32Array([3,first.v.length/11,first.i.length,first.groups.length]);
const chunks=[Buffer.from(header.buffer),Buffer.from(new Uint32Array(first.i).buffer),Buffer.from(new Uint32Array(first.groups.flat()).buffer)];
for(const p of poses){if(p.v.length!==first.v.length)throw Error('pose topology mismatch');chunks.push(Buffer.from(new Float32Array(p.v).buffer));}
fs.writeFileSync(path.join(root,'assets/rider-poses.bin'),Buffer.concat(chunks));console.log('Original rider baked: '+header.join(',')+' (poses, vertices, indices, draw groups)');
