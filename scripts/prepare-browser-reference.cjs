const fs=require('node:fs'),path=require('node:path');
const root=path.resolve(__dirname,'..');const out=path.join(root,'build-reference');fs.mkdirSync(out,{recursive:true});
let html=fs.readFileSync(path.join(root,'fixtures/falllinie.html'),'utf8');
html=html.replace(/<script\b[^>]*\bsrc=[^>]*>[\s\S]*?<\/script>/g,'');
html=html.replace('<script>','<script>window.requestAnimationFrame=()=>0;\n');
fs.writeFileSync(path.join(out,'index.html'),html);
console.log('Sanitized local browser reference: no external analytics, no animation scheduling.');
