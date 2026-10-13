/* Stock Duktape 2.7.0 required-syntax probe, no production integration. */
#include "duktape.h"
#include <stdio.h>
int main(void){duk_context* c=duk_create_heap_default();if(!c)return 1;
 const char* cases[]={"const f = x => x + 1;", "let [a,b]=[1,2];", "class MB { constructor(){this.v=[];} }", "new Map([[1,2]])", "2 ** 3", "new Float64Array([1,2,3]).length"};
 for(int i=0;i<6;++i){int rc=duk_peval_string(c,cases[i]);printf("%s => %s: %s\n",cases[i],rc?"FAIL":"PASS",duk_safe_to_string(c,-1));duk_pop(c);if((i<4&&!rc)||(i>=4&&rc)){duk_destroy_heap(c);return 1;}}
 duk_destroy_heap(c);puts("EXPECTED COMPATIBILITY REJECTIONS PASS");return 0;}
