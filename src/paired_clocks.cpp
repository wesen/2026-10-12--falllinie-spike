#include "paired_clocks.hpp"
#include <iostream>
#include <stdexcept>
#include <time.h>
#include <unistd.h>
#if defined(__linux__)
#include <sys/syscall.h>
#endif

namespace fl {
namespace {
bool wall_clock(uint64_t& ns){
 timespec t{};if(clock_gettime(CLOCK_MONOTONIC,&t)!=0)return false;
 ns=uint64_t(t.tv_sec)*1000000000+uint64_t(t.tv_nsec);return true;
}
bool cpu_clock(uint64_t& ns){
#if defined(__linux__) && defined(CLOCK_THREAD_CPUTIME_ID)
 timespec t{};if(clock_gettime(CLOCK_THREAD_CPUTIME_ID,&t)!=0)return false;
 ns=uint64_t(t.tv_sec)*1000000000+uint64_t(t.tv_nsec);return true;
#else
 (void)ns;return false;
#endif
}
const char* stage_name(FrameStage s){
 switch(s){case FrameStage::Render:return "render";case FrameStage::Events:return "events";case FrameStage::Physics:return "physics";case FrameStage::Player:return "player";case FrameStage::Context:return "context";case FrameStage::Swap:return "swap";case FrameStage::Pacing:return "pacing";}return "invalid";
}
const char* status(bool start,bool end,uint64_t a,uint64_t b){
 if(!start&&!end)return "both_failed";
 if(!start)return "start_failed";
 if(!end)return "end_failed";
 return b<a?"regressed":"valid";
}
void timestamp(std::ostream& out,bool ok,uint64_t ns){if(ok)out<<ns;else out<<"null";}
}
PairedClocks::PairedClocks(const std::string& path,unsigned cadence,const ClockApi* test_api):clocks_(test_api?*test_api:ClockApi{wall_clock,cpu_clock}),cadence_(cadence){
 if(path.empty())return; // disabled: not even an initial clock/identity read
 if(cadence<1||cadence>3600||!clocks_.wall||!clocks_.cpu)throw std::runtime_error("invalid paired clock configuration");
 if(!clocks_.wall(run_start_))throw std::runtime_error("paired clocks: CLOCK_MONOTONIC unavailable at startup");
 pid_=getpid();
#if defined(__linux__)
 tid_=syscall(SYS_gettid);
#else
 throw std::runtime_error("paired clocks require Linux TID identity");
#endif
 if(tid_<=0)throw std::runtime_error("paired clocks: cannot read Linux TID");
 out_.open(path);if(!out_)throw std::runtime_error("cannot open paired clock sidecar");
 prefix("meta");out_<<",\"cadence\":"<<cadence_<<",\"capacity\":"<<capacity<<",\"wall_clock\":\"CLOCK_MONOTONIC\",\"cpu_clock\":\"CLOCK_THREAD_CPUTIME_ID\"}\n";
}
void PairedClocks::prefix(const char* record){
 out_<<"{\"record\":\""<<record<<"\",\"schema\":1,\"pid\":"<<pid_<<",\"tid\":"<<tid_<<",\"run_start_ns\":"<<run_start_;
}
void PairedClocks::Scope::start(PairedClocks* p,uint64_t frame,int view,FrameStage stage,const char* pass){
 if(p_||!p||!p->out_.is_open()||frame%p->cadence_)return;
 p_=p;sample_={};sample_.sequence=++p->attempted_;++p->active_;
 sample_.frame=frame;sample_.view=view;sample_.stage=stage;sample_.pass=pass;
 // Wall brackets CPU reads. Differences include read skew/accounting residual.
 sample_.wall_start_ok=p->clocks_.wall(sample_.wall_start);
 sample_.cpu_start_ok=p->clocks_.cpu(sample_.cpu_start);
}
void PairedClocks::Scope::stop(){
 if(!p_)return;
 sample_.cpu_end_ok=p_->clocks_.cpu(sample_.cpu_end);
 sample_.wall_end_ok=p_->clocks_.wall(sample_.wall_end);
 --p_->active_;
 if(!sample_.wall_start_ok||!sample_.wall_end_ok||sample_.wall_end<sample_.wall_start)++p_->invalid_wall_;
 if(!sample_.cpu_start_ok||!sample_.cpu_end_ok||sample_.cpu_end<sample_.cpu_start)++p_->invalid_cpu_;
 if(p_->count_<capacity)p_->samples_[p_->count_++]=sample_;else ++p_->dropped_;
 p_=nullptr;
}
void PairedClocks::flush(){
 if(!out_.is_open()||active_)return; // never serialize within a measured paired scope
 for(size_t i=0;i<count_;++i){const auto& s=samples_[i];
  prefix("sample");out_<<",\"sequence\":"<<s.sequence<<",\"frame\":"<<s.frame<<",\"view\":"<<s.view<<",\"pass\":\""<<s.pass<<"\",\"stage\":\""<<stage_name(s.stage)<<"\",\"wall_start_ns\":";
  timestamp(out_,s.wall_start_ok,s.wall_start);out_<<",\"wall_end_ns\":";timestamp(out_,s.wall_end_ok,s.wall_end);
  out_<<",\"cpu_start_ns\":";timestamp(out_,s.cpu_start_ok,s.cpu_start);out_<<",\"cpu_end_ns\":";timestamp(out_,s.cpu_end_ok,s.cpu_end);
  out_<<",\"wall_status\":\""<<status(s.wall_start_ok,s.wall_end_ok,s.wall_start,s.wall_end)<<"\",\"cpu_status\":\""<<status(s.cpu_start_ok,s.cpu_end_ok,s.cpu_start,s.cpu_end)<<"\"}\n";
  if(out_)++written_;
 }
 count_=0;
}
PairedClocks::~PairedClocks(){
 if(!out_.is_open())return;
 flush();prefix("final");out_<<",\"attempted\":"<<attempted_<<",\"written\":"<<written_<<",\"dropped\":"<<dropped_<<",\"invalid_wall\":"<<invalid_wall_<<",\"invalid_cpu\":"<<invalid_cpu_<<",\"active\":"<<active_<<"}\n";
 out_.flush();if(!out_)std::cerr<<"paired clocks: sidecar write failed; collection incomplete\n";
}
}
