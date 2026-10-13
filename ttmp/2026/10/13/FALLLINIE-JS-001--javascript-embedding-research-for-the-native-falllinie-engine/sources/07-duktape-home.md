---
Title: "Upstream capture 07-duktape-home"
Ticket: FALLLINIE-JS-001
Status: active
Topics: [research]
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: ["https://duktape.org/"]
Summary: "Fetched upstream evidence; see capture timestamp and hash."
LastUpdated: 2026-10-13T14:14:08.833764+00:00
WhatFor: "Runtime comparison evidence."
WhenToUse: "Audit claims against fetched upstream material."
---

Fetched 2026-10-13T14:14:08.833764+00:00; URL https://duktape.org/; final URL https://duktape.org/; raw SHA256 `7f02d90331d5b59385c8d74405a0ffc15b80a3f38c4c2ee2ead11ec7242c44c5`.
Mutable main/release pages are captures, not immutable release pins. Defuddle Markdown may omit page navigation; raw body can be recaptured with this script.

[![Fork me on GitHub](https://s3.amazonaws.com/github/ribbons/forkme_right_gray_6d6d6d.png)](https://github.com/svaarala/duktape)

---

Duktape is an **embeddable Javascript** engine, with a focus on **portability** and compact **footprint**.

Duktape is easy to integrate into a C/C++ project: add `duktape.c`,`duktape.h`, and `duk_config.h` to your build, and use the Duktape API to call ECMAScript functions from C code and vice versa.

---

## Main features

- Embeddable, portable, compact: can run on platforms with 160kB flash and 64kB RAM
- [ECMAScript E5/E5.1](http://www.ecma-international.org/ecma-262/5.1/), with some semantics updated from ES2015+
- Partial support for [ECMAScript 2015 (E6)](http://www.ecma-international.org/ecma-262/6.0/index.html) and [ECMAScript 2016 (E7)](http://www.ecma-international.org/ecma-262/7.0/index.html), see [Post-ES5 feature status](http://wiki.duktape.org/PostEs5Features.html) and [kangax/compat-table](https://kangax.github.io/compat-table)
- ES2015 [TypedArray](https://www.khronos.org/registry/typedarray/specs/latest/) and [Node.js Buffer](https://nodejs.org/docs/v6.9.1/api/buffer.html) bindings
- [CBOR](http://cbor.io/) bindings
- [Encoding API](https://encoding.spec.whatwg.org/#api) bindings based on the WHATWG Encoding Living Standard
- [performance.now()](https://www.w3.org/TR/hr-time/#dom-performance-now)
- Built-in debugger
- Built-in regular expression engine
- Built-in Unicode support
- Minimal, retargetable platform dependencies
- Combined reference counting and mark-and-sweep garbage collection with finalization
- Coroutines
- Property virtualization using a subset of ECMAScript ES2015 Proxy object
- Bytecode dump/load for caching compiled functions
- Distributable includes an optional logging framework, CommonJS-based module loading implementations, etc
- Liberal license (MIT)

---

## Code and RAM footprint

For a "Hello world" [example](https://github.com/svaarala/duktape/blob/master/util/index_page_sizes.sh):

| Config | Code footprint (kB) | Startup RAM (kB) |
| --- | --- | --- |
| thumb default | 148 | 78 |
| thumb lowmem | 96 | 27 |
| thumb full lowmem | 119 | 1.5 |
| x86 default | 187 | 78 |
| x86 lowmem | 124 | 27 |
| x86 full lowmem | 148 | 1.5 |

See [GCC options](https://github.com/svaarala/duktape/blob/master/doc/low-memory.rst#optimizing-code-footprint) for minimizing code footprint. Full [lowmem](https://github.com/svaarala/duktape/blob/master/doc/low-memory.rst) uses "pointer compression" and ROM-based strings/objects. ROM-based strings/objects can also be used without other low memory options.

---

## Current status

- Stable

---

## Support

- Duktape Wiki: [wiki.duktape.org](http://wiki.duktape.org)
- User community Q&A: Stack Overflow [duktape](http://stackoverflow.com/questions/tagged/duktape) tag
- Bugs and feature requests: [GitHub issues](https://github.com/svaarala/duktape/issues)
- General discussion: IRC `#duktape` on `chat.freenode.net` ([webchat](https://webchat.freenode.net/))

---

## Some projects using Duktape

See: [Projects using Duktape](http://wiki.duktape.org/ProjectsUsingDuktape.html).

If you're using Duktape in your project, send an e-mail or open a GitHub issue to be added to the list.

---

## Similar engines

There are multiple Javascript engines targeting similar use cases as Duktape, at least:

- [Espruino](https://github.com/espruino/Espruino) (MPL v2.0)
- [JerryScript](http://jerryscript.net/) (Apache License v2.0)
- [MuJS](http://mujs.com/) (Affero GPL)
- [quad-wheel](https://code.google.com/p/quad-wheel/) (MIT License)
- [QuickJS](https://bellard.org/quickjs/) (MIT License)
- [tiny-js](https://github.com/gfwilliams/tiny-js) (MIT license)
- [v7](https://github.com/cesanta/v7) (GPL v2.0)

Also see [List of ECMAScript engines](https://en.wikipedia.org/wiki/List_of_ECMAScript_engines).

---

## 1 Add to build

(See [Getting started](/guide#gettingstarted) for a more detailed introduction.)

Add Duktape C source and header to your build. Any build system can be used. The distributable contains an example Makefile for reference. In the simplest case:

```
$ gcc -std=c99 -otest test.c duktape.c -lm
$ ./test
1+2=3
```

To [customize Duktape configuration](http://wiki.duktape.org/Configuring.html), here to disable ECMAScript 6 `Proxy` object support:

```
$ python2 duktape-2.6.0/tools/configure.py --output-directory src-duktape \
      -UDUK_USE_ES6_PROXY
$ ls src-duktape/
duk_config.h  duk_source_meta.json  duktape.c  duktape.h
$ gcc -std=c99 -otest -Isrc-duktape \
      test.c src-duktape/duktape.c -lm
$ ./test
1+2=3
```

---

## 2 Initialize a context

Initialize and use Duktape somewhere in your program:

```c
/* test.c */
#include <stdio.h>
#include "duktape.h"

int main(int argc, char *argv[]) {
  duk_context *ctx = duk_create_heap_default();
  duk_eval_string(ctx, "1+2");
  printf("1+2=%d\n", (int) duk_get_int(ctx, -1));
  duk_destroy_heap(ctx);
  return 0;
}
```

---

## 3 Add C function bindings

To call a C function from ECMAScript code, first declare your C functions:

```c
/* Being an embeddable engine, Duktape doesn't provide I/O
 * bindings by default.  Here's a simple one argument print()
 * function.
 */
static duk_ret_t native_print(duk_context *ctx) {
  printf("%s\n", duk_to_string(ctx, 0));
  return 0;  /* no return value (= undefined) */
}

/* Adder: add argument values. */
static duk_ret_t native_adder(duk_context *ctx) {
  int i;
  int n = duk_get_top(ctx);  /* #args */
  double res = 0.0;

  for (i = 0; i < n; i++) {
    res += duk_to_number(ctx, i);
  }

  duk_push_number(ctx, res);
  return 1;  /* one return value */
}
```

Register your functions e.g. into the global object:

```c
duk_push_c_function(ctx, native_print, 1 /*nargs*/);
duk_put_global_string(ctx, "print");
duk_push_c_function(ctx, native_adder, DUK_VARARGS);
duk_put_global_string(ctx, "adder");
```

You can then call your function from ECMAScript code:

```c
duk_eval_string_noresult(ctx, "print('2+3=' + adder(2, 3));");
```

---

Duktape is (C) by its [authors](https://github.com/svaarala/duktape/blob/master/AUTHORS.rst) and licensed under the [MIT license](https://github.com/svaarala/duktape/blob/master/LICENSE.txt).

