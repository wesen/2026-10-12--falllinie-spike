---
Title: "Upstream capture 14-quickjspp-readme"
Ticket: FALLLINIE-JS-001
Status: active
Topics: [research]
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: ["https://raw.githubusercontent.com/ftk/quickjspp/01cdd3047ced48265b127790848a0ca88204f2c7/readme.md"]
Summary: "Fetched upstream evidence; see capture timestamp and hash."
LastUpdated: 2026-10-13T14:14:26.221718+00:00
WhatFor: "Runtime comparison evidence."
WhenToUse: "Audit claims against fetched upstream material."
---

Fetched 2026-10-13T14:14:26.221718+00:00; URL https://raw.githubusercontent.com/ftk/quickjspp/01cdd3047ced48265b127790848a0ca88204f2c7/readme.md; final URL https://raw.githubusercontent.com/ftk/quickjspp/01cdd3047ced48265b127790848a0ca88204f2c7/readme.md; raw SHA256 `36b379ccdc04acfd9ad6311fec95eba75d227e18ab05eb7cdc8337323cf3889f`.
Mutable main/release pages are captures, not immutable release pins. Defuddle Markdown may omit page navigation; raw body can be recaptured with this script.

```text
QuickJSPP is QuickJS wrapper for C++. It allows you to easily embed Javascript engine into your program.

QuickJS is a small and embeddable Javascript engine. It supports the ES2020 specification including modules, asynchronous generators and proxies. More info: <https://bellard.org/quickjs/>

# Example
```cpp
#include "quickjspp.hpp"
#include <iostream>

class MyClass
{
public:
    MyClass() {}
    MyClass(std::vector<int>) {}

    double member_variable = 5.5;
    std::string member_function(const std::string& s) { return "Hello, " + s; }
};

void println(qjs::rest<std::string> args) {
    for (auto const & arg : args) std::cout << arg << " ";
    std::cout << "\n";
}

int main()
{
    qjs::Runtime runtime;
    qjs::Context context(runtime);
    try
    {
        // export classes as a module
        auto& module = context.addModule("MyModule");
        module.function<&println>("println");
        module.class_<MyClass>("MyClass")
                .constructor<>()
                .constructor<std::vector<int>>("MyClassA")
                .fun<&MyClass::member_variable>("member_variable")
                .fun<&MyClass::member_function>("member_function");
        // import module
        context.eval(R"xxx(
            import * as my from 'MyModule';
            globalThis.my = my;
        )xxx", "<import>", JS_EVAL_TYPE_MODULE);
        // evaluate js code
        context.eval(R"xxx(
            let v1 = new my.MyClass();
            v1.member_variable = 1;
            let v2 = new my.MyClassA([1,2,3]);
            function my_callback(str) {
              my.println("at callback:", v2.member_function(str));
            }
        )xxx");

        // callback
        auto cb = (std::function<void(const std::string&)>) context.eval("my_callback");
        cb("world");
    }
    catch(qjs::exception)
    {
        auto exc = context.getException();
        std::cerr << (std::string) exc << std::endl;
        if((bool) exc["stack"])
            std::cerr << (std::string) exc["stack"] << std::endl;
        return 1;
    }
}
```

# Installation
QuickJSPP is header-only - put quickjspp.hpp into your include search path.
Compiler that supports C++17 or later is required.
The program needs to be linked against QuickJS.
Sample CMake project files are provided.

# License
QuickJSPP is licensed under [CC0](https://creativecommons.org/publicdomain/zero/1.0/). QuickJS is licensed under [MIT](https://opensource.org/licenses/MIT).

```

