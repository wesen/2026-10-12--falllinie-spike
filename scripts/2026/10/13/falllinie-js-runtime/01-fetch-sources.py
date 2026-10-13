#!/usr/bin/env python3
"""Capture known upstream URLs, not a search. Raw downloads stay ignored."""
import datetime, hashlib, html, json, pathlib, subprocess, urllib.request, sys
ROOT=pathlib.Path(__file__).resolve().parents[5]
HERE=pathlib.Path(__file__).resolve().parent
T=ROOT/'ttmp/2026/10/13/FALLLINIE-JS-001--javascript-embedding-research-for-the-native-falllinie-engine'
URLS={
 '01-bellard-home':'https://bellard.org/quickjs/',
 '02-bellard-manual':'https://bellard.org/quickjs/quickjs.html',
 '03-bellard-changelog':'https://bellard.org/quickjs/Changelog',
 '04-ng-readme':'https://raw.githubusercontent.com/quickjs-ng/quickjs/v0.17.0/readme.md',
 '05-ng-api':'https://raw.githubusercontent.com/quickjs-ng/quickjs/v0.17.0/quickjs.h',
 '06-ng-build':'https://raw.githubusercontent.com/quickjs-ng/quickjs/v0.17.0/CMakeLists.txt',
 '07-duktape-home':'https://duktape.org/',
 '08-duktape-guide':'https://duktape.org/guide.html',
 '09-v8-embed':'https://v8.dev/docs/embed',
 '10-v8-cross':'https://v8.dev/docs/cross-compile-arm',
 '11-jsc-api':'https://raw.githubusercontent.com/WebKit/WebKit/main/Source/JavaScriptCore/API/JSContextRef.h',
 '12-jsc-values':'https://raw.githubusercontent.com/WebKit/WebKit/main/Source/JavaScriptCore/API/JSValueRef.h',
 '13-jsc-typed-arrays':'https://raw.githubusercontent.com/WebKit/WebKit/main/Source/JavaScriptCore/API/JSTypedArray.h',
 '14-quickjspp-readme':'https://raw.githubusercontent.com/ftk/quickjspp/01cdd3047ced48265b127790848a0ca88204f2c7/readme.md',
 '15-webkit-port':'https://docs.webkit.org/Ports/WebKitGTK%20and%20WPE%20WebKit/DependenciesPolicy.html',
 '16-v8-version':'https://raw.githubusercontent.com/v8/v8/main/include/v8-version.h',
 '17-webkit-release':'https://webkitgtk.org/',
 '18-ng-releases':'https://api.github.com/repos/quickjs-ng/quickjs/releases/latest',
 '19-duktape-timeout':'https://wiki.duktape.org/HowtoTimeout.html',
 '20-duktape-modules':'https://wiki.duktape.org/Modules.html',
 '21-duktape-sandbox':'https://wiki.duktape.org/Sandboxing.html',
 '22-duktape-features':'https://wiki.duktape.org/PostEs5Features.html',
 '23-jsc-limits-private':'https://raw.githubusercontent.com/WebKit/WebKit/9f4a36c5c53a4fa67421fa25fdf5194572d29795/Source/JavaScriptCore/API/JSContextRefPrivate.h',
 '24-v8-isolate-api':'https://raw.githubusercontent.com/v8/v8/cce823212e9a36b5e22c72bb7edc4e017045bd5e/include/v8-isolate.h',
 '25-v8-license':'https://raw.githubusercontent.com/v8/v8/cce823212e9a36b5e22c72bb7edc4e017045bd5e/LICENSE',
}
W=HERE/'.work/captures'; W.mkdir(parents=True,exist_ok=True)
(T/'sources').mkdir(exist_ok=True)
receipt_path=T/'sources/fetch-receipt.json'
receipts=json.loads(receipt_path.read_text()) if receipt_path.exists() else []
for name,url in URLS.items():
 if len(sys.argv)>1 and name not in sys.argv[1:]: continue
 receipts=[r for r in receipts if r['name']!=name]
 stamp=datetime.datetime.now(datetime.timezone.utc).isoformat()
 try:
  with urllib.request.urlopen(url,timeout=40) as r: data=r.read(); final=r.url
  raw=W/(name+'.txt'); raw.write_bytes(data)
  # Defuddle consumes the saved file, so the body and receipt identify one fetch.
  if data.lstrip().startswith(b'<'):
   p=subprocess.run(['defuddle','parse',str(raw),'--md'],capture_output=True,text=True)
   if p.returncode: raise RuntimeError('defuddle: '+p.stderr)
   text=p.stdout
   if len(data)>100000: text='**Extraction warning:** Defuddle stdout is abbreviated for this long page; this is not a complete manual capture. Raw hash identifies the full fetched body. Use smaller topical captures for detailed API claims.\n\n'+text
  else: text='```text\n'+data.decode('utf-8')+'\n```\n'
  sha=hashlib.sha256(data).hexdigest()
  doc=f'''---
Title: "Upstream capture {name}"
Ticket: FALLLINIE-JS-001
Status: active
Topics: [research]
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: ["{url}"]
Summary: "Fetched upstream evidence; see capture timestamp and hash."
LastUpdated: {stamp}
WhatFor: "Runtime comparison evidence."
WhenToUse: "Audit claims against fetched upstream material."
---

Fetched {stamp}; URL {url}; final URL {final}; raw SHA256 `{sha}`.
Mutable main/release pages are captures, not immutable release pins. Defuddle Markdown may omit page navigation; raw body can be recaptured with this script.

{text}
'''
  (T/'sources'/(name+'.md')).write_text(doc)
  receipts.append(dict(name=name,url=url,final=final,timestamp=stamp,sha256=sha,bytes=len(data),status='ok'))
  print(name,len(data),sha)
 except Exception as e:
  receipts.append(dict(name=name,url=url,timestamp=stamp,status='failed',error=str(e)))
  print(name,'FAILED',str(e))
(T/'sources/fetch-receipt.json').write_text(json.dumps(receipts,indent=2)+'\n')
if any(r['status']!='ok' for r in receipts): raise SystemExit(1)
