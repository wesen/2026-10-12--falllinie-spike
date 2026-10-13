// Parent acceptance checks for the review's three documentation/provenance findings.
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const root=path.resolve(process.argv[2]||path.resolve(__dirname,'../../../../..'));
const t=path.join(root,'ttmp/2026/10/13/FALLLINIE-JS-001--javascript-embedding-research-for-the-native-falllinie-engine');
const read=f=>fs.readFileSync(f,'utf8');
const initial=JSON.parse(read(path.join(t,'sources/initial-fetch-failures.json')));
const index=JSON.parse(read(path.join(t,'sources/fetch-receipt.json')));
const retry=JSON.parse(read(path.join(t,'sources/fetch-receipt-lowercase-retry.json')));
const ng=index.find(x=>x.name==='04-ng-readme');
assert.equal(ng.status,'ok');assert(ng.url.endsWith('/README.md'));
assert.deepEqual(ng,initial.find(x=>x.name===ng.name&&x.status==='ok'));
const capture=read(path.join(t,'sources/04-ng-readme.md'));
for(const field of ['url','timestamp','sha256'])assert(capture.includes(ng[field]),field);
assert.equal(index.filter(x=>x.status==='failed').length,1);
assert.equal(retry.filter(x=>x.status==='failed').length,2);
assert(retry.some(x=>x.name===ng.name&&x.status==='failed'&&x.url.endsWith('/readme.md')));
const guide=read(path.join(t,'design-doc/01-javascript-runtime-research-and-integration-design.md'));
assert(guide.includes('Historical research context')&&guide.includes('has since been deployed')&&guide.includes('gate failed'));
assert(read(path.join(root,'scripts/2026/10/13/falllinie-js-runtime/01-fetch-sources.py')).includes('v0.17.0/README.md'));
for(const file of ['index.md','scripts/01-research-script-index.md']){
 const full=path.join(t,file);
 for(const m of read(full).matchAll(/\]\(([^)]+)\)/g)){
  if(/^https?:/.test(m[1]))continue;
  assert(fs.existsSync(path.resolve(path.dirname(full),m[1].split('#')[0])),m[1]);
 }
}
console.log('PASS: historical status annotation, actual successful ng metadata, failed retry preservation, corrected fetch URL and ticket/script links. No runtime experiment or device JS executed.');
