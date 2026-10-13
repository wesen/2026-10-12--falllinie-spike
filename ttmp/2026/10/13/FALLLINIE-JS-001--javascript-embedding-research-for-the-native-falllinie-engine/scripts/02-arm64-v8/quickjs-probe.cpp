// Pinned direct C API; no quickjs-libc, wrappers, SDL or audio services.
#include <cstring>
#include "quickjs.h"
#include "probe-common.hpp"
class QuickJSProbe {
 JSRuntime* rt=nullptr;JSContext* ctx=nullptr;JSValue compiled=JS_UNDEFINED,tagGetter=JS_UNDEFINED;
 std::map<std::string,JSValue> functions;CopyState copied;
 static QuickJSProbe* self(JSContext* c){return static_cast<QuickJSProbe*>(JS_GetContextOpaque(c));}
 std::string string(JSValueConst v){size_t n=0;const char* s=JS_ToCStringLen(ctx,&n,v);if(!s)throw std::runtime_error("string conversion failed");std::string out(s,n);JS_FreeCString(ctx,s);return out;}
 JSValue checked(JSValue v){if(!JS_IsException(v))return v;auto ex=JS_GetException(ctx);std::string msg;try{msg=string(ex);}catch(...){msg="exception formatting unavailable";}JS_FreeValue(ctx,ex);throw std::runtime_error(msg);}
 JSValue eval(const std::string& s){return checked(JS_Eval(ctx,s.data(),s.size(),"qualification-eval.js",JS_EVAL_TYPE_GLOBAL));}
 static JSValue consume(JSContext* c,JSValueConst,int argc,JSValueConst* args){
  auto* e=self(c);if(argc!=1)return JS_ThrowTypeError(c,"one Float64Array required");
#ifdef QUICKJS_NG
  if(JS_GetTypedArrayType(args[0])!=JS_TYPED_ARRAY_FLOAT64)return JS_ThrowTypeError(c,"Float64Array required");
#else
  // Captured intrinsic getter reads internal typed-array brand, unlike instanceof.
  auto tag=JS_Call(c,e->tagGetter,args[0],0,nullptr);if(JS_IsException(tag))return tag;
  const char* name=JS_ToCString(c,tag);bool exact=name&&std::strcmp(name,"Float64Array")==0;if(name)JS_FreeCString(c,name);JS_FreeValue(c,tag);
  if(!exact)return JS_ThrowTypeError(c,"Float64Array required");
#endif
  size_t offset=0,bytes=0,bpe=0,len=0;auto owner=JS_GetTypedArrayBuffer(c,args[0],&offset,&bytes,&bpe);if(JS_IsException(owner))return owner;
  auto* p=JS_GetArrayBuffer(c,&len,owner);
  bool valid=p&&bpe==8&&bytes<=749*8&&offset<=len&&bytes<=len-offset;
  if(valid)valid=e->copied.copy(p+offset,bytes); // no JS calls while bytes are borrowed
  JS_FreeValue(c,owner);
  if(!valid)return JS_ThrowRangeError(c,"invalid finite numeric batch");return JS_NewFloat64(c,e->copied.sum);
 }
 static JSValue detachForProbe(JSContext* c,JSValueConst,int argc,JSValueConst* args){
  if(argc!=1)return JS_ThrowTypeError(c,"one buffer required");JS_DetachArrayBuffer(c,args[0]);return JS_UNDEFINED;
 }
 static JSValue copiedSum(JSContext* c,JSValueConst,int,JSValueConst*){return JS_NewFloat64(c,self(c)->copied.sum);}
 static JSValue scalar(JSContext* c,JSValueConst,int argc,JSValueConst* args){double n=0;if(argc!=1||!JS_IsNumber(args[0]))return JS_ThrowTypeError(c,"numeric scalar required");if(JS_ToFloat64(c,&n,args[0]))return JS_EXCEPTION;if(!std::isfinite(n))return JS_ThrowRangeError(c,"finite scalar required");return JS_NewFloat64(c,n);}
 static JSValue acceptScore(JSContext* c,JSValueConst,int argc,JSValueConst* args){
  if(argc!=1||!JS_IsObject(args[0]))return JS_ThrowTypeError(c,"score object required");
  auto pts=JS_GetPropertyStr(c,args[0],"pts");if(JS_IsException(pts))return pts;
  auto name=JS_GetPropertyStr(c,args[0],"name");if(JS_IsException(name)){JS_FreeValue(c,pts);return name;}
  auto ubers=JS_GetPropertyStr(c,args[0],"ubers");if(JS_IsException(ubers)){JS_FreeValue(c,pts);JS_FreeValue(c,name);return ubers;}
  double n=0,u=0;bool valid=JS_IsNumber(pts)&&JS_IsString(name)&&JS_IsNumber(ubers);
  if(valid)valid=JS_ToFloat64(c,&n,pts)==0&&JS_ToFloat64(c,&u,ubers)==0&&std::isfinite(n)&&n==std::floor(n)&&n>=0&&n<=1000000&&std::isfinite(u)&&u==std::floor(u)&&u>=0&&u<=16;
  size_t size=0;const char* label=nullptr;if(valid){label=JS_ToCStringLen(c,&size,name);valid=label&&size<=256;}
  if(label)JS_FreeCString(c,label);JS_FreeValue(c,pts);JS_FreeValue(c,name);JS_FreeValue(c,ubers);
  if(!valid)return JS_ThrowTypeError(c,"invalid score output");return JS_NewFloat64(c,n);
 }
 void release(){if(ctx){for(auto& entry:functions)JS_FreeValue(ctx,entry.second);functions.clear();JS_FreeValue(ctx,compiled);JS_FreeValue(ctx,tagGetter);JS_FreeContext(ctx);ctx=nullptr;}if(rt){JS_FreeRuntime(rt);rt=nullptr;}}
public:
 QuickJSProbe(){try{
  rt=JS_NewRuntime();require(rt,"NewRuntime");JS_SetMemoryLimit(rt,32*1024*1024);JS_SetMaxStackSize(rt,512*1024);
  ctx=JS_NewContext(rt);require(ctx,"NewContext");JS_SetContextOpaque(ctx,this);
  auto g=JS_GetGlobalObject(ctx);
  JS_SetPropertyStr(ctx,g,"consume",JS_NewCFunction(ctx,consume,"consume",1));
  JS_SetPropertyStr(ctx,g,"scalar",JS_NewCFunction(ctx,scalar,"scalar",1));
  JS_SetPropertyStr(ctx,g,"copiedSum",JS_NewCFunction(ctx,copiedSum,"copiedSum",0));
  JS_SetPropertyStr(ctx,g,"detachForProbe",JS_NewCFunction(ctx,detachForProbe,"detachForProbe",1));
  JS_SetPropertyStr(ctx,g,"acceptScore",JS_NewCFunction(ctx,acceptScore,"acceptScore",1));JS_FreeValue(ctx,g);
#ifndef QUICKJS_NG
  tagGetter=eval("Object.getOwnPropertyDescriptor(Object.getPrototypeOf(Float64Array.prototype),Symbol.toStringTag).get");
#endif
 }catch(...){release();throw;}}
 ~QuickJSProbe(){release();}
 QuickJSProbe(const QuickJSProbe&)=delete;QuickJSProbe& operator=(const QuickJSProbe&)=delete;
 std::string version(){
#ifdef QUICKJS_NG
  return std::string("quickjs-ng ")+JS_GetVersion()+" commit=6d46d07d04041b40f4f49eaa7fdebe44c314c699 interpreter";
#else
  return "quickjs-bellard 2026-06-04 archive=b376e839b322978313d929fd20663b11ba58b75df5a46c126dd19ea2fa70ad2a interpreter";
#endif
 }
 static std::string configuration(){return "heap_limit_bytes=33554432 stack_limit_bytes=524288";}
 void compile(const std::string& s){compiled=checked(JS_Eval(ctx,s.data(),s.size(),"fixture-and-adapters.js",JS_EVAL_TYPE_GLOBAL|JS_EVAL_FLAG_COMPILE_ONLY));}
 void execute(){auto v=compiled;compiled=JS_UNDEFINED;v=checked(JS_EvalFunction(ctx,v));JS_FreeValue(ctx,v);}
 void cache(const std::string& name){if(functions.count(name))return;auto v=eval(name);if(!JS_IsFunction(ctx,v)){JS_FreeValue(ctx,v);throw std::runtime_error("not function: "+name);}functions.emplace(name,v);}
 double call(const std::string& name){cache(name);auto v=checked(JS_Call(ctx,functions.at(name),JS_UNDEFINED,0,nullptr));double n=0;int rc=JS_ToFloat64(ctx,&n,v);JS_FreeValue(ctx,v);require(!rc,"call number conversion");return n;}
 double number(const std::string& s){auto v=eval(s);double n=0;int rc=JS_ToFloat64(ctx,&n,v);JS_FreeValue(ctx,v);require(!rc,"number conversion");return n;}
 std::string text(const std::string& s){auto v=eval(s);try{auto out=string(v);JS_FreeValue(ctx,v);return out;}catch(...){JS_FreeValue(ctx,v);throw;}}
 void drainKnownJob(){JSContext* job=nullptr;int count=0,rc=0;while((rc=JS_ExecutePendingJob(rt,&job))>0){require(++count<=4,"known-job budget");}if(rc<0){auto ex=JS_GetException(job);JS_FreeValue(job,ex);throw std::runtime_error("pending job failed");}require(count==1,"expected one job");}
 void gc(){JS_RunGC(rt);}
 void memory(const char* stage){JSMemoryUsage m{};JS_ComputeMemoryUsage(rt,&m);std::cout<<"memory stage="<<stage<<" used_bytes="<<m.memory_used_size<<" allocated_bytes="<<m.malloc_size<<" objects="<<m.obj_count<<" automatic_gc_count=unavailable gc_pause_causality=unavailable rss_kib="<<rss()<<'\n';}
};
int main(int argc,char** argv){return probeMain<QuickJSProbe>(argc,argv);}
