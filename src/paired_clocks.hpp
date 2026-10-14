#pragma once
#include <array>
#include <cstdint>
#include <fstream>
#include <string>

namespace fl {
enum class FrameStage {Render,Events,Physics,Player,Context,Swap,Pacing};
// Main-thread only, like Profiler. No allocation or serialization in a scope.
class PairedClocks {
public:
 struct ClockApi {
  bool (*wall)(uint64_t&);
  bool (*cpu)(uint64_t&);
 };
 explicit PairedClocks(const std::string& path,unsigned cadence,const ClockApi* test_api=nullptr);
 ~PairedClocks();
 PairedClocks(const PairedClocks&)=delete;PairedClocks& operator=(const PairedClocks&)=delete;
 void flush();
 class Scope {
 public:
  Scope()=default;
  Scope(PairedClocks* p,uint64_t frame,int view,FrameStage stage,const char* pass="none"){start(p,frame,view,stage,pass);}
  ~Scope(){stop();}
  Scope(const Scope&)=delete;Scope& operator=(const Scope&)=delete;
  void start(PairedClocks* p,uint64_t frame,int view,FrameStage stage,const char* pass="none");
  void stop();
 private:
  PairedClocks* p_=nullptr;
  struct Sample {
   uint64_t sequence=0,frame=0,wall_start=0,wall_end=0,cpu_start=0,cpu_end=0;
   int view=-1;FrameStage stage=FrameStage::Render;const char* pass="none";
   bool wall_start_ok=false,wall_end_ok=false,cpu_start_ok=false,cpu_end_ok=false;
  } sample_;
  friend class PairedClocks;
 };
private:
 using Sample=Scope::Sample;
 static constexpr size_t capacity=64;
 std::array<Sample,capacity> samples_{};size_t count_=0;
 std::ofstream out_;ClockApi clocks_;unsigned cadence_;
 uint64_t run_start_=0,attempted_=0,written_=0,dropped_=0,invalid_wall_=0,invalid_cpu_=0,active_=0;
 long pid_=0,tid_=0;
 void prefix(const char* record);
};
}
