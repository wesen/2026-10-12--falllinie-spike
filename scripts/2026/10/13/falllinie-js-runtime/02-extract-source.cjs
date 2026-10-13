// Extract unchanged source bodies; wrappers are explicit and outside the fixture.
const fs=require('node:fs'),crypto=require('node:crypto'),vm=require('node:vm'),path=require('node:path');
const root=path.resolve(__dirname,'../../../../..'),out=path.join(__dirname,'.work');fs.mkdirSync(out,{recursive:true});
const html=fs.readFileSync(path.join(root,'fixtures/falllinie.html'),'utf8');
const cut=(a,b)=>{const i=html.indexOf(a),j=html.indexOf(b,i);if(i<0||j<i)throw Error(`missing anchor ${a}`);return html.slice(i,j);};
const pure=cut('const TAU =','// WebGL 2 renderer:')+'\n'+cut('const GRABS =','const inp =')+'\nconst G={recent:[]};\n'+cut('function trickName(','function addToCombo(')+'\n'+cut('const UCOLS =','function finishTerrain(');
fs.writeFileSync(path.join(out,'pure.js'),pure);
fs.writeFileSync(path.join(out,'inline.js'),cut('"use strict";','</script>'));
const wrapper=fs.readFileSync(path.join(__dirname,'probe.js'),'utf8');
const ctx=vm.createContext({});vm.runInContext(pure+'\n'+wrapper,ctx,{timeout:2000});
fs.writeFileSync(path.join(out,'node-oracle.json'),vm.runInContext('JSON.stringify(oracle())',ctx)+'\n');
fs.writeFileSync(path.join(out,'fixture-sha256.txt'),crypto.createHash('sha256').update(html).digest('hex')+'\n');
console.log('extracted unchanged pure source bytes',Buffer.byteLength(pure));
