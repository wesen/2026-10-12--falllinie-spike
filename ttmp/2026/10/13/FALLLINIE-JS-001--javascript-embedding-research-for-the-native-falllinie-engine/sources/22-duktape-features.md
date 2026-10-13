---
Title: "Upstream capture 22-duktape-features"
Ticket: FALLLINIE-JS-001
Status: active
Topics: [research]
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: ["https://wiki.duktape.org/PostEs5Features.html"]
Summary: "Fetched upstream evidence; see capture timestamp and hash."
LastUpdated: 2026-10-13T14:24:17.271667+00:00
WhatFor: "Runtime comparison evidence."
WhenToUse: "Audit claims against fetched upstream material."
---

Fetched 2026-10-13T14:24:17.271667+00:00; URL https://wiki.duktape.org/PostEs5Features.html; final URL https://wiki.duktape.org/postes5features; raw SHA256 `3450d2cd936ca856422dee9e3d389f75e15a0878a7023d9144dc04b55690962f`.
Mutable main/release pages are captures, not immutable release pins. Defuddle Markdown may omit page navigation; raw body can be recaptured with this script.

## Post-ES5 features

This page summarizes features implemented from [ES2015](http://www.ecma-international.org/ecma-262/6.0/index.html) (ES6), [ES2016](http://www.ecma-international.org/ecma-262/7.0/index.html) (ES7), and later. Custom features like Node.js Buffer and WHATWG Encoding API are also listed.

Many of the features can be disabled through config options such as `DUK_USE_ES6_PROXY` and `DUK_USE_BUFFEROBJECT_SUPPORT`.

Duktape status is also updated for new releases in [kangax/compat-table](https://kangax.github.io/compat-table).

## Summary

| Feature | Specification | Status | Duktape   version | Notes |
| --- | --- | --- | --- | --- |
| [Duktape object](http://duktape.org/guide.html#builtin-duktape) | Duktape | n/a | 1.0.0 | Object providing Duktape specific operations like inspecting values, forcing a garbage collection run, etc. |
| [Proxy object](http://www.ecma-international.org/ecma-262/6.0/#sec-proxy-objects) | ES2015 | Partial | 1.0.0 | Partial support, see separate Proxy trap support table below. |
| [Object.setPrototypeOf()](http://www.ecma-international.org/ecma-262/6.0/#sec-object.setprototypeof) | ES2015 | Full | 1.0.0 | `Object.setPrototypeOf()` allows user to set the internal prototype of an object which is not supported in ECMAScript E5. |
| [Object.prototype.**proto**](http://www.ecma-international.org/ecma-262/6.0/#sec-object.prototype.__proto__) | ES2015 Annex B | Partial | 1.0.0 | `Object.prototype.**proto**` is a setter/getter which provides the same functionality as `Object.getPrototypeOf()` and `Object.setPrototypeOf()` but is compatible with existing code base which has relied on a non-standard `**proto**` property for a while. The property is not available for "bare objects". Duktape does not support [**proto** property name in an object initializer](http://www.ecma-international.org/ecma-262/6.0/index.html#sec-__proto__-property-names-in-object-initializers). |
| [Additional RegExp syntax](http://www.ecma-international.org/ecma-262/6.0/#sec-regular-expressions-patterns) | ES2015 Annex B | Partial | 1.0.0 | Support for non-ES5 RegExp forms (most of them described in ES2015 Annex B) has been added incrementally over releases. |
| [ArrayBuffer](http://www.ecma-international.org/ecma-262/6.0/#sec-arraybuffer-objects) | ES2015 | Partial | 1.3.0 | Original Duktape implementation was based on the Khronos specification. Detached ArrayBuffers are not yet supported. |
| [Typed arrays](http://www.ecma-international.org/ecma-262/6.0/#sec-typedarray-objects) | ES2015 | Partial | 1.3.0 | Original Duktape implementation was based on the Khronos specification. |
| [DataView](http://www.ecma-international.org/ecma-262/6.0/#sec-dataview-objects) | ES2015 | Partial | 1.3.0 | Original Duktape implementation was based on the Khronos specification. |
| [Node.js Buffer](https://nodejs.org/api/buffer.html) | Node.js v6.9.1 | Partial | 1.3.0 | Original implementation was based on Node.js Buffer v0.12.1, current goal is Node.js v6.9.1 but not all methods are yet implemented. Duktape tracks the latest Node.js Buffer API. |
| [const declaration](http://www.ecma-international.org/ecma-262/6.0/#sec-let-and-const-declarations) | ES2015 | Partial | 1.4.0 | Support is partial, `const` is mostly just an alias for `var` except that an initializer is required: const variables are writable and function scope rather than block scope. |
| [Computed property names](http://www.ecma-international.org/ecma-262/6.0/#sec-object-initializer-runtime-semantics-evaluation) | ES2015 | Partial | 2.0.0 | Computed method name in object literal not yet supported. Example: `{ [1+2]: 'three' }`. |
| [Octal number literal](http://www.ecma-international.org/ecma-262/6.0/#sec-literals-numeric-literals) | ES2015 | Full | 2.0.0 | Example: `0o755`. Legacy octal literals like `0755` are also supported. |
| [Binary number literal](http://www.ecma-international.org/ecma-262/6.0/#sec-literals-numeric-literals) | ES2015 | Full | 2.0.0 | Example: `0b10001010`. |
| [Unicode escape](http://www.ecma-international.org/ecma-262/6.0/#sec-literals-string-literals) | ES2015 | Partial | 2.0.0 | The escape syntax is allowed in both string literals and identifier names. Not yet supported in RegExps (requires /u flag support which is not yet implemented). Non-BMP escapes decode to surrogate pairs. Example: `"pile of "`. |
| [Reflect object](http://www.ecma-international.org/ecma-262/6.0/#sec-reflect-object) | ES2015 | Partial | 2.0.0 | Provides access to several fundamental ECMAScript primitives as function calls. For example, `Reflect.construct()` behaves like `new`. Currently has some limitations, e.g. explicit `newTarget` is not supported in `Reflect.construct()`. |
| [ES2015 enumeration order](http://www.ecma-international.org/ecma-262/6.0/#sec-ordinary-object-internal-methods-and-internal-slots-ownpropertykeys) | ES2015 | Full | 2.0.0 | `Object.getOwnPropertyNames()` follows the [ES2015 \[\[OwnPropertyKeys\]\]](http://www.ecma-international.org/ecma-262/6.0/#sec-ordinary-object-internal-methods-and-internal-slots-ownpropertykeys) enumeration order: (1) array indices in ascending order, (2) other properties in insertion order, (3) symbols in insertion order. While ES2015 or ES2016 don't require it, Duktape follows this same order also for `for-in`, `Object.keys()`, and `duk_enum()` in general. As in V8, the rule is applied for every "inheritance level" in turn, i.e. inherited non-duplicate properties always follow child properties. |
| [Exponentiation operator](http://www.ecma-international.org/ecma-262/7.0/#sec-exp-operator) | ES2016 | Full | 2.0.0 | Example: `x ** y`, `x **= y`. |
| [RegExp getter lenience](https://github.com/tc39/ecma262/pull/263) | ES2017 draft | n/a | 2.0.0 | RegExp.prototype.flags and other getters accept the RegExp.prototype object as a this binding without throwing a TypeError. See [RegExp.prototype issues](https://github.com/tc39/ecma262/pull/263). |
| [Symbol object](http://www.ecma-international.org/ecma-262/6.0/#sec-symbol-objects) | ES2015 | Full | 2.0.0 | Experimental, `Symbol` binding is disabled by default in Duktape 2.0.0, enable using `DUK_USE_SYMBOL_BUILTIN`. |
| [Encoding API](https://encoding.spec.whatwg.org/) | WHATWG | Full | 2.0.0 | TextEncoder() and TextDecoder(), supports UTF-8 encoding. |
| ["global" binding](https://github.com/tc39/proposal-global) | TC39 proposal | Full | 2.1.0 | Experimental, named binding "global" for the global object. Disabled by default, enable using `DUK_USE_GLOBAL_BINDING`. |
| [HTML comment syntax](http://www.ecma-international.org/ecma-262/6.0/#sec-html-like-comments) | ES2015 | Full | 2.1.0 | HTML comments syntax, recognizing `<!--` and `-->` as specified in ES2015 Annex B.1.3. |
| [new.target](http://www.ecma-international.org/ecma-262/6.0/#sec-meta-properties-runtime-semantics-evaluation) | ES2015 | Full | 2.2.0 | `new.target` syntax. |
| [**defineGetter**, etc](https://tc39.github.io/ecma262/#sec-additional-properties-of-the-object.prototype-object) | ES2017 | Full | 2.2.0 | Annex B `Object.prototype.{__defineGetter,**defineSetter**}` and `Object.prototype.{**lookupGetter**,**lookupSetter**}`, also used in a lot of legacy code. |
| [performance](https://www.w3.org/TR/hr-time/#dom-performance-now) | W3C | Partial | 2.2.0 | W3C High Resolution Time API `performance.now()` with sub-millisecond resolution (when platform provides it). |
| [ES2015 Number built-in](https://www.ecma-international.org/ecma-262/6.0/#sec-number-objects) | ES2015 | Full | 2.3.0 | New Number built-in properties from ES2015, e.g. `EPSILON`, `MAX_SAFE_INTEGER`, `parseInt()`, `parseFloat()`. |
| [ES2016 Number built-in](https://www.ecma-international.org/ecma-262/7.0/#sec-number-objects) | ES2016 | Full | 2.3.0 | New Number built-in properties from ES2016, only `MIN_SAFE_INTEGER`. |
| [@ @hasInstance](https://www.ecma-international.org/ecma-262/6.0/#sec-well-known-symbols) | ES2015 | Full | 2.3.0 | @ @hasInstance well-known symbol support in `instanceof`. |
| [@ @toStringTag](https://www.ecma-international.org/ecma-262/6.0/#sec-well-known-symbols) | ES2015 | Full | 2.3.0 | @ @toStringTag well-known symbol support in `Object.prototype.toString()`. |
| [@ @toPrimitive](https://www.ecma-international.org/ecma-262/6.0/#sec-well-known-symbols) | ES2015 | Full | 2.3.0 | @ @toPrimitive well-known symbol support in `ToPrimitive()`. |
| [@ @isConcatSpreadable](https://www.ecma-international.org/ecma-262/6.0/#sec-well-known-symbols) | ES2015 | Full | 2.3.0 | @ @isConcatSpreadable well-known symbol support in `Array.prototype.concat`. |

## Proxy handlers (traps)

The ECMAScript ES2015 `Proxy` object allows property virtualization and fine-grained access control for accessing an underlying plain object. Duktape implements a strict subset of the `Proxy` object from ES2015. Status of trap implementation:

| Trap | Implemented in version | Notes |
| --- | --- | --- |
| getPrototypeOf | no |  |
| setPrototypeOf | no |  |
| isExtensible | no |  |
| preventExtension | no |  |
| getOwnPropertyDescriptor | no |  |
| defineProperty | no |  |
| has | 1.0.0 | `Object.hasOwnProperty()` does not invoke the trap at the moment, `key in obj` does. |
| get | 1.0.0 |  |
| set | 1.0.0 |  |
| deleteProperty | 1.0.0 |  |
| enumerate | removed | The "enumerate" trap was removed in ES2016 and for-in uses "ownKeys" trap; Duktape 1.x supports "enumerate" trap in for-in. |
| ownKeys | 1.0.0 | Some trap result validation (non-configurable properties, non-extensible target) not yet implemented. |
| apply | 2.2.0 |  |
| construct | 2.2.0 | Some limitations for new.target and.prototype lookup. |

Duktape specific behavior:

- Proxy key argument is not string coerced at present, e.g. `proxy[123]` invokes the `.get` trap with a number (123) rather than a string ("123") key argument. Standard behavior is to string coerce the key which is much slower when virtualizing indexed objects. Future work is to change the behavior to conforming, but provide some way of configuring a Proxy to provide the sometimes preferred non-coercing behavior.

Limitations include:

- Only about half of the ES2015 traps have been implemented. This causes odd behavior if you e.g. call `Object.defineProperty()` on a proxy object.
- Proxy trap results are not always validated, e.g. `ownKeys` trap result validation steps described in [\[\[OwnPropertyKeys\]\] ()](http://www.ecma-international.org/ecma-262/6.0/#sec-proxy-object-internal-methods-and-internal-slots-ownpropertykeys) for non-configurable target properties and/or non-extensible target object are not yet implemented.
- Inheriting from Proxy objects does not always work correctly, e.g. `get` trap is not invoked when reading a property from an object inheriting from a Proxy.
- Proxy revocation feature of ES2015 is not supported.
- The target and handler objects given to `new Proxy()` cannot be Proxy objects themselves. ES2015 poses no such limitation, but Duktape enforces it for now to simplify the internal implementation.

