#include "profiler.hpp"
#include "gl_api.hpp"
#include <iostream>
#include <iomanip>
namespace fl {
static constexpr GLenum elapsed=0x88BF,available=0x8867,result=0x8866,disjoint=0x8FBB;
static const char* name(RenderPass p){switch(p){case RenderPass::View:return "view";case RenderPass::Scene:return "scene";case RenderPass::Post:return "post";case RenderPass::Output:return "output";case RenderPass::DynamicPrep:return "dynamic_prep";case RenderPass::DynamicUpload:return "dynamic_upload";case RenderPass::Acquire:return "acquire";}return "invalid";}
Profiler::Profiler(const std::string& path,GpuMode mode,unsigned cadence,const QueryApi* test_api,bool paired_clocks,const PairedClocks::ClockApi* test_clocks):paired_(paired_clocks&&!path.empty()?path+".paired.jsonl":"",cadence,test_clocks),paired_enabled_(paired_clocks),mode_(mode),cadence_(cadence){
 if(paired_clocks&&path.empty())throw std::runtime_error("paired clocks require --profile CSV");
 if(cadence<1||cadence>3600)throw std::runtime_error("profile cadence must be 1..3600");
 if(path.empty())return;
 cpu_.open(path);gpu_.open(path+".gpu.csv");passes_.open(path+".passes.csv");stats_.open(path+".stats.csv");
 if(!cpu_||!gpu_||!passes_||!stats_)throw std::runtime_error("cannot open profile logs");
 cpu_<<"frame,interval_ms,events_ms,physics_ms,player_ms,stream_ms,top_submit_ms,top_swap_ms,bottom_submit_ms,bottom_swap_ms,pace_wait_ms\n";
 gpu_<<"frame,view,pass,status,gpu_ms\n";passes_<<"frame,view,pass,parent,cpu_ms\n";
 cpu_<<std::setprecision(9);gpu_<<std::setprecision(9);passes_<<std::setprecision(9);
 if(mode!=GpuMode::Off){
  if(test_api)api_=*test_api;
  else {
   const auto* ext=reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
   if(ext&&(" "+std::string(ext)+" ").find(" GL_EXT_disjoint_timer_query ")!=std::string::npos){
    api_.gen=reinterpret_cast<decltype(api_.gen)>(SDL_GL_GetProcAddress("glGenQueriesEXT"));
    api_.del=reinterpret_cast<decltype(api_.del)>(SDL_GL_GetProcAddress("glDeleteQueriesEXT"));
    api_.begin=reinterpret_cast<decltype(api_.begin)>(SDL_GL_GetProcAddress("glBeginQueryEXT"));
    api_.end=reinterpret_cast<decltype(api_.end)>(SDL_GL_GetProcAddress("glEndQueryEXT"));
    api_.get=reinterpret_cast<decltype(api_.get)>(SDL_GL_GetProcAddress("glGetQueryObjectuivEXT"));
    api_.get64=reinterpret_cast<decltype(api_.get64)>(SDL_GL_GetProcAddress("glGetQueryObjectui64vEXT"));api_.integer=glGetIntegerv;
   }
  }
  if(api_)for(auto& q:queries_)api_.gen(1,&q.id);
 }
 std::cout<<"PROFILE mode="<<(mode==GpuMode::Off?"off":mode==GpuMode::View?"view":"passes")<<" cadence="<<cadence<<" supported="<<bool(api_)<<" pool=32 CPU_passes=sparse,nested,nonadditive\n";
}
void Profiler::row(const Query& q,const char* status,double ms){gpu_<<q.frame<<','<<q.view<<','<<name(q.pass)<<','<<status<<',';if(ms>=0)gpu_<<ms;gpu_<<'\n';}
void Profiler::flush_cpu(){for(size_t i=0;i<sample_count_;++i){const auto& s=samples_[i];passes_<<s.frame<<','<<s.view<<','<<name(s.pass)<<','<<(s.pass==RenderPass::View?"none":s.pass==RenderPass::DynamicPrep||s.pass==RenderPass::DynamicUpload?"scene":s.pass==RenderPass::Acquire?"output":"view")<<','<<s.ms<<'\n';++cpu_samples_;}sample_count_=0;}
void Profiler::poll(){
 paired_.flush();flush_cpu();if(!api_)return;
 GLint changed=0;api_.integer(disjoint,&changed);
 if(changed){++disjoint_polls_;for(auto& q:queries_)if(q.pending)q.invalid=true;if(active_>=0)queries_[size_t(active_)].invalid=true;}
 for(auto& q:queries_){if(!q.pending)continue;GLuint ready=0;api_.get(q.id,available,&ready);if(!ready){++unavailable_;continue;}
  // An invalid sample is retired without reading a meaningless result.
  if(q.invalid){++invalid_;row(q,"disjoint");}else{GLuint64 ns=0;api_.get64(q.id,result,&ns);++valid_;row(q,"valid",double(ns)/1e6);}q.pending=false;
 }
}
int Profiler::begin(uint64_t frame,int view,RenderPass pass){
 const bool selected=mode_==GpuMode::View?pass==RenderPass::View:mode_==GpuMode::Passes&&(pass==RenderPass::Scene||pass==RenderPass::Post||pass==RenderPass::Output);
 if(!selected)return -1;
 if(!api_){++unsupported_;return -1;}
 if(active_>=0){++misuse_;return -1;}
 for(size_t i=0;i<queries_.size();++i){auto& q=queries_[i];if(q.pending)continue;q.frame=frame;q.view=view;q.pass=pass;q.invalid=false;active_=int(i);api_.begin(elapsed,q.id);++issued_;return active_;}
 ++skipped_;return -1;
}
void Profiler::end(int token){if(token<0)return;if(token!=active_){++misuse_;return;}api_.end(elapsed);queries_[size_t(token)].pending=true;active_=-1;}
Profiler::Scope::Scope(Profiler* p,uint64_t f,int v,RenderPass pass):frame_(f),view_(v),pass_(pass){
 if(!p||!p->enabled()||f%p->cadence_)return;
 if(v<0||v>1){++p->misuse_;return;}p_=p;query_=p_->begin(f,v,pass);paired_scope_.start(p_->paired(),f,v,FrameStage::Render,name(pass));start_=std::chrono::steady_clock::now();
}
Profiler::Scope::~Scope(){if(!p_)return;const auto stop=std::chrono::steady_clock::now();paired_scope_.stop();p_->end(query_);if(p_->sample_count_<p_->samples_.size())p_->samples_[p_->sample_count_++]={frame_,view_,pass_,std::chrono::duration<double,std::milli>(stop-start_).count()};else ++p_->cpu_dropped_;}
void Profiler::cpu(uint64_t f,double i,double e,double p,double u,double s,double t,double ts,double b,double bs,double pace){if(enabled())cpu_<<f<<','<<i<<','<<e<<','<<p<<','<<u<<','<<s<<','<<t<<','<<ts<<','<<b<<','<<bs<<','<<pace<<'\n';}
Profiler::~Profiler(){
 if(!enabled())return;
 if(active_>=0){api_.end(elapsed);queries_[size_t(active_)].pending=true;queries_[size_t(active_)].invalid=true;active_=-1;++misuse_;}
 poll();size_t pending=0;if(api_)for(auto& q:queries_){if(q.pending){++pending;row(q,q.invalid?"pending_disjoint":"pending");}api_.del(1,&q.id);}
 stats_<<"mode,cadence,supported,issued,valid,disjoint,unavailable_polls,skipped_pool,unsupported_scopes,misuse,disjoint_polls,pending,cpu_dropped,schema_version,cpu_samples\n"<<(mode_==GpuMode::Off?"off":mode_==GpuMode::View?"view":"passes")<<','<<cadence_<<','<<bool(api_)<<','<<issued_<<','<<valid_<<','<<invalid_<<','<<unavailable_<<','<<skipped_<<','<<unsupported_<<','<<misuse_<<','<<disjoint_polls_<<','<<pending<<','<<cpu_dropped_<<",2,"<<cpu_samples_<<'\n';
}
}
