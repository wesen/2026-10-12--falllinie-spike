// Raw V8 embedding via pinned Debian libnode108. NO node:: initialization/runner.
#include <cstring>
#include <v8.h>
#include <libplatform/libplatform.h>
#include "probe-common.hpp"
class V8Probe {
 std::unique_ptr<v8::ArrayBuffer::Allocator> allocator;
 v8::Isolate* isolate=nullptr;v8::Global<v8::Context> context;v8::Global<v8::Script> compiled;
 std::map<std::string,v8::Global<v8::Function>> functions;CopyState copied;
 unsigned gcObserved=0,explicitRequests=0;
 static V8Probe* self(const v8::FunctionCallbackInfo<v8::Value>& args){return static_cast<V8Probe*>(args.Data().As<v8::External>()->Value());}
 static v8::Local<v8::String> str(v8::Isolate* i,const std::string& s){return v8::String::NewFromUtf8(i,s.data(),v8::NewStringType::kNormal,static_cast<int>(s.size())).ToLocalChecked();}
 static void reject(v8::Isolate* i,const char* s){i->ThrowException(v8::Exception::TypeError(str(i,s)));}
 static void consume(const v8::FunctionCallbackInfo<v8::Value>& args){
  auto* e=self(args);auto* i=args.GetIsolate();
  if(args.Length()!=1||!args[0]->IsFloat64Array()){reject(i,"Float64Array required");return;}
  auto view=args[0].As<v8::Float64Array>();auto buffer=view->Buffer();
  if(buffer->WasDetached()||view->ByteLength()>749*8){reject(i,"invalid batch size or detached buffer");return;}
  auto backing=buffer->GetBackingStore();size_t offset=view->ByteOffset(),bytes=view->ByteLength();
  if(offset>backing->ByteLength()||bytes>backing->ByteLength()-offset||(!backing->Data()&&bytes)){reject(i,"invalid batch bounds");return;}
  // Rooted view + shared backing owner; native-owned copy; no JS calls while borrowing.
  double empty=0;const void* data=bytes?static_cast<const void*>(static_cast<const uint8_t*>(backing->Data())+offset):static_cast<const void*>(&empty);
  if(!e->copied.copy(data,bytes)){reject(i,"nonfinite numeric batch");return;}
  args.GetReturnValue().Set(e->copied.sum);
 }
 static void scalar(const v8::FunctionCallbackInfo<v8::Value>& args){if(args.Length()!=1||!args[0]->IsNumber()||!std::isfinite(args[0].As<v8::Number>()->Value())){reject(args.GetIsolate(),"finite numeric scalar required");return;}args.GetReturnValue().Set(args[0]);}
 static void detachForProbe(const v8::FunctionCallbackInfo<v8::Value>& args){
  if(args.Length()!=1||!args[0]->IsArrayBuffer()||!args[0].As<v8::ArrayBuffer>()->IsDetachable()){reject(args.GetIsolate(),"detachable buffer required");return;}
  args[0].As<v8::ArrayBuffer>()->Detach();
 }
 static void copiedSum(const v8::FunctionCallbackInfo<v8::Value>& args){args.GetReturnValue().Set(self(args)->copied.sum);}
 static void acceptScore(const v8::FunctionCallbackInfo<v8::Value>& args){
  auto* i=args.GetIsolate();auto c=i->GetCurrentContext();
  if(args.Length()!=1||!args[0]->IsObject()){reject(i,"score object required");return;}
  auto obj=args[0].As<v8::Object>();v8::Local<v8::Value> pts,name,ubers;
  // Preserve any original property-get exception; do not ToLocalChecked here.
  if(!obj->Get(c,str(i,"pts")).ToLocal(&pts)||!obj->Get(c,str(i,"name")).ToLocal(&name)||!obj->Get(c,str(i,"ubers")).ToLocal(&ubers))return;
  if(!pts->IsNumber()||!name->IsString()||!ubers->IsNumber()){reject(i,"invalid score output");return;}
  double n=pts.As<v8::Number>()->Value(),u=ubers.As<v8::Number>()->Value();
  if(!std::isfinite(n)||n!=std::floor(n)||n<0||n>1000000||!std::isfinite(u)||u!=std::floor(u)||u<0||u>16||name.As<v8::String>()->Utf8Length(i)>256){reject(i,"invalid score output");return;}
  args.GetReturnValue().Set(n); // no game side effects, fully validated first
 }
 static void observedGC(v8::Isolate*,v8::GCType,v8::GCCallbackFlags,void* data){++static_cast<V8Probe*>(data)->gcObserved;}
 std::string error(v8::TryCatch& tc){v8::String::Utf8Value text(isolate,tc.Exception());return *text?std::string(*text,text.length()):"exception formatting unavailable";}
 v8::Local<v8::Value> eval(v8::Local<v8::Context> c,const std::string& source){
  v8::TryCatch tc(isolate);v8::Local<v8::Script> s;v8::Local<v8::Value> value;
  if(!v8::Script::Compile(c,str(isolate,source)).ToLocal(&s)||!s->Run(c).ToLocal(&value))throw std::runtime_error(error(tc));return value;
 }
public:
 V8Probe(){
  allocator.reset(v8::ArrayBuffer::Allocator::NewDefaultAllocator());
  v8::Isolate::CreateParams params;params.array_buffer_allocator=allocator.get();
  params.constraints.set_max_old_generation_size_in_bytes(24*1024*1024);
  params.constraints.set_max_young_generation_size_in_bytes(8*1024*1024);
  isolate=v8::Isolate::New(params);require(isolate,"Isolate::New");
  isolate->SetMicrotasksPolicy(v8::MicrotasksPolicy::kExplicit);
  isolate->AddGCPrologueCallback(observedGC,this);
  v8::Isolate::Scope enter(isolate);v8::HandleScope handles(isolate);
  auto c=v8::Context::New(isolate);context.Reset(isolate,c);v8::Context::Scope realm(c);
  auto data=v8::External::New(isolate,this);
  for(auto entry:std::vector<std::pair<const char*,v8::FunctionCallback>>{{"consume",consume},{"scalar",scalar},{"copiedSum",copiedSum},{"acceptScore",acceptScore},{"detachForProbe",detachForProbe}}){
   auto f=v8::Function::New(c,entry.second,data).ToLocalChecked();c->Global()->Set(c,str(isolate,entry.first),f).Check();
  }
 }
 ~V8Probe(){
  // No Local/HandleScope/Context::Scope may survive this point.
  for(auto& entry:functions)entry.second.Reset();functions.clear();compiled.Reset();context.Reset();
  isolate->RemoveGCPrologueCallback(observedGC,this);isolate->Dispose(); // allocator outlives isolate
 }
 V8Probe(const V8Probe&)=delete;V8Probe& operator=(const V8Probe&)=delete;
 std::string version(){return std::string("v8 ")+v8::V8::GetVersion()+" Debian-libnode108 Node18.20.4+dfsg-1~deb12u2 raw-C++-API default-JIT not-interpreter standalone-build=false";}
 const CopyState& copyState() const{return copied;} // qualification-only native state observation
 static std::string configuration(){return "old_generation_limit_bytes=25165824 young_generation_limit_bytes=8388608 flags=--expose-gc,--stack-size=512 microtasks=explicit";}
 void compile(const std::string& source){v8::Isolate::Scope enter(isolate);v8::HandleScope handles(isolate);auto c=context.Get(isolate);v8::Context::Scope realm(c);v8::TryCatch tc(isolate);v8::Local<v8::Script> s;v8::ScriptOrigin origin(isolate,str(isolate,"fixture-and-adapters.js"));if(!v8::Script::Compile(c,str(isolate,source),&origin).ToLocal(&s))throw std::runtime_error(error(tc));compiled.Reset(isolate,s);}
 void execute(){v8::Isolate::Scope enter(isolate);v8::HandleScope handles(isolate);auto c=context.Get(isolate);v8::Context::Scope realm(c);v8::TryCatch tc(isolate);v8::Local<v8::Value> v;if(!compiled.Get(isolate)->Run(c).ToLocal(&v))throw std::runtime_error(error(tc));compiled.Reset();}
 void cache(const std::string& name){if(functions.count(name))return;v8::Isolate::Scope enter(isolate);v8::HandleScope handles(isolate);auto c=context.Get(isolate);v8::Context::Scope realm(c);auto value=eval(c,name);require(value->IsFunction(),"not function: "+name);functions.emplace(name,v8::Global<v8::Function>(isolate,value.As<v8::Function>()));}
 double call(const std::string& name){cache(name);v8::Isolate::Scope enter(isolate);v8::HandleScope handles(isolate);auto c=context.Get(isolate);v8::Context::Scope realm(c);v8::TryCatch tc(isolate);v8::Local<v8::Value> value;if(!functions.at(name).Get(isolate)->Call(c,v8::Undefined(isolate),0,nullptr).ToLocal(&value))throw std::runtime_error(error(tc));require(value->IsNumber(),"call nonnumber");return value.As<v8::Number>()->Value();}
 double number(const std::string& s){v8::Isolate::Scope enter(isolate);v8::HandleScope handles(isolate);auto c=context.Get(isolate);v8::Context::Scope realm(c);auto v=eval(c,s);require(v->IsNumber(),"nonnumber");return v.As<v8::Number>()->Value();}
 std::string text(const std::string& s){v8::Isolate::Scope enter(isolate);v8::HandleScope handles(isolate);auto c=context.Get(isolate);v8::Context::Scope realm(c);auto v=eval(c,s);v8::String::Utf8Value t(isolate,v);require(*t,"string conversion");return std::string(*t,t.length());}
 void drainKnownJob(){v8::Isolate::Scope enter(isolate);v8::HandleScope handles(isolate);auto c=context.Get(isolate);v8::Context::Scope realm(c);isolate->PerformMicrotaskCheckpoint();} // one trusted finite job, not arbitrary queue qualification
 void gc(){v8::Isolate::Scope enter(isolate);v8::HandleScope handles(isolate);++explicitRequests;isolate->RequestGarbageCollectionForTesting(v8::Isolate::kFullGarbageCollection);}
 void memory(const char* stage){v8::HeapStatistics h;isolate->GetHeapStatistics(&h);std::cout<<"memory stage="<<stage<<" used_heap_bytes="<<h.used_heap_size()<<" total_heap_bytes="<<h.total_heap_size()<<" heap_size_limit_bytes="<<h.heap_size_limit()<<" external_bytes="<<h.external_memory()<<" gc_prologue_observed="<<gcObserved<<" explicit_gc_requests="<<explicitRequests<<" gc_pause_causality=unavailable rss_kib="<<rss()<<'\n';}
};
int main(int argc,char** argv){
 // Platform is process-owned; every isolate is disposed before platform shutdown.
 v8::V8::SetFlagsFromString("--expose-gc --stack-size=512");
 auto platform=v8::platform::NewDefaultPlatform(1);v8::V8::InitializePlatform(platform.get());v8::V8::Initialize();
 int rc=probeMain<V8Probe>(argc,argv);v8::V8::Dispose();v8::V8::ShutdownPlatform();return rc;
}
