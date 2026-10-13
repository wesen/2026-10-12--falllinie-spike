// Reconcile the retained capture index from actual historical receipts, not a new fetch.
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'../../../../..');
const dir=path.join(root,'ttmp/2026/10/13/FALLLINIE-JS-001--javascript-embedding-research-for-the-native-falllinie-engine/sources');
const file=path.join(dir,'fetch-receipt.json'),archive=path.join(dir,'fetch-receipt-lowercase-retry.json');
const current=JSON.parse(fs.readFileSync(file,'utf8'));
const initial=JSON.parse(fs.readFileSync(path.join(dir,'initial-fetch-failures.json'),'utf8'));
const success=initial.find(r=>r.name==='04-ng-readme'&&r.status==='ok'&&r.url.endsWith('/README.md'));
assert(success,'historical uppercase README success required');
const capture=fs.readFileSync(path.join(dir,'04-ng-readme.md'),'utf8');
assert(capture.includes(success.sha256)&&capture.includes(success.url)&&capture.includes(success.timestamp),'capture must identify the successful receipt');
const old=current.find(r=>r.name==='04-ng-readme');
if(old.status==='failed'){
 if(!fs.existsSync(archive))fs.writeFileSync(archive,fs.readFileSync(file));
 else assert.deepEqual(JSON.parse(fs.readFileSync(archive,'utf8')),current,'do not overwrite conflicting historical retry');
 const reconciled=current.map(r=>r.name==='04-ng-readme'?success:r);
 fs.writeFileSync(file,JSON.stringify(reconciled,null,2)+'\n');
}
console.log('Retained-capture index reconciled with original timestamp/hash; failed retry preserved, no new network fetch.');
