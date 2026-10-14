#include "profiler.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {
uint64_t wall=1000000,cpu=0;unsigned walls=0,cpus=0;bool fail_wall=false,fail_cpu=false;
bool wall_read(uint64_t& ns){++walls;ns=wall;wall+=100;return !fail_wall;}
bool cpu_read(uint64_t& ns){++cpus;ns=cpu;cpu+=20;return !fail_cpu;}
std::string contents(const std::string& path){std::ifstream in(path);return {std::istreambuf_iterator<char>(in),{}};}
}
int main(int argc,char** argv){
 assert(argc==2);std::filesystem::create_directories(argv[1]);const std::string root=std::string(argv[1])+"/";
 using C=fl::PairedClocks;using S=C::Scope;using F=fl::FrameStage;
 C::ClockApi clocks{wall_read,cpu_read};
 {C disabled("",1,&clocks);S s(&disabled,0,-1,F::Events);}
 assert(walls==0&&cpus==0);
 {fl::Profiler disabled(root+"legacy.csv",fl::GpuMode::Off,1,nullptr,false,&clocks);fl::Profiler::Scope s(&disabled,0,0,fl::RenderPass::View);}
 assert(walls==0&&cpus==0);assert(!std::filesystem::exists(root+"legacy.csv.paired.jsonl"));
 bool rejected=false;try{fl::Profiler p("",fl::GpuMode::Off,1,nullptr,true,&clocks);}catch(const std::runtime_error&){rejected=true;}assert(rejected);assert(walls==0&&cpus==0);
 {
  C p(root+"valid.jsonl",2,&clocks);
  {S s(&p,1,-1,F::Events);}assert(cpus==0); // unsampled has no clock reads
  {S s(&p,0,-1,F::Events);s.stop();s.stop();} // zero initial CPU timestamp is valid
  {S parent(&p,2,0,F::Render,"view");{S child(&p,2,0,F::Render,"scene");}p.flush();} // flush while active is deferred
  try{S s(&p,4,-1,F::Player);throw std::runtime_error("unwind");}catch(const std::runtime_error&){}
 }
 auto valid=contents(root+"valid.jsonl");
 assert(valid.find("\"cpu_start_ns\":0,\"cpu_end_ns\":20")!=std::string::npos);
 assert(valid.find("\"attempted\":4,\"written\":4,\"dropped\":0,\"invalid_wall\":0,\"invalid_cpu\":0,\"active\":0")!=std::string::npos);
 {
  C p(root+"failures.jsonl",1,&clocks);
  {fail_cpu=true;S s(&p,0,-1,F::Events);fail_cpu=false;}
  {S s(&p,1,-1,F::Physics);fail_cpu=true;}fail_cpu=false;
  {fail_cpu=true;S s(&p,2,-1,F::Player);}fail_cpu=false;
  {fail_wall=true;S s(&p,3,-1,F::Pacing);fail_wall=false;}
  {S s(&p,4,0,F::Swap);fail_wall=true;}fail_wall=false;
  {fail_wall=true;S s(&p,5,0,F::Context);}fail_wall=false;
  {S s(&p,6,1,F::Context);cpu=0;wall=0;} // regressed clocks, not valid duration
 }
 const auto failures=contents(root+"failures.jsonl");
 for(auto status:{"start_failed","end_failed","both_failed","regressed"})assert(failures.find(status)!=std::string::npos);
 assert(failures.find("\"cpu_start_ns\":null")!=std::string::npos);
 assert(failures.find("\"invalid_wall\":4,\"invalid_cpu\":4")!=std::string::npos);
 {
  C p(root+"overflow.jsonl",1,&clocks);
  for(int i=0;i<70;++i){S s(&p,i,-1,F::Events);}
  p.flush();{S s(&p,70,-1,F::Pacing);}
 }
 assert(contents(root+"overflow.jsonl").find("\"attempted\":71,\"written\":65,\"dropped\":6")!=std::string::npos);
 {
  fl::Profiler p(root+"integrated.csv",fl::GpuMode::Off,2,nullptr,true,&clocks);
  {fl::Profiler::Scope s(&p,0,0,fl::RenderPass::View);{fl::Profiler::Scope child(&p,0,0,fl::RenderPass::Scene);}}
  {fl::Profiler::Scope s(&p,1,1,fl::RenderPass::View);}
  {S s(p.paired(),2,-1,F::Physics);}p.poll();
 }
 assert(contents(root+"integrated.csv.paired.jsonl").find("\"attempted\":3,\"written\":3")!=std::string::npos);
 assert(contents(root+"integrated.csv.passes.csv").find("frame,view,pass,parent,cpu_ms\n")==0);
 fail_wall=true;rejected=false;try{C p(root+"startup-failed.jsonl",1,&clocks);}catch(const std::runtime_error&){rejected=true;}fail_wall=false;assert(rejected);
 std::cout<<"paired clocks: disabled, cadence, zero timestamp, nesting, deferred flush, unwind, failures, regressions, drops and profiler integration passed\n";
}
