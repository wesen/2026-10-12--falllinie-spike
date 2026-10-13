// Host-only QuickJS C API research. No SDL/GL/audio callback, no libc modules.
#include "quickjs.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
using Clock=std::chrono::steady_clock;
static Clock::time_point deadline;
static int interrupts=0,released=0;
static int interrupt(JSRuntime*,void*){++interrupts;return Clock::now()>deadline;}
static std::string load(const char* path){std::ifstream f(path);if(!f)throw std::runtime_error(path);std::ostringstream s;s<<f.rdbuf();return s.str();}
static std::string text(JSContext* c,JSValueConst v){const char* s=JS_ToCString(c,v);if(!s)throw std::runtime_error("ToCString failed");std::string out(s);JS_FreeCString(c,s);return out;}
static std::string exception(JSContext* c){JSValue e=JS_GetException(c);auto s=text(c,e);JS_FreeValue(c,e);return s;}
static JSValue eval(JSContext* c,const std::string& src,int flags=JS_EVAL_TYPE_GLOBAL){return JS_Eval(c,src.c_str(),src.size(),"research.js",flags);}
static void must(JSContext* c,const std::string& src){JSValue v=eval(c,src);if(JS_IsException(v))throw std::runtime_error(exception(c));JS_FreeValue(c,v);}
static void failure(JSContext* c,const std::string& src,const char* label,int flags=JS_EVAL_TYPE_GLOBAL){auto v=eval(c,src,flags);if(!JS_IsException(v)){JS_FreeValue(c,v);throw std::runtime_error(std::string("expected failure: ")+label);}std::cout<<label<<": "<<exception(c)<<'\n';}
static JSValue consume(JSContext* c,JSValueConst,int argc,JSValueConst* argv){
 if(argc!=1)return JS_ThrowTypeError(c,"one Float64 numeric batch required");
 size_t offset=0,bytes=0,bpe=0,len=0;
 JSValue owner=JS_GetTypedArrayBuffer(c,argv[0],&offset,&bytes,&bpe);
 if(JS_IsException(owner))return owner;
 uint8_t* p=JS_GetArrayBuffer(c,&len,owner);
 // Narrow internal protocol: 8-byte elements; not an untrusted type-safe API.
 if(!p||bpe!=8||bytes>749*8||offset>len||bytes>len-offset){JS_FreeValue(c,owner);return JS_ThrowRangeError(c,"invalid numeric batch");}
 std::vector<double> copy(bytes/8);std::memcpy(copy.data(),p+offset,bytes);
 JS_FreeValue(c,owner); // no borrowed pointer retained; no JS calls while borrowing
 double sum=0;for(double n:copy){if(!std::isfinite(n))return JS_ThrowRangeError(c,"nonfinite numeric batch");sum+=n;}
 return JS_NewFloat64(c,sum);
}
static JSValue scalar(JSContext* c,JSValueConst,int argc,JSValueConst* argv){double n=0;if(argc!=1||JS_ToFloat64(c,&n,argv[0]))return JS_EXCEPTION;return JS_NewFloat64(c,n);}
#ifdef QUICKJS_NG
// ng v0.17 API explicitly differs: refuse resize/transfer; free only size==0.
static void* manage_lease(JSRuntime*,void*,void* ptr,size_t size){if(size)return nullptr;delete[] static_cast<uint8_t*>(ptr);++released;return nullptr;}
#else
static void free_lease(JSRuntime*,void*,void* ptr){delete[] static_cast<uint8_t*>(ptr);++released;}
#endif
static void bench(JSContext* c,const char* name){
 auto f=eval(c,name);if(JS_IsException(f))throw std::runtime_error(exception(c));
 std::vector<double> us;for(int i=0;i<330;i++){auto a=Clock::now();auto v=JS_Call(c,f,JS_UNDEFINED,0,nullptr);auto b=Clock::now();if(JS_IsException(v))throw std::runtime_error(exception(c));JS_FreeValue(c,v);if(i>=30)us.push_back(std::chrono::duration<double,std::micro>(b-a).count());}
 JS_FreeValue(c,f);std::sort(us.begin(),us.end());std::cout<<"bench "<<name<<" n=300 warmup=30 p50_us="<<us[149]<<" p95_us="<<us[284]<<" p99_us="<<us[296]<<" max_us="<<us.back()<<'\n';
}
int main(int argc,char** argv){try{
 if(argc!=4)throw std::runtime_error("host pure.js probe.js inline.js");
 JSRuntime* rt=JS_NewRuntime();if(!rt)throw std::runtime_error("NewRuntime");JS_SetMemoryLimit(rt,32*1024*1024);JS_SetMaxStackSize(rt,512*1024);
 JSContext* c=JS_NewContext(rt);if(!c)throw std::runtime_error("NewContext");
 auto global=JS_GetGlobalObject(c);JS_SetPropertyStr(c,global,"consume",JS_NewCFunction(c,consume,"consume",1));JS_SetPropertyStr(c,global,"scalar",JS_NewCFunction(c,scalar,"scalar",1));JS_FreeValue(c,global);
 must(c,load(argv[1]));must(c,load(argv[2]));
 auto v=eval(c,"JSON.stringify(oracle())");if(JS_IsException(v))throw std::runtime_error(exception(c));std::cout<<"oracle "<<text(c,v)<<'\n';JS_FreeValue(c,v);
 auto syntax=eval(c,load(argv[3]),JS_EVAL_TYPE_GLOBAL|JS_EVAL_FLAG_COMPILE_ONLY);if(JS_IsException(syntax))throw std::runtime_error(exception(c));JS_FreeValue(c,syntax);std::cout<<"full inline syntax PASS (not browser execution)\n";
 failure(c,"<meta charset=\"utf-8\">","HTML eval");
 auto browserless=JS_NewContext(rt);if(!browserless)throw std::runtime_error("browserless context");failure(browserless,load(argv[3]),"inline without browser");JS_FreeContext(browserless);
 failure(c,"throw new Error('intentional bridge test')","exception");
 must(c,"if(consume(new Float64Array([99,1,2,3]).subarray(1))!==6)throw Error('offset');");
 failure(c,"consume(new Float32Array(1))","wrong width");failure(c,"consume(new Float64Array([NaN]))","NaN");failure(c,"consume(new Float64Array(750))","oversize");
 must(c,"const cp=new Float64Array([1,2,3]);if(consume(cp)!==6)throw Error('copy');cp[0]=500;");
 // An explicitly owned external lease survives GC while rooted, then is released once.
 auto bytes=new uint8_t[24];double nums[3]={1,2,3};std::memcpy(bytes,nums,24);
#ifdef QUICKJS_NG
 auto buffer=JS_NewArrayBuffer(c,bytes,24,0,manage_lease,nullptr,false);
#else
 auto buffer=JS_NewArrayBuffer(c,bytes,24,free_lease,nullptr,false);
#endif
 if(JS_IsException(buffer))throw std::runtime_error(exception(c));
 global=JS_GetGlobalObject(c);JS_SetPropertyStr(c,global,"lease",buffer);JS_FreeValue(c,global);
 must(c,"globalThis.view=new Float64Array(lease);view[0]=4;if(consume(view)!==9)throw Error('lease');");
 JS_RunGC(rt);if(released)throw std::runtime_error("premature lease release");must(c,"view=null;lease=null");JS_RunGC(rt);if(released!=1)throw std::runtime_error("lease release count");std::cout<<"copy/offset/lease PASS release_count="<<released<<'\n';
 for(const char* name:{"noop","batch","scalarBridge","batchedBridge","churn"})bench(c,name);
 JSMemoryUsage before{},after{};JS_ComputeMemoryUsage(rt,&before);auto a=Clock::now();JS_RunGC(rt);auto b=Clock::now();JS_ComputeMemoryUsage(rt,&after);
 std::cout<<"gc explicit_us="<<std::chrono::duration<double,std::micro>(b-a).count()<<" used_before="<<before.memory_used_size<<" used_after="<<after.memory_used_size<<" objects_before="<<before.obj_count<<" objects_after="<<after.obj_count<<'\n';
 deadline=Clock::now()+std::chrono::milliseconds(20);JS_SetInterruptHandler(rt,interrupt,nullptr);a=Clock::now();failure(c,"while(true){}","interrupt");b=Clock::now();JS_SetInterruptHandler(rt,nullptr,nullptr);std::cout<<"interrupt wall_ms="<<std::chrono::duration<double,std::milli>(b-a).count()<<" polls="<<interrupts<<'\n';must(c,"1+1");
 JS_FreeContext(c);JS_FreeRuntime(rt);
 rt=JS_NewRuntime();JS_SetMemoryLimit(rt,1024*1024);c=JS_NewContext(rt);if(!c)throw std::runtime_error("limited context setup");failure(c,"new ArrayBuffer(16*1024*1024)","memory limit");must(c,"1+1");JS_FreeContext(c);JS_FreeRuntime(rt);
 std::cout<<"HOST PROBE PASS\n";return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
