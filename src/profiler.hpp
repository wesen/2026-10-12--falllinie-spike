#pragma once
#include <GLES3/gl3.h>
#include "paired_clocks.hpp"
#include <fstream>
#include <array>
#include <chrono>
#include <cstdint>
#include <string>
namespace fl {
// Injectable dispatch permits lifecycle tests without a GPU. All calls are main-thread only.
struct QueryApi {
 void (*gen)(GLsizei,GLuint*)=nullptr;void (*del)(GLsizei,const GLuint*)=nullptr;
 void (*begin)(GLenum,GLuint)=nullptr;void (*end)(GLenum)=nullptr;
 void (*get)(GLuint,GLenum,GLuint*)=nullptr;void (*get64)(GLuint,GLenum,GLuint64*)=nullptr;
 void (*integer)(GLenum,GLint*)=nullptr;
 explicit operator bool()const{return gen&&del&&begin&&end&&get&&get64&&integer;}
};
enum class GpuMode {Off,View,Passes};
enum class RenderPass {View,Scene,Post,Output,DynamicPrep,DynamicUpload};
class Profiler {
public:
 explicit Profiler(const std::string& path,GpuMode mode=GpuMode::Off,unsigned cadence=60,const QueryApi* test_api=nullptr,bool paired_clocks=false,const PairedClocks::ClockApi* test_clocks=nullptr);
 ~Profiler();
 Profiler(const Profiler&)=delete;Profiler& operator=(const Profiler&)=delete;
 PairedClocks* paired(){return paired_enabled_?&paired_:nullptr;}
 void poll();
 void cpu(uint64_t frame,double interval,double events,double physics,double player,double stream,double top_submit,double top_swap,double bottom_submit,double bottom_swap,double pace=0);
 bool enabled()const{return cpu_.is_open();}
 bool sampling(uint64_t frame)const{return enabled()&&frame%cadence_==0;}
 class Scope {
 public:
  Scope(Profiler* profiler,uint64_t frame,int view,RenderPass pass);
  ~Scope();
  Scope(const Scope&)=delete;Scope& operator=(const Scope&)=delete;
 private:
  Profiler* p_=nullptr;uint64_t frame_=0;int view_=0,query_=-1;RenderPass pass_=RenderPass::View;
  std::chrono::steady_clock::time_point start_;
  PairedClocks::Scope paired_scope_;
 };
private:
 PairedClocks paired_;bool paired_enabled_=false;
 struct Query{GLuint id=0;uint64_t frame=0;int view=0;RenderPass pass=RenderPass::View;bool pending=false,invalid=false;};
 struct CpuSample{uint64_t frame;int view;RenderPass pass;double ms;};
 std::array<Query,32> queries_{};
 std::array<CpuSample,32> samples_{};size_t sample_count_=0;
 std::ofstream cpu_,gpu_,passes_,stats_;
 QueryApi api_;GpuMode mode_;unsigned cadence_;int active_=-1;
 uint64_t issued_=0,valid_=0,invalid_=0,unavailable_=0,skipped_=0,unsupported_=0,misuse_=0,disjoint_polls_=0,cpu_dropped_=0,cpu_samples_=0;
 int begin(uint64_t frame,int view,RenderPass pass);
 void end(int token);
 void flush_cpu();
 void row(const Query& q,const char* status,double ms=-1);
};
}
