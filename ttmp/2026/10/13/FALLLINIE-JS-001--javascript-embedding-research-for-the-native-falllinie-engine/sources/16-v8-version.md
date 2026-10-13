---
Title: "Upstream capture 16-v8-version"
Ticket: FALLLINIE-JS-001
Status: active
Topics: [research]
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: ["https://raw.githubusercontent.com/v8/v8/main/include/v8-version.h"]
Summary: "Fetched upstream evidence; see capture timestamp and hash."
LastUpdated: 2026-10-13T14:14:27.625119+00:00
WhatFor: "Runtime comparison evidence."
WhenToUse: "Audit claims against fetched upstream material."
---

Fetched 2026-10-13T14:14:27.625119+00:00; URL https://raw.githubusercontent.com/v8/v8/main/include/v8-version.h; final URL https://raw.githubusercontent.com/v8/v8/main/include/v8-version.h; raw SHA256 `8578ab88b5b496de0bee2aabb2bc5c35a591f72e988e9d68b0cece04f825cb53`.
Mutable main/release pages are captures, not immutable release pins. Defuddle Markdown may omit page navigation; raw body can be recaptured with this script.

```text
// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_INCLUDE_VERSION_H_  // V8_VERSION_H_ conflicts with src/version.h
#define V8_INCLUDE_VERSION_H_

// These macros define the version number for the current version.
// NOTE these macros are used by some of the tool scripts and the build
// system so their names cannot be changed without changing the scripts.
#define V8_MAJOR_VERSION 15
#define V8_MINOR_VERSION 8
#define V8_BUILD_NUMBER 0
#define V8_PATCH_LEVEL 0

// Use 1 for candidates and 0 otherwise.
// (Boolean macro values are not supported by all preprocessors.)
#define V8_IS_CANDIDATE_VERSION 1

#endif  // V8_INCLUDE_VERSION_H_

```

