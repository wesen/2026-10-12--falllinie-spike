---
Title: "Upstream capture 08-duktape-guide"
Ticket: FALLLINIE-JS-001
Status: active
Topics: [research]
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: ["https://duktape.org/guide.html"]
Summary: "Fetched upstream evidence; see capture timestamp and hash."
LastUpdated: 2026-10-13T14:23:55.356608+00:00
WhatFor: "Runtime comparison evidence."
WhenToUse: "Audit claims against fetched upstream material."
---

Fetched 2026-10-13T14:23:55.356608+00:00; URL https://duktape.org/guide.html; final URL https://duktape.org/guide.html; raw SHA256 `8708c7cbbcc3f75f91f2e3e6d17a1a695f7e22d16f45e45e497d4cf6b7e5f962`.
Mutable main/release pages are captures, not immutable release pins. Defuddle Markdown may omit page navigation; raw body can be recaptured with this script.

**Extraction warning:** Defuddle stdout is abbreviated for this long page; this is not a complete manual capture. Raw hash identifies the full fetched body. Use smaller topical captures for detailed API claims.

[![Fork me on GitHub](https://s3.amazonaws.com/github/ribbons/forkme_right_gray_6d6d6d.png)](https://github.com/svaarala/duktape)

**Duktape Programmer's Guide**

---

## Introduction

Version: 2.6.0 (2020-10-13)

## Document scope

This guide provides an introduction to using Duktape in your programs. Once you're familiar with the basics, there is a concise [API reference](/api) for looking up API details. The [Duktape Wiki](http://wiki.duktape.org) provides more detailed examples and best practices.

This document doesn't cover Duktape internals, see the [Duktape repo](https://github.com/svaarala/duktape/tree/master/doc) if you wish to tinker with them.

## What is Duktape?

Duktape is an embeddable ECMAScript® engine with a focus on portability and compact footprint. By integrating Duktape into your C/C++ program you can easily extend its functionality through scripting. You can also build the main control flow of your program in ECMAScript and use fast C code functions to do heavy lifting.

Embeddability means that Duktape makes minimal assumptions about the underlying platform and its capabilities; instead, the application which embeds Duktape is in control of which native bindings are provided and in what manner. For example, there are no default bindings for printing text to the console or to interact with the file system. Duktape distributable package includes example providers which you can easily integrate if they suit your needs.

The terms ECMAScript and Javascript are often considered more or less equivalent, although Javascript and its variants are technically just one environment where the ECMAScript language is used. The line between the two is not very clear in practice: even non-browser ECMAScript environments often provide some browser-specific built-ins. Even so, we use the term ECMAScript throughout to refer to the language implemented by Duktape.

## Conformance

Duktape conforms to ES5.0/ES5.1 with semantics updated from ES2015 or later where appropriate:

- [ECMAScript Language Specification 5th Edition](http://www.ecma-international.org/publications/files/ECMA-ST-ARCH/ECMA-262%205th%20edition%20December%202009.pdf)
- [ECMAScript® Language Specification 5.1 Edition](http://www.ecma-international.org/ecma-262/5.1/)

Duktape tracks the latest ECMAScript specification for semantics and built-ins (however support for ES2015 and later is still incomplete), see:

- [ECMAScript® 2015 Language Specification](http://www.ecma-international.org/ecma-262/6.0/)
- [ECMAScript® 2016 Language Specification](http://www.ecma-international.org/ecma-262/7.0/)

In some specific cases Duktape may follow a specification draft, see work in progress in [TC39/ecma262](https://github.com/tc39/ecma262). This is done when a feature in the latest specifications conflicts with real world code (see e.g. [RegExp.prototype issues](https://github.com/tc39/ecma262/pull/263)).

TypedArray support is based on ES2016 TypedArray; initial implementation was based on Khronos TypedArray specification:

- [Typed Array Specification (Editor's Draft 18 July 2013)](https://www.khronos.org/registry/typedarray/specs/latest/) (Note: link is to latest)
- `svn co -r 30720 https://cvs.khronos.org/svn/repos/registry/trunk/public/typedarray`

Node.js Buffer support is based on:

- [Buffer Node.js v6.9.1](https://nodejs.org/docs/v6.9.1/api/buffer.html)

TextEncoder() and TextDecoder() bindings are based on:

- [Encoding API](https://encoding.spec.whatwg.org/#api), WHATWG Encoding Living Standard

Performance.now() binding is based on:

- [High Resolution Time Level 2](https://www.w3.org/TR/hr-time/#dom-performance-now)

## Features

Besides standard ECMAScript features, Duktape has the following additional features (some are visible to applications, while others are internal):

- ES2015 [TypedArray](https://www.khronos.org/registry/typedarray/specs/latest/) and [Node.js Buffer](https://nodejs.org/docs/v6.9.1/api/buffer.html) bindings, plain buffer type (lightweight Uint8Array)
- From ECMAScript 2015 (ES6): `setPrototypeOf` / `__proto__`, a subset of `Proxy` objects, `Reflect`, computed property names, and minimal `const` support
- From ECMAScript 2016 (ES7): exponentiation operator (`**`, `**=`)
- [Encoding API](https://encoding.spec.whatwg.org/#api) bindings based on the WHATWG Encoding Living Standard
- Duktape specific built-ins: provided by the `Duktape` global object
- Extended types: custom "buffer" and "pointer" types, extended string type which supports arbitary binary strings and non- [BMP](http://en.wikipedia.org/wiki/Plane_\(Unicode\)#Basic_Multilingual_Plane) strings (standard ECMAScript only supports 16-bit codepoints)
- Combined reference counting and mark-and-sweep garbage collection, with finalizers and emergency garbage collection (you can also build with just mark-and-sweep)
- Coroutine support
- Tail call support
- Built-in debugger framework based on a debugger protocol
- Built-in regular expression engine with no platform dependencies
- Built-in Unicode support with no platform dependencies
- Built-in number parsing and formatting with no platform dependencies
- Built-in CBOR binding for binary serialization
- Additional custom JSON formats (JX and JC)
- Bytecode dump/load mechanism for caching compiled functions

## Goals

**Compliance**. ECMAScript E5/E5.1 and real world compliance. ECMAScript compliance requires regular expression and Unicode support. When possible, implement features from latest or draft ECMAScript specifications to minimize Duktape custom features.

**Portability**. Minimal system dependencies are nice when porting, so Duktape depends on very few system libraries. For example, number formatting and parsing, regular expressions, and Unicode are all implemented internally by Duktape. One of the few dependencies that cannot be fully eliminated is system date/time integration in the `Date` built-in. Duktape supports major platforms directly but you can also use an external Date provider on exotic platforms.

**Easy C interface**. The interface between Duktape and C programs should be natural and error-tolerant. As a particular issue, string representation should be UTF-8 with automatic NUL terminators to match common C use.

**Small footprint**. Code and data footprint should be as small as possible, even for small programs. Duktape is portable even to "bare metal" targets with no standard libraries. This is more important than performance, as there are already several very fast engines but fewer very compact, portable engines.

**Reasonable performance**. Small footprint (and portability, to some extent) probably eliminates the possibility of a competitive JIT-based engine, so there is no practical way of competing with very advanced JIT-based engines like SpiderMonkey (and its optimized variants) or Google V8. Performance should still be reasonable for typical embedded programs.[Lua](http://www.lua.org/) is a good benchmark in this respect. (Adding optional, modular support for JITing or perhaps off-line compilation would be nice.)

**ASCII string performance**. It's important that operations dealing with plain ASCII strings be very fast: ASCII dominates most embedded use. Operations dealing with non-ASCII strings need to perform reasonably but are not critical. This is a necessary trade-off: using C-compatible strings means essentially using UTF-8 string representation which makes string indexing and many other operations slower than with fixed size character representations. It's still important to support common idioms like iterating strings sequentially (in either direction) efficiently.

## Document organization

[Getting started](#gettingstarted) guides you through downloading, compiling, and integrating Duktape into your program. It also provides concrete examples of how you can integrate scripting capabilities into your program.

[Programming model](#programming), [Stack types](#stacktypes), and [C types](#ctypes) discuss core Duktape concepts such as *heap*, *context*, *value stacks*,*Duktape API*, and *Duktape/C functions*. Duktape stack types and C type wrappers are discussed in detail.

Duktape specific ECMAScript features are discussed in multiple sections:[Type algorithms](#typealgorithms) (for custom types),[Duktape built-ins](#duktapebuiltins) (additional built-ins), [Post-ES5 features](#postes5features) (features implemented from ES2016 and beyond),[Custom behavior](#custombehavior) (behavior differing from standard),[Custom JSON formats](#customjson),[Custom directives](#customdirectives),[Buffer objects](#bufferobjects),[Error objects](#errorobjects) (properties and traceback support),[Function objects](#functionobjects) (properties),[Date and time](#datetime),[Random numbers](#random),[Debugger](#debugger),[Modules](#modules),[Finalization](#finalization),[Coroutines](#coroutines),[Virtual properties](#virtualproperties),[Symbols](#symbols),[Bytecode dump/load](#bytecodedumpload),[Threading](#threading),[Sandboxing](#sandboxing).

[Performance](#performance) provides a few Duktape-specific tips for improving performance and avoiding performance pitfalls.[Memory usage](#memoryusage) summarizes Duktape memory usage and gives pointers for minimizing it.[Compiling](#compiling) describes how to configure and compile Duktape as part of your application.[Portability](#portability) covers platform and compiler specific issues and other portability issues.[Compatibility](#compatibility) discusses Duktape's compatibility with ECMAScript dialects, extensions, and frameworks.[Versioning](#versioning) describes Duktape versioning and what version compatibility to expect.[Limitations](#limitations) summarizes currently known limitations and provides possible workarounds.

[Comparison to Lua](#comparisontolua) discusses some differences between Lua and Duktape; it may be useful reading if you're already familiar with Lua.

---

## Getting started

## Downloading

Download the source distributable from the [Download](/download) page.

## Command line tool for testing

Unpack the distributable:

```
$ cd /tmp
$ tar xvfJ duktape-<version>.tar.xz
```

Compile the command line tool using the provided Makefile:

```
$ cd /tmp/duktape-<version>/
$ make -f Makefile.cmdline
```

The Makefile assumes you have `gcc` installed. If you don't, you can just edit the Makefile to match your compiler (the Makefile is quite simple).

Duktape doesn't provide built-in bindings for file or console I/O to avoid portability issues (for example, some platforms don't have I/O APIs at all). The command line utility provides `print()` and `alert()` bindings using [extras/print-alert](https://github.com/svaarala/duktape/blob/master/extras/print-alert) to make it easier to play with. There are useful "extras" in the distributable providing useful (optional) bindings such as:
- [print() and alert()](https://github.com/svaarala/duktape/blob/master/extras/print-alert)
- [console object, e.g. console.log()](https://github.com/svaarala/duktape/blob/master/extras/console)
**Throughout the guide examples will assume a `print()` binding for illustration.**

You can now run ECMAScript code interactively:

```
$ ./duk
((o) Duktape 2.6.0 (v2.6.0)
duk> print('Hello world!')
Hello world!
= undefined
```

You can also run ECMAScript code from a file which is useful for playing with features and algorithms. As an example, create `fib.js`:

```
// fib.js
function fib(n) {
    if (n == 0) { return 0; }
    if (n == 1) { return 1; }
    return fib(n-1) + fib(n-2);
}

function test() {
    var res = [];
    for (i = 0; i < 20; i++) {
        res.push(fib(i));
    }
    print(res.join(' '));
}

test();
```

Test the script from the command line:

```
$ ./duk fib.js
0 1 1 2 3 5 8 13 21 34 55 89 144 233 377 610 987 1597 2584 4181
```

## Integrating Duktape into your program

The command line tool is simply an example of a program which embeds Duktape. Embedding Duktape into your program is very simple:

- Run `duktape-N.N.N/tools/configure.py` to configure Duktape for build. The result is a directory containing `duktape.c`, `duktape.h`, and `duk_config.h`.
- Add `duktape.c`, `duktape.h`, and `duk_config.h` to your build, and call the Duktape API from elsewhere in your program.

The Duktape distributable (duktape-N.N.N.tar.xz) `src/` directory contains preconfigured header and source files for the Duktape default configuration which can usually be used as is. If needed, the configuration tool allows you to customize Duktape options, such as optimizing Duktape for low memory targets and enable/disable features. See [Compiling](#compiling) and [Configuring Duktape for build](http://wiki.duktape.org/Configuring.html) for more details and examples.

The distributable contains a very simple example program, `hello.c`, which illustrates this process. Compile the test program with the preconfigured Duktape header and source files e.g. as follows:

```
$ cd /tmp/duktape-<version>/
$ gcc -std=c99 -o hello -Isrc src/duktape.c examples/hello/hello.c -lm
```

To customize Duktape configuration use `configure.py`:

```
$ cd /tmp/duktape-<version>/
# Here we disable ECMAScript 6 Proxy object support
$ python2 tools/configure.py --output-directory duktape-src -UDUK_USE_ES6_PROXY
$ gcc -std=c99 -o hello -Iduktape-src duktape-src/duktape.c examples/hello/hello.c -lm
```

The test program creates a Duktape context and uses it to run some ECMAScript code:

```
$ ./hello
Hello world!
2+3=5
```

Because Duktape is an embeddable engine, you don't need to change the basic control flow of your program. The basic approach is:

- Create a Duktape context e.g. in program initialization (or even on-demand when scripting is needed). Usually you would also load your scripts during initialization, though that can also be done on-demand.
- Identify points in your code where you would like to use scripting and insert calls to script functions there.
- To make a script function call, first push call arguments to the Duktape context's *value stack* using the Duktape API. Then, use another Duktape API call to initiate the actual call.
- Once script execution is finished, control is returned to your program (the API call returns) and a return value is left on the Duktape context's value stack. C code can then access the return value using the Duktape API.

More broadly, there are several approaches to how you can use Duktape with native code; for example:

- Run the main application in C/C++ code but call into Duktape for extending base functionality (e.g. plugins or configuration).
- Run the main application in ECMAScript code, and call into simple C/C++ native bindings for I/O, performance intensive operations, etc. The native bindings are often kept stateless so that state logic remains in view of script code.
- Run the main application in ECMAScript code, but use more complex, stateful C/C++ native bindings for performance intensive operations. For example, a graphics engine can be implemented as a native object.

See the following Wiki articles for detailed examples:

- [Getting started: line processing](http://wiki.duktape.org/GettingStartedLineProcessing.html)
- [Getting started: primality testing](http://wiki.duktape.org/GettingStartedPrimalityTesting.html)

---

## Programming model

## Overview

Programming with Duktape is quite straightforward:

- Add Duktape source (`duktape.c`) and headers (`duktape.h` and `duk_config.h`) to your build. If the default configuration is not appropriate, use `python2 tools/configure.py` to prepare Duktape sources and headers for a custom configuration.
- Create a Duktape **heap** (a garbage collection region) and an initial **context** (essentially a thread handle) in your program.
- Load the necessary ECMAScript script files, and register your Duktape/C functions. Duktape/C functions are C functions you can call from ECMAScript code for better performance, bindings to native libraries, etc.
- Use the Duktape API to call ECMAScript functions whenever appropriate. Duktape API is used to pass values to and from functions. Values are converted between their C representation and the Duktape internal (ECMAScript compatible) representation.
- Duktape API is also used by Duktape/C functions (called from ECMAScript) to access call arguments and to provide return values.

Let's look at all the steps and their related concepts in more detail.

## Heap and context

A Duktape **heap** is a single region for garbage collection. A heap is used to allocate storage for strings, ECMAScript objects, and other variable size, garbage collected data. Objects in the heap have an internal heap header which provides the necessary information for reference counting, mark-and-sweep garbage collection, object finalization, etc. Heap objects can reference each other, creating a reachability graph from a garbage collection perspective. For instance, the properties of an ECMAScript object reference both the keys and values of the object's property set. You can have multiple heaps, but objects in different heaps cannot reference each other directly; you need to use serialization to pass values between heaps.

A Duktape **context** is an ECMAScript "thread of execution" which lives in a certain Duktape heap. A context is represented by a `duk_context *` in the Duktape API, and is associated with an internal Duktape coroutine (a form of a co-operative thread). Each context is also associated with an environment consisting of global objects; contexts may share the same global environment but can also have different environments. The context handle is given to almost every Duktape API call, and allows the caller to interact with the **value stack** of the Duktape coroutine: values can be inserted and queries, functions can be called, and so on.

Each coroutine has a **call stack** which controls execution, keeping track of function calls, native or ECMAScript, within the ECMAScript engine. Each coroutine also has a **value stack** which stores all the ECMAScript values of the coroutine's active call stack. The value stack always has an active **stack frame** for the most recent function call (when no function calls have been made, the active stack frame is the value stack as is). The Duktape API calls operate almost exclusively in the currently active stack frame. There's also internal book-keeping for error catching sites established using e.g. `try-catch-finally`.

Multiple contexts can share the same Duktape **heap**. In more concrete terms this means that multiple contexts can share the same garbage collection state, and can exchange object references safely. Contexts in different heaps cannot exchange direct object references; all values must be serialized in one way or another.

Almost every API call provided by the Duktape API takes a context pointer as its first argument: no global variables or states are used, and there are no restrictions on running multiple, independent Duktape heaps and contexts at the same time. There are multi-threading restrictions, however: only one native thread can execute any code within a single heap at any time, see [Threading](#threading).

To create a Duktape heap and an initial context inside the heap, you can simply use:

```c
duk_context *ctx = duk_create_heap_default();
if (!ctx) { exit(1); }
```

If you wish to provide your own memory allocation functions and a fatal error handler function (recommended), use:

```c
duk_context *ctx = duk_create_heap(my_alloc,
                                   my_realloc,
                                   my_free,
                                   my_udata,
                                   my_fatal);
if (!ctx) { exit(1); }
```

To create a new context inside the same heap, with the context sharing the same global objects:

```c
duk_context *new_ctx;

(void) duk_push_thread(ctx);
new_ctx = duk_get_context(ctx, -1 /*index*/);
```

To create a new context inside the same heap, but with a fresh set of global object:

```c
duk_context *new_ctx;

(void) duk_push_thread_new_globalenv(ctx);
new_ctx = duk_get_context(ctx, -1 /*index*/);
```

Contexts are automatically garbage collected when they become unreachable. This also means that if your C code holds a `duk_context *`, the corresponding Duktape coroutine MUST be reachable from a garbage collection point of view.

A heap must be destroyed explicitly when the caller is done with it:

```c
duk_destroy_heap(ctx);
```

This frees all heap objects allocated, and invalidates any pointers to such objects. In particular, if the calling program holds string pointers to values which resided on the value stack of a context associated with the heap, such pointers are invalidated and must never be dereferenced after the heap destruction call returns.

## Call stack (of a context)

The call stack of a context is not directly visible to the caller. It keeps track of the chain of function calls, either C or ECMAScript, currently executing in a context. The main purpose of this book-keeping is to facilitate the passing of arguments and results between function callers and callees, and to keep track of how the value stack is divided between function calls. The call stack also allows Duktape to construct a traceback for errors.

Because Duktape supports tail calls, the call stack does not always accurately represent the true call chain: tail calls will be "squashed" together in the call stack.

Don't confuse with the C stack.

## Value stack (of a context) and value stack index

The value stack of a context is an array of tagged type values related to the current execution state of a coroutine. The tagged types used are:`undefined`, `null`, boolean, number, string, object, buffer, pointer, and lightfunc. For a detailed discussion of the available tagged types, see [Types](#stacktypes).

The value stack is divided between the currently active function calls (activations) on the coroutine's call stack. At any time, there is an active stack frame which provides an origin for indexing elements on the stack. More concretely, at any time there is a **bottom** which is referred to with the index zero in the Duktape API. There is also a conceptual **top** which identifies the stack element right above the highest currently used element. The following diagram illustrates this:

```
Value stack
 of 15 entries
 (absolute indices)

.----.
| 15 |
| 14 |
| 13 |
| 12 |      Active stack frame (indices
| 11 |      relative to stack bottom)
| 10 |
|  9 |      .---.   Stack top is 6 (relative to stack bottom).
|  8 | <--- | 5 |   API index 5 is highest used (at value stack index 8).
|  7 |      | 4 |
|  6 |      | 3 |
|  5 |      | 2 |
|  4 |      | 1 |
|  3 | <--- | 0 |   API index 0 is bottom (at value stack index 3).
|  2 |      \`---'
|  1 |
|  0 |
\`----'
```

There is no direct way to refer to elements in the internal value stack: Duktape API always deals with the currently active stack frame. Stack frames are shown horizontally throughout the documentation for space reasons. For example, the active stack frame in the figure above would be shown as:

\[0 1 2 3 4 5\]

A **value stack index** is a signed integer index used in the Duktape API to refer to elements in currently active stack frame, relative to the current frame bottom.

Non-negative (>= 0) indices refer to stack entries in the current stack frame, relative to the frame bottom:

\[0 1 2 3 4 5\]

Negative (< 0) indices refer to stack entries relative to the top:

\[\-6 \-5 \-4 \-3 \-2 \-1\]

The special constant `DUK_INVALID_INDEX` is a negative integer which denotes an invalid stack index. It can be returned from API calls and can also be given to API calls to indicate a "no value".

The **value stack top** (or just "top") is the non-negative index of an imaginary element just above the highest used index. For instance, above the highest used index is 5, so the stack top is 6. The top indicates the current stack size, and is also the index of the next element pushed to the stack.

\[0 1 2 3 4 5 6\]

API stack operations are always confined to the current stack frame. There is no way to refer to stack entries below the current frame. This is intentional, as it protects functions in the call stack from affecting each other's values.

Don't confuse with the C stack.

## Growing the value stack

At any time, the value stack of a context is allocated for a certain maximum number of entries. An attempt to push values beyond the allocated size will cause an error to be thrown, it will **not** cause the value stack to be automatically extended. This simplifies the internal implementation and also improves performance by minimizing reallocations when you know, beforehand, that a certain number of entries will be needed during a function.

When a value stack is created or a Duktape/C function is entered, the value stack is always guaranteed to have space for the call arguments and `DUK_API_ENTRY_STACK` (currently 64) elements. In the typical case this is more than sufficient so that the majority of Duktape/C functions don't need to extend the value stack. Only functions that need more space or perhaps need an input-dependent amount of space need to grow the value stack.

You can extend the stack allocation explicitly with `[duk_check_stack()](/api#duk_check_stack)` or (usually more preferably) `[duk_require_stack()](/api#duk_require_stack)`. Once successfully extended, you are again guaranteed that the specified number of elements can be pushed to the stack. There is no way to shrink the allocation except by returning from a Duktape/C function.

Consider, for instance, the following function which will uppercase an input ASCII string by pushing uppercased characters one-by-one on the stack and then concatenating the result. This example illustrates how the number of value stack entries required may depend on the input (otherwise this is not a very good approach for uppercasing a string):

```c
/* uppercase.c */
#include <stdio.h>
#include <stdlib.h>
#include "duktape.h"

static int dummy_upper_case(duk_context *ctx) {
    size_t sz;
    const char *val = duk_require_lstring(ctx, 0, &sz);
    size_t i;

    /* We're going to need 'sz' additional entries on the stack. */
    duk_require_stack(ctx, sz);

    for (i = 0; i < sz; i++) {
        char ch = val[i];
        if (ch >= 'a' && ch <= 'z') {
            ch = ch - 'a' + 'A';
        }
        duk_push_lstring(ctx, (const char *) &ch, 1);
    }

    duk_concat(ctx, sz);
    return 1;
}

int main(int argc, char *argv[]) {
    duk_context *ctx;

    if (argc < 2) { exit(1); }

    ctx = duk_create_heap_default();
    if (!ctx) { exit(1); }

    duk_push_c_function(ctx, dummy_upper_case, 1);
    duk_push_string(ctx, argv[1]);
    duk_call(ctx, 1);
    printf("%s -> %s\n", argv[1], duk_to_string(ctx, -1));
    duk_pop(ctx);

    duk_destroy_heap(ctx);
    return 0;
}
```

In addition to user reserved elements, Duktape keeps an automatic internal value stack reserve to ensure all API calls have enough value stack space to work without further allocations. The value stack is also extended and shrunk in somewhat large steps to minimize memory reallocation activity. As a result the internal number of value stack elements available beyond the caller specified extra varies considerably. The caller does not need to take this into account and should never rely on any additional elements being available.

## ECMAScript array index

ECMAScript object and array keys can only be strings or Symbols. Array indices (e.g. 0, 1, 2) are represented as canonical string representations of the respective numbers (e.g. "0", "1", "2"). More technically, all canonical string representations of the integers in the range \[0, 2\*\*32-2\] are valid array indices.

To illustrate the ECMAScript array index handling, consider the following example:

```
var arr = [ 'foo', 'bar', 'quux' ];

print(arr[1]);     // refers to 'bar'
print(arr["1"]);   // refers to 'bar'

print(arr[1.0]);   // refers to 'bar', canonical encoding is "1"
print(arr["1.0"]); // undefined, not an array index
```

Some API calls operating on ECMAScript arrays accept numeric array index arguments. This is really just a short hand for denoting a string conversion of that number. For instance, if the API is given the integer 123, this really refers to the property name "123".

Internally, Duktape tries to avoid converting numeric indices to actual strings whenever possible, so it is preferable to use array index API calls when they are relevant. Similarly, when writing ECMAScript code it is preferable to use numeric rather than string indices, as the same fast path applies for ECMAScript code.

## Duktape API

Duktape API is the collection of user callable API calls defined in `duktape.h` and documented in the [API reference](/api).

The Duktape API calls are generally error tolerant and will check all arguments for errors (such as `NULL` pointers). However, to minimize footprint, the `ctx` argument is not checked, and the caller MUST NOT call any Duktape API calls with a `NULL` context.

All Duktape API calls are potentially macros. Calling code must not rely on any Duktape API call being available as a function pointer. The implementation of a certain API call may change between a macro and an actual function even between compatible releases. The Duktape API provides the following guarantees for macros:

- Arguments are never evaluated more than once (unless explicitly mentioned). However, an argument may not be evaluated at all if an argument is ignored in the current version.
- An API call with a return value can be used as an expression. If the API macro contains multiple statements, it is implemented as a comma expression (e.g. `(foo, bar, quux)`).
- An API call with a `void` return value may not necessarily work as part of an expression. The API macro may be implemented as a block statement or as a dummy `do {...} while (0)` loop.

## Duktape/C function

A C function with a Duktape/C API signature can be associated with an ECMAScript function object, and gets called when the ECMAScript function object is called. A Duktape/C API function looks as follows:

```c
duk_ret_t my_func(duk_context *ctx) {
    duk_push_int(ctx, 123);
    return 1;
}
```

The function gets ECMAScript call arguments in the value stack of `ctx`, with `[duk_get_top()](/api#duk_get_top)` indicating the number of arguments present on the value stack. The `this` binding is not automatically pushed to the value stack; use `[duk_push_this()](/api#duk_push_this)` to access it if necessary. When creating an ECMAScript function object associated with a Duktape/C function, one can select the desired number of arguments. Extra arguments are dropped and missing arguments are replaced with `undefined`. A function can also be registered as a vararg function (by giving `DUK_VARARGS` as the argument count) in which case call arguments are not modified prior to C function entry.

The function can return one of the following:

- Return value 1 indicates that the value on the stack top is to be interpreted as a return value.
- Return value 0 indicates that there is no explicit return value on the value stack; an `undefined` is returned to caller.
- A negative return value indicates that an error is to be automatically thrown. Error codes named `DUK_RET_xxx` map to specific kinds of errors (do not confuse these with `DUK_ERR_xxx` which are positive values). This is an optional shorthand which has a small footprint but also some drawbacks (e.g. no error message).
- A return value higher than 1 is currently undefined, as ECMAScript doesn't support multiple return values in Edition 5.1. (Values higher than 1 may be taken into to support multiple return values in ECMAScript Edition 6.)

When a Duktape/C function returns, the value stack is automatically unwound so there is no need to manually clean up the value stack when the function returns.

A negative error return value is intended to simplify common error handling, and is an alternative to constructing and throwing an error explicitly with Duktape API calls. No error message can be given; a message is automatically constructed by Duktape. For example:

```c
duk_ret_t my_func(duk_context *ctx) {
    if (duk_get_top(ctx) == 0) {
        /* throw TypeError if no arguments given */
        return DUK_RET_TYPE_ERROR;
    }
    /* ... */
}
```

All Duktape/C functions are considered **strict** in the [ECMAScript sense](http://www.ecma-international.org/ecma-262/5.1/#sec-4.2.2). Duktape API calls always obey ECMAScript strict mode semantics, even when the API calls are made outside of any Duktape/C function, i.e. with an empty call stack. For instance, attempt to delete a non-configurable property using `[duk_del_prop()](/api#duk_del_prop)` will cause an error to be thrown. This is the case with a strict ECMAScript function too:

```
function f() {
    'use strict';
    var arr = [1, 2, 3];
    return delete arr.length;  // array 'length' is non-configurable
}

print(f());  // this throws an error because f() is strict
```

Another consequence of Duktape/C function strictness is that the `this` binding given to Duktape/C functions is not [coerced](http://www.ecma-international.org/ecma-262/5.1/#sec-10.4.3). This is also the case for strict ECMAScript code:

```
function strictFunc() { 'use strict'; print(typeof this); }
function nonStrictFunc() { print(typeof this); }

strictFunc.call('foo');     // prints 'string' (uncoerced)
nonStrictFunc.call('foo');  // prints 'object' (coerced)
```

Duktape/C functions are currently always **constructable**, i.e. they can always be used in `new Foo()` expressions. You can check whether a function was called in constructor mode as follows:

```c
static duk_ret_t my_func(duk_context *ctx) {
    if (duk_is_constructor_call(ctx)) {
        printf("called as a constructor\n");
    } else {
        printf("called as a function\n");
    }
}
```

To save memory, Duktape/C functions don't have a `prototype` property by default, so the default object instance (given to the constructor as `this`) inherits from `Object.prototype`. To use a custom prototype you can define `prototype` for the Duktape/C function explicitly. Another approach is to ignore the default object instance and construct one manually within the Duktape/C call: as with ECMAScript functions, if a constructor returns an object value, that value replaces the default object instance and becomes the value of the `new` expression.

The `this` binding is not automatically pushed to the value stack; use `[duk_push_this()](/api#duk_push_this)` to access it.

## Storing state for a Duktape/C function

Sometimes it would be nice to provide parameters or additional state to a Duktape/C function out-of-band, i.e. outside explicit call arguments. There are several ways to achieve this.

### Properties of function

First, a Duktape/C function can use its Function object to store state or parameters. A certain Duktape/C function (the actual C function) is always represented by an ECMAScript Function object which is internally associated with the underlying C function. The Function object can be used to store properties related to that particular instance of the function. Note that a certain Duktape/C function can be associated with multiple independent Function objects and thus independent states.

Accessing the ECMAScript Function object related to a Duktape/C function is easy:

```c
duk_push_current_function(ctx);
duk_get_prop_string(ctx, -1, "my_state_variable");
```

### 'this' binding

Another alternative for storing state is to call the Duktape/C function as a method and then use the `this` binding for storing state. For instance, consider a Duktape/C function called as:

```
foo.my_c_func()
```

When called, the Duktape/C function gets `foo` as its `this` binding, and one could store state directly in `foo`. The difference to using the Function object approach is that the same object is shared by all methods, which has both advantages and disadvantages.

Accessing the `this` binding is easy:

```c
duk_push_this(ctx);
duk_get_prop_string(ctx, -1, "my_state_variable");
```

### Hidden Symbol properties

If data needs to be associated with an object, but hidden from ECMAScript code, hidden [Symbols](#symbols) can be used as a property key. The key is only accessible via the C API, unless passed to ECMAScript code from the C API. A common use case is to associated backing pointers/data to C memory. Symbols are created as a string, but are differentiated with macros that mark them as Symbols.

For example, setting and getting a hidden symbol on `this`:

```c
my_context_data_t *my_context_data = malloc(sizeof(my_context_data_t));
duk_push_this(ctx);
duk_push_pointer(ctx, my_context_data);
duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("my_context_data"));
/* ... */
duk_push_this(ctx);
duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("my_context_data"));
my_context_data_t *my_context_data = duk_get_pointer(ctx, -1);
```

### Magic value of function

Duktape/C function objects can store an internal 16-bit signed integer "magic" value (zero by default) with no extra memory cost. The magic value can be used to pass flags and/or small values to a Duktape/C function at minimal cost, so that a single native function can provide slightly varied behavior for multiple function objects:

```c
/* Magic value example: two lowest bits are used for a prefix index, bit 2 (0x04)
 * is used to select newline style for a log write helper.
 */
const char *prefix[4] = { "INFO", "WARN", "ERROR", "FATAL" };
duk_int_t magic = duk_get_current_magic(ctx);

printf("%s: %s", prefix[magic & 0x03], duk_safe_to_string(ctx, 0));
if (magic & 0x04) {
    printf("\r\n");
} else {
    printf("\n");
}
```

For an API usage example, see the test case [test-get-set-magic.c](https://github.com/svaarala/duktape/blob/master/tests/api/test-get-set-magic.c). Duktape uses magic values a lot internally to minimize size of compiled code, see e.g.[duk\_bi\_math.c](https://github.com/svaarala/duktape/blob/master/src-input/duk_bi_math.c).

The magic value mechanism is liable to change between major Duktape versions, as the number of available spare bits changes. Use magic values only when it really matters for footprint. Properties stored on the function object is a more stable alternative.

### Heap stash

The heap stash is an object visible only from C code. It is associated with the Duktape heap, and allows Duktape/C code to store "under the hood" state data which is not exposed to ECMAScript code. It is accessed with the `[duk_push_heap_stash()](/api#duk_push_heap_stash)` API call.

### Global stash

The global stash is like the heap stash, but is associated with a global object. It is accessed with the `[duk_push_global_stash()](/api#duk_push_global_stash)` API call. There can be several environments with different global objects within the same heap.

### Thread stash

The thread stash is like the heap stash, but is associated with a Duktape thread (i.e. a `ctx` pointer). It is accessible with the `[duk_push_thread_stash()](/api#duk_push_thread_stash)` API call.

## Duktape version specific code

The Duktape version is available through the `DUK_VERSION` define, with the numeric value `(major * 10000) + (minor * 100) + patch`. The same value is available to ECMAScript code through `Duktape.version`. Calling code can use this define for Duktape version specific code.

For C code:

```c
#if (DUK_VERSION >= 20403)
/* Duktape 2.4.3 or later */
#elif (DUK_VERSION >= 10500)
/* Duktape 1.5.0 or later */
#else
/* Duktape lower than 1.5.0 */
#endif
```

For ECMAScript code (also see [Duktape built-ins](#duktapebuiltins)):

```
if (typeof Duktape !== 'object') {
    print('not Duktape');
} else if (Duktape.version >= 20403) {
    print('Duktape 2.4.3 or higher');
} else if (Duktape.version >= 10500) {
    print('Duktape 1.5.0 or higher (but lower than 2.4.3)');
} else {
    print('Duktape lower than 1.5.0');
}
```

## Numeric error codes

When errors are created or thrown using the Duktape API, the caller must assign a numeric error code to the error. Error codes are positive integers, with a range restricted to 24 bits at the moment: the allowed error number range is thus \[1,16777215\]. Built-in error codes are defined in `duktape.h`, e.g. `DUK_ERR_TYPE_ERROR`.

The remaining high bits are used internally to carry e.g. additional flags. Negative error values are used in the Duktape/C API as a shorthand to automatically throw an error.

Error handling in the Duktape API is similar to how ECMAScript handles errors: errors are thrown either explicitly or implicitly, then caught and handled. Instead of a try-catch statement application C code uses `[protected](/api#taglist-protected)` Duktape API calls to establish points in C code where errors can be caught and handled. All Duktape API calls except protected calls may throw errors: most ECMAScript operations may cause error throws in some situations, and an out-of-memory error is possible in almost any situation. The long control transfer between the throw site and the catch site is based either on `setjmp()` / `longjmp()` (or their platform specific variants), or a C++ exception throw (when `DUK_USE_CPP_EXCEPTIONS` is enabled), see [Long control transfers](#long-control-transfers).

An uncaught error causes the fatal error handler to be called, which is considered an unrecoverable situation and should ordinarily be avoided, see [Normal and fatal errors](#normal-and-fatal-errors) and [How to handle fatal errors](http://wiki.duktape.org/HowtoFatalErrors.html). To avoid fatal errors, typical application code should establish an error catch site before making other Duktape API calls. This is done using protected Duktape API calls, for example:

- Use protected calls to evaluate code ([duk\_peval()](/api#duk_peval)), compile code ([duk\_pcompile()](/api#duk_pcompile)), and call ([duk\_pcall()](/api#duk_pcall)) functions.
- Use a single [duk\_safe\_call()](/api#duk_safe_call) to establish an error catcher so that you can use unsafe primitives freely inside the safe call.

An example of the first technique:

```c
/* Use duk_peval() variant to evaluate a file so that script errors are
 * handled safely.  Both syntax errors and runtime errors are caught.
 */

push_file_as_string(ctx, "myscript.js");
if (duk_peval(ctx) != 0) {
    /* Use duk_safe_to_string() to convert error into string.  This API
     * call is guaranteed not to throw an error during the coercion.
     */
    printf("Script error: %s\n", duk_safe_to_string(ctx, -1));
}
duk_pop(ctx);
```

An example of the second technique:

```c
/* Use duk_safe_call() to wrap all unsafe code into a separate C function.
 * This approach has the advantage of covering all API calls automatically
 * but is a bit more verbose.
 */

static duk_ret_t unsafe_code(duk_context *ctx, void *udata) {
    /* Here we can use unprotected calls freely. */

    (void) udata;  /* 'udata' may be used to pass e.g. a struct pointer */

    push_file_as_string(ctx, "myscript.js");
    duk_eval(ctx);

    /* ... */

    return 0;  /* success return, no return value */
}

/* elsewhere: */

if (duk_safe_call(ctx, unsafe_code, NULL /*udata*/, 0 /*nargs*/, 1 /*nrets */) != 0) {
    /* The 'nrets' argument should be at least 1 so that an error value
     * is left on the stack if an error occurs.  To avoid further errors,
     * use duk_safe_to_string() for safe error printing.
     */
    printf("Unexpected error: %s\n", duk_safe_to_string(ctx, -1));
}
duk_pop(ctx);
```

Even within protected calls there are some rare cases, such as internal errors, that will either cause a fatal error or propagate an error outwards from a protected API call. These should only happen in abnormal conditions and are not considered recoverable. To handle also these cases well, a production quality application should always have a fatal error handler with a reasonable strategy for dealing with fatal errors. Such a strategy is necessarily application dependent, but could be something like:

- On an embedded device the fatal error handler could write the fatal error information to a flash file and reboot the device. After the reboot, the fatal error could be reported to a diagnostics server so that it can be investigated.
- On a UNIX system the fatal error handler could simply exit the process (the default fatal handler uses `abort()`) and let a wrapper script restart the application.

Note that it may be fine for some applications to make API calls without an error catcher and risk throwing uncaught errors leading to a fatal error. It's not safe to continue execution after a fatal error, so such applications would typically simply exit when a fatal error occurs. Even without an actual recovery strategy, a fatal error handler should be used to e.g. write fatal error information to `stderr` before process exit.

An **ordinary error** is caused by a `throw` statement, a `[duk_throw()](/api#duk_throw)` API call (or similar), or by an internal, recoverable Duktape error. Ordinary errors can be caught with a `try-catch` in ECMAScript code or e.g. `[duk_pcall()](/api#duk_pcall)` (see API calls tagged `[protected](/api#taglist-protected)`) in C code.

A **fatal error** is caused by an uncaught error, an assertion failure (if enabled), an explicit call to `[duk_fatal()](/api#duk_fatal)`, or an unrecoverable error inside Duktape.

Each Duktape heap has a heap-wide fatal error handler registered in `[duk_create_heap()](/api#duk_create_heap)`. If no handler is given a built-in default handler is used:

- Default configuration: the built-in default fatal error handler writes a debug log message but **won't** write anything to `stdout` or `stderr`. Debug logging is disabled by default, so that the fatal error message is not displayed by default. The handler then calls `abort()`. If the abort() call exits for some reason, the handler then enters an infinite loop to ensure execution doesn't resume after a fatal error.
- `DUK_USE_CPP_EXCEPTIONS` enabled: the built-in default fatal error handler throws a `duk_fatal_exception`. This exception inherits from `std::runtime_error` so that it can be easily caught and provides a `::what()` method to read the fatal error message. It is unsafe to continue execution after catching the fatal error.
- If `DUK_USE_FATAL_HANDLER` is defined, it is always used as the built-in default fatal error handler, even if C++ exceptions are enabled.

There's no safe way to resume execution after a fatal error, so that a fatal error handler must not return or call into the Duktape API. Instead the handler should e.g. write out the message to the console or a log file and then exit the process (or restart an embedded device). This also applies when using C++ exceptions as the long control transfer mechanism. **If the application continues execution after a fatal error all bets are off: memory leaks are possible, and memory safety may be compromised.**

Fatal errors may also happen without any heap context, so that Duktape can't look up a possible heap-specific fatal error handler. Duktape will then always call the built-in default fatal error handler (with the handler userdata argument set to NULL). Fatal errors handled this way are currently limited to assertion failures, so that if you don't enable assertions, no such errors will currently occur and all fatal error handling goes through the heap-associated fatal error handler which is in direct application control.

See [How to handle fatal errors](http://wiki.duktape.org/HowtoFatalErrors.html) for more detail and examples.

## Long control transfers

The specific long control transfer mechanism Duktape uses internally for error throwing and catching is not directly visible to application code: application code uses a `[protected](/api#taglist-protected)` call to catch errors, while errors are thrown by Duktape or by the application using a variety of Duktape API calls, e.g. `duk_error()`.

### Default configuration, without DUK\_USE\_CPP\_EXCEPTIONS

In the default configuration a Duktape API protected call uses `setjmp()` to establish a catch site. The error throw site uses `longjmp()` to unwind the native C stack and return to the (nearest) catch site. On some platforms variants of the calls, such as `sigsetjmp()` and `siglongjmp` are used. There are minor differences between the call variants e.g. with regards to performance and signal handling. The variant is chosen by `duk_config.h`.

A `longjmp()` unwinds all the native C stack frames between the `longjmp()` and the `setjmp()`. However, this unwinding process won't invoke C++ automatic destructors which may be a significant limitation for some C++ applications.

For fatal errors, e.g. uncaught errors, the default fatal error handler uses `abort()`, see [Normal and fatal errors](#normal-and-fatal-errors).

### C++ mode, with DUK\_USE\_CPP\_EXCEPTIONS

When `DUK_USE_CPP_EXCEPTIONS` is enabled the long control transfer is based on a C++ exception throw. The protected call uses a C++ try-catch to establish a catch site; note that this happens inside Duktape, and is not visible to the application. The error throw site throws a `duk_internal_exception` which is caught by the (nearest) Duktape catch site. **Application code must not catch (or throw) the exception.**To minimize that risk, the exception doesn't inherit from any standard exception class so it won't be caught by a boilerplate `std::exception` catch site.

For fatal errors, e.g. uncaught errors, a `duk_fatal_exception` is thrown by the default fatal error handler. The exception inherits from `std::runtime_error` and is intended to be caught by user code. The exception provides a `::what()` method returning the fatal error message. Even though the fatal error is catchable, it is still unsafe to continue execution after catching the error. See [Normal and fatal errors](#normal-and-fatal-errors).

For both C++ exception types the C++ native stack unwind process supports automatic destructors for the stack frames between the throw site and the catch site.

---

## Stack types

## Overview

| [(none)](#type-none) | DUK\_TYPE\_NONE | DUK\_TYPE\_MASK\_NONE | no type (missing value, invalid index, etc) |
| --- | --- | --- | --- |
| [undefined](#type-undefined) | DUK\_TYPE\_UNDEFINED | DUK\_TYPE\_MASK\_UNDEFINED | `undefined` |
| [null](#type-null) | DUK\_TYPE\_NULL | DUK\_TYPE\_MASK\_NULL | `null` |
| [boolean](#type-boolean) | DUK\_TYPE\_BOOLEAN | DUK\_TYPE\_MASK\_BOOLEAN | `true` and `false` |
| [number](#type-number) | DUK\_TYPE\_NUMBER | DUK\_TYPE\_MASK\_NUMBER | IEEE double |
| [string](#type-string) | DUK\_TYPE\_STRING | DUK\_TYPE\_MASK\_STRING | immutable (plain) string or (plain) Symbol |
| [object](#type-object) | DUK\_TYPE\_OBJECT | DUK\_TYPE\_MASK\_OBJECT | object with properties |
| [buffer](#type-buffer) | DUK\_TYPE\_BUFFER | DUK\_TYPE\_MASK\_BUFFER | mutable (plain) byte buffer, fixed/dynamic/external; mimics an Uint8Array |
| [pointer](#type-pointer) | DUK\_TYPE\_POINTER | DUK\_TYPE\_MASK\_POINTER | opaque pointer (void \*) |
| [lightfunc](#type-lightfunc) | DUK\_TYPE\_LIGHTFUNC | DUK\_TYPE\_MASK\_LIGHTFUNC | plain Duktape/C function pointer (non-object); mimics a Function |

## Memory allocations

The following stack types involve additional heap allocations:

- String: a single allocation contains a combined heap and string header, followed by the immutable string data. When external strings are used (not enabled by default, typically in low memory environments) the string data may also be external.
- Object: one allocation is used for a combined heap and object header, and another allocation is used for object properties. The property allocation contains both array entries and normal properties, and if the object is large enough, a hash table to speed up lookups.
- Buffer: for fixed buffers a single allocation contains a combined heap and buffer header, followed by the mutable fixed-size buffer. For dynamic buffers the current buffer is allocated separately. For external buffers a single heap object is allocated and points to a user buffer.

While strings and buffers are considered a primitive (pass-by-value) types, they are a heap allocated type from a memory allocation viewpoint.

## Pointer stability

Heap objects allocated by Duktape have stable pointers: the objects are not relocated in memory while they are reachable from a garbage collection point of view. This is the case for the main heap object, but not necessarily for any additional allocations related to the object, such as dynamic property tables or dynamic buffer data area. A heap object is reachable e.g. when it resides on the value stack of a reachable thread or is reachable through the global object. Once a heap object becomes unreachable any pointers held by user C code referring to the object are unsafe and should no longer be dereferenced.

In practice the only heap allocated data directly referenced by user code are strings, fixed buffers, and dynamic buffers. The data area of strings and fixed buffers is stable; it is safe to keep a C pointer referring to the data even after a Duktape/C function returns as long the string or fixed buffer remains reachable from a garbage collection point of view at all times. Note that this is *not* the case for Duktape/C value stack arguments, for instance, unless specific arrangements are made.

The data area of a dynamic buffer does **not** have a stable pointer. The buffer itself has a heap header with a stable address but the current buffer is allocated separately and potentially relocated when the buffer is resized. It is thus unsafe to hold a pointer to a dynamic buffer's data area across a buffer resize, and it's probably best not to hold a pointer after a Duktape/C function returns (as there would be no easy way of being sure that the buffer hadn't been resized).

For external buffers the stability of the data pointer is up to the user code which sets and updates the pointer.

## Type masks

Type masks allows calling code to easily check whether a type belongs to a certain type set. For instance, to check that a certain stack value is a number, string, or an object:

```c
if (duk_get_type_mask(ctx, -3) & (DUK_TYPE_MASK_NUMBER |
                                  DUK_TYPE_MASK_STRING |
                                  DUK_TYPE_MASK_OBJECT)) {
    printf("type is number, string, or object\n");
}
```

There is a specific API call for matching a set of types even more conveniently:

```c
if (duk_check_type_mask(ctx, -3, DUK_TYPE_MASK_NUMBER |
                                 DUK_TYPE_MASK_STRING |
                                 DUK_TYPE_MASK_OBJECT)) {
    printf("type is number, string, or object\n");
}
```

These are faster and more compact than the alternatives:

```c
// alt 1
if (duk_is_number(ctx, -3) || duk_is_string(ctx, -3) || duk_is_object(ctx, -3)) {
    printf("type is number, string, or object\n");
}

// alt 2
int t = duk_get_type(ctx, -3);
if (t == DUK_TYPE_NUMBER || t == DUK_TYPE_STRING || t == DUK_TYPE_OBJECT) {
    printf("type is number, string, or object\n");
}
```

## None

The **none** type is not actually a type but is used in the API to indicate that a value does not exist, a stack index is invalid, etc.

## Undefined

The **undefined** type maps to ECMAScript `undefined`, which is distinguished from a `null`.

Values read from outside the active value stack range read back as **undefined**.

## Null

The **null** type maps to ECMAScript `null`.

## Boolean

The **boolean** type is represented in the C API as an integer: zero for false, and non-zero for true.

Whenever giving boolean values as arguments in API calls, any non-zero value is accepted as a "true" value. Whenever API calls return boolean values, the value `1` is always used for a "true" value. This allows certain C idioms to be used. For instance, a bitmask can be built directly based on API call return values, as follows:

```c
/* this works and generates nice code */
int bitmask = (duk_get_boolean(ctx, -3) << 2) |
              (duk_get_boolean(ctx, -2) << 1) |
              duk_get_boolean(ctx, -1);

/* more verbose variant not relying on "true" being represented by 1 */
int bitmask = ((duk_get_boolean(ctx, -3) ? 1 : 0) << 2) |
              ((duk_get_boolean(ctx, -2) ? 1 : 0) << 1) |
              (duk_get_boolean(ctx, -1) ? 1 : 0);

/* another verbose variant */
int bitmask = (duk_get_boolean(ctx, -3) ? (1 << 2) : 0) |
              (duk_get_boolean(ctx, -2) ? (1 << 1) : 0) |
              (duk_get_boolean(ctx, -1) ? 1 : 0);
```

## Number

The **number** type is an IEEE double, including +/- Infinity and NaN values. Zero sign is also preserved. An IEEE double represents all integers up to 53 bits accurately.

IEEE double allows NaN values to have additional signaling bits. Because these bits are used by Duktape internal tagged type representation (when using 8-byte packed values), NaN values in the Duktape API are normalized. Concretely, if you push a certain NaN value to the value stack, another (normalized) NaN value may come out. Don't rely on NaNs preserving their exact form.

## String

The **string** stack type is used to represent both plain strings and plain Symbols (introduced in ES2015). A string is an arbitrary byte sequence of a certain length which may contain internal NUL (0x00) values. Strings are always automatically NUL terminated for C coding convenience. The NUL terminator is not counted as part of the string length. For instance, the string `"foo"` has byte length 3 and is stored in memory as `{ 'f', 'o', 'o', '\0' }`. Because of the guaranteed NUL termination, strings can always be pointed to using a simple `const char *` as long as internal NULs are not an issue for the application; if they are, the explicit byte length of the string can be queried with the API. Calling code can refer directly to the string data held by Duktape. Such string data pointers are valid (and stable) for as long as a string is reachable in the Duktape heap.

Strings are [interned](http://en.wikipedia.org/wiki/String_interning) for efficiency: only a single copy of a certain string ever exists at a time. Strings are immutable and must NEVER be changed by calling C code. Doing so will lead to very mysterious issues which are hard to diagnose.

Calling code most often deals with ECMAScript strings, which may contain arbitrary 16-bit codepoints in the range U+0000 to U+FFFF but cannot represent non- [BMP](http://en.wikipedia.org/wiki/Basic_Multilingual_Plane#Basic_Multilingual_Plane) codepoints. This is how strings are defined in the ECMAScript standard. In Duktape, ECMAScript strings are encoded with [CESU-8](http://en.wikipedia.org/wiki/CESU-8) encoding. CESU-8 matches [UTF-8](http://en.wikipedia.org/wiki/UTF-8) except that it allows codepoints in the surrogate pair range (U+D800 to U+DFFF) to be encoded directly (prohibited in UTF-8). CESU-8, like UTF-8, encodes all 7-bit ASCII characters as-is which is convenient for C code. For example:

- U+0041 ("A") encodes to `41`.
- U+1234 (ETHIOPIC SYLLABLE SEE) encodes to `e1 88 b4`.
- U+D812 (high surrogate) encodes to `ed a0 92`. This would be [invalid UTF-8](http://en.wikipedia.org/wiki/UTF-8#Invalid_code_points).

Duktape also uses extended strings internally. Codepoints up to U+10FFFF can be represented with UTF-8, and codepoints above that up to full 32 bits can be represented with [extended UTF-8](http://en.wikipedia.org/wiki/UTF-8#Extending_from_31_bit_to_36_bit_range). The extended UTF-8 encoding used by Duktape is described in the table below. The leading byte is shown in binary (with "x" marking data bits) while continuation bytes are marked with "C" (indicating the bit sequence 10xxxxxx):

| U+0000 to U+007F | 7 | 0xxxxxxx |  |
| --- | --- | --- | --- |
| U+0080 to U+07FF | 11 | 110xxxxx C |  |
| U+0800 to U+FFFF | 16 | 1110xxxx C C | U+D800 to U+DFFF allowed (unlike UTF-8) |
| U+1 0000 to U+1F FFFF | 21 | 11110xxx C C C | Above U+10FFFF allowed (unlike UTF-8) |
| U+20 0000 to U+3FF FFFF | 26 | 111110xx C C C C |  |
| U+400 0000 to U+7FFF FFFF | 31 | 1111110x C C C C C |  |
| U+8000 0000 to U+F FFFF FFFF | 36 | 11111110 C C C C C C | Only 32 bits used in practice (up to U+FFFF FFFF) |

The downside of the encoding for codepoints above U+7FFFFFFF is that the leading byte will be `0xFE` which conflicts with Unicode byte order marker encoding. This is not a practical concern in Duktape's internal use.

Finally, invalid extended UTF-8 byte sequences are used for special purposes such as representing Symbol values. Invalid extended UTF-8/CESU-8 byte sequences never conflict with standard ECMAScript strings (which are CESU-8) and will remain cleanly separated within object property tables. For more information see [Symbols](#symbols) and [symbols.rst](https://github.com/svaarala/duktape/blob/master/doc/symbols.rst).

Strings with invalid extended UTF-8 sequences can be pushed on the value stack from C code and also passed to ECMAScript functions, with two caveats:

- If the invalid byte sequence matches the internal format used to represent Symbols, the value will appear as a Symbol rather than a string for ECMAScript code. For example, `typeof val` will be `symbol`.
- Behavior of string operations on invalid byte sequences if not well defined and results may vary, and change even in minor Duktape version updates.

## Object

The **object** type includes ECMAScript objects and arrays, functions, threads (coroutines), and buffer objects. In other words, anything with properties is an object. Properties are key-value pairs with a string key and an arbitrary value (including **undefined**).

Objects may participate in garbage collection finalization.

## Buffer

The plain **buffer** type is a raw buffer for user data. It's more memory efficient than standard buffer object types like Uint8Array or Node.js Buffer. There are three plain buffer sub-types:

| Buffer sub-type | Data pointer | Resizable | Memory managed by | Description |
| --- | --- | --- | --- | --- |
| Fixed | Stable, non-NULL | No | Duktape | Buffer size is fixed at creation, memory is managed automatically by Duktape. Fixed buffers have an unchanging (stable) non-NULL data pointer. |
| Dynamic | Unstable, may be NULL | Yes | Duktape | Buffer size can be changed after creation, memory is managed automatically by Duktape. Requires two memory allocations internally to allow resizing. Data pointer may change if buffer is resized. Zero-size buffer may have a NULL data pointer. |
| External | Unstable, may be NULL | Yes | Duktape and user code | Buffer data is externally allocated. Duktape allocates and manages a heap allocated buffer header structure, while the data area pointer and length are configured by user code explicitly. External buffers are useful to allow a buffer to point to data structures outside Duktape heap, e.g. a frame buffer allocated by a graphics library. Zero-size buffer may have a NULL data pointer. |

Unlike strings, buffer data areas are not automatically NUL-terminated and calling code must never access bytes beyond the currently allocated buffer size. The data pointer of a zero-size dynamic or external buffer may be NULL; fixed buffers always have a non-NULL data pointer.

Fixed and dynamic buffers are automatically garbage collected. This also means that C code must not hold onto a buffer data pointer unless the buffer is reachable to Duktape, e.g. resides in an active value stack. The data area of an external buffer is not automatically garbage collected so user code is responsible for managing its life cycle. User code must also make sure that no external buffer value is accessed when the external buffer is no longer (or not yet) valid.

Plain buffers mimic Uint8Array objects to a large extent, so they are a non-standard, memory efficient alternative to working with e.g. ArrayBuffer and typed arrays. The standard buffer objects are implemented on top of plain buffers, so that e.g. an ArrayBuffer will be backed by a plain buffer. See [Buffer objects](#bufferobjects) for more discussion.

A few notes:

- Because the value written to a buffer index is number coerced, assigning a one-character value does not work as often expected. For instance, `buf[123] = 'x'` causes zero to be written to the buffer, as ToNumber('x') = 0. For clarity, you should only assign number values, e.g. `buf[123] = 0x78`.
- There is a fast path for reading and writing numeric indices of plain buffer values, e.g. `x = buf[123]` or `buf[123] = x`. A similar fast path exists for the different buffer object values.
- Buffer virtual properties are not currently implemented in `defineProperty()`, so you can't write to buffer indices or buffer `length` with `defineProperty()` now (attempt to do so results in a `TypeError`).

## Pointer

The **pointer** type is a raw, uninterpreted C pointer, essentially a `void *`. Pointers can be used to point to native objects (memory allocations, handles, etc), but because Duktape doesn't know their use, they are not automatically garbage collected. You can, however, put one or more pointers inside an object and use the object finalizer to free the native resources related to the pointer(s).

## Lightfunc

The **lightfunc** type is a plain Duktape/C function pointer and a small set of control flags pac
