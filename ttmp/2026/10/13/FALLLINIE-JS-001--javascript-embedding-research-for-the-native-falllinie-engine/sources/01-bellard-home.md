---
Title: "Upstream capture 01-bellard-home"
Ticket: FALLLINIE-JS-001
Status: active
Topics: [research]
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: ["https://bellard.org/quickjs/"]
Summary: "Fetched upstream evidence; see capture timestamp and hash."
LastUpdated: 2026-10-13T14:14:05.573341+00:00
WhatFor: "Runtime comparison evidence."
WhenToUse: "Audit claims against fetched upstream material."
---

Fetched 2026-10-13T14:14:05.573341+00:00; URL https://bellard.org/quickjs/; final URL https://bellard.org/quickjs/; raw SHA256 `5acc651e8d32fb01a3d6c28e89b5fd3b104cc09012b8d9c7d10bf61be4aded7e`.
Mutable main/release pages are captures, not immutable release pins. Defuddle Markdown may omit page navigation; raw body can be recaptured with this script.

## News

- 2026-06-04:
	- New release ([Changelog](Changelog)). It is 42% faster than the previous release based on the bench-v8 (aka v8-v7) score.
- 2025-09-13:
	- New release ([Changelog](Changelog))
- 2025-04-26:
	- New release ([Changelog](Changelog)). The bignum extensions and the qjscalc application were removed to simplify the code. The [BFCalc calculator](/libbf) ([web version](http://numcalc.com)) can be used as a replacement for qjscalc.

## Introduction

QuickJS is a small and embeddable Javascript engine. It supports the [ES2025](https://tc39.github.io/ecma262/2025) specification.

Main Features:

- Small and easily embeddable: just a few C files, no external dependency, 367 KiB of x86 code for a simple hello world program.
- Fast interpreter with very low startup time: runs the [ECMAScript Test Suite](https://github.com/tc39/test262) in about 2 minutes on a single core of a desktop PC. The complete life cycle of a runtime instance completes in less than 300 microseconds.
- Almost complete [ES2025](https://tc39.github.io/ecma262/2023) support including modules, asynchronous generators and full Annex B support (legacy web compatibility).
- Passes nearly 100% of the ECMAScript Test Suite tests when selecting the ES2025 features (see [test262.fyi](https://test262.fyi)).
- Can compile Javascript sources to executables with no external dependency.
- Garbage collection using reference counting (to reduce memory usage and have deterministic behavior) with cycle removal.
- Command line interpreter with contextual colorization implemented in Javascript.
- Small built-in standard library with C library wrappers.

## Online Demo

qjs can be run in [JSLinux](https://bellard.org/jslinux/vm.html?url=alpine-x86.cfg).

## Benchmarks

- [JavaScript engines zoo](https://zoo.js.org/?arch=amd64&v8=true)
- [Boa benchmarks](https://boajs.dev/benchmarks)
- [ahaoboy/js-engine-benchmark](https://github.com/ahaoboy/js-engine-benchmark)

## Documentation

QuickJS documentation: [HTML version](quickjs.html), [PDF version](quickjs.pdf).

## Download

- QuickJS source code: [quickjs-2026-06-04.tar.xz](quickjs-2026-06-04.tar.xz)
- QuickJS extras (contain the unicode files needed to rebuild the unicode tables and the bench-v8 benchmark): [quickjs-extras-2026-06-04.tar.xz](quickjs-extras-2026-06-04.tar.xz)
- Official [GitHub repository](https://github.com/bellard/quickjs).
- Binary releases are available [here](binary_releases).
- [Cosmopolitan](https://github.com/jart/cosmopolitan) binaries running on Linux, Mac, Windows, FreeBSD, OpenBSD, NetBSD for both the ARM64 and x86\_64 architectures: [quickjs-cosmo-2026-06-04.zip](binary_releases/quickjs-cosmo-2026-06-04.zip).
- Typescript compiler compiled with QuickJS: [quickjs-typescript-5.9.3-linux-x86.tar.xz](quickjs-typescript-5.9.3-linux-x86.tar.xz)
- Babel compiler compiled with QuickJS: [quickjs-babel-linux-x86.tar.xz](quickjs-babel-linux-x86.tar.xz)

## Sub-projects

QuickJS embeds the following C libraries which can be used in other projects:
- **libregexp**: small and fast regexp library fully compliant with the Javascript ES2023 specification.
- **libunicode**: small unicode library supporting case conversion, unicode normalization, unicode script queries, unicode general category queries and all unicode binary properties.
- **dtoa**: small library implementing float64 printing and parsing.

## Links

- [Micro QuickJS](https://github.com/bellard/mquickjs): a Javascript engine for microcontrollers.

## Licensing

QuickJS is released under the [MIT license](https://opensource.org/licenses/MIT).

Unless otherwise specified, the QuickJS sources are copyright Fabrice Bellard and Charlie Gordon.

---

Fabrice Bellard - [https://bellard.org/](https://bellard.org)

