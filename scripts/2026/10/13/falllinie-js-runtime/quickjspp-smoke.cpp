// Header compatibility only; does not assert wrapper runtime or ownership correctness.
#include "quickjspp.hpp"
int main(){qjs::Runtime r;qjs::Context c(r);c.eval("1+1");}
