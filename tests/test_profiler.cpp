#include "profiler.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
namespace {
unsigned next_id=0,deleted=0,begins=0,ends=0,reads=0;bool active=false,ready=false,disjoint=false;
void gen(GLsizei,GLuint* id){*id=++next_id;}
void del(GLsizei,const GLuint*){++deleted;assert(!active);}
void begin(GLenum,GLuint){assert(!active);active=true;++begins;}
void end(GLenum){assert(active);active=false;++ends;}
void get(GLuint,GLenum,GLuint* r){*r=ready;}
void get64(GLuint,GLenum,GLuint64* r){assert(ready);++reads;*r=123000;}
void integer(GLenum,GLint* r){*r=disjoint;disjoint=false;}
std::string contents(const std::string& p){std::ifstream f(p);return {std::istreambuf_iterator<char>(f),{}};}
}
int main(int argc,char** argv){
 assert(argc==2);std::filesystem::create_directories(argv[1]);std::string path=std::string(argv[1])+"/profiler.csv";
 fl::QueryApi api{gen,del,begin,end,get,get64,integer};
 using P=fl::RenderPass;using S=fl::Profiler::Scope;
 {
  fl::Profiler p(path,fl::GpuMode::Passes,1,&api);
  for(int frame=0;frame<6;++frame){for(int view=0;view<2;++view){S whole(&p,frame,view,P::View);{S scene(&p,frame,view,P::Scene);S nested(&p,frame,view,P::DynamicPrep);} {S post(&p,frame,view,P::Post);} {S out(&p,frame,view,P::Output);}}p.poll();}
  assert(begins==32&&reads==0); // pool exhausted; no result read before available
  disjoint=true;p.poll();ready=true;p.poll();assert(reads==0); // invalid samples discarded
  try{S scene(&p,9,1,P::Scene);throw std::runtime_error("scope unwind");}catch(const std::runtime_error&){}
  assert(!active);p.poll();assert(reads==1);
  {S scene(&p,10,0,P::Scene);S invalid_nested(&p,10,0,P::Post);} // suppress misuse, preserve owner
  {S invalid_view(&p,10,2,P::Scene);}
  ready=false; // unresolved at shutdown, never wait
 }
 assert(begins==ends&&deleted==32);
 auto gpu=contents(path+".gpu.csv"),stats=contents(path+".stats.csv");
 assert(gpu.find("9,1,scene,valid,0.123")!=std::string::npos);
 assert(gpu.find("10,0,scene,pending,")!=std::string::npos);
 assert(stats.find("passes,1,1,34,1,32,")!=std::string::npos);
 assert(stats.find(",4,0,2,1,1,0,2,63\n")!=std::string::npos); // skipped, unsupported, misuse, disjoint polls, pending, dropped, schema, CPU rows
 unsigned before=begins;ready=true;
 {fl::Profiler p(path+"view",fl::GpuMode::View,2,&api);{S whole(&p,0,0,P::View);S scene(&p,0,0,P::Scene);} {S unsampled(&p,1,0,P::View);}p.poll();}
 assert(begins==before+1);
 {fl::QueryApi unsupported;fl::Profiler p(path+"unsupported",fl::GpuMode::Passes,1,&unsupported);S s(&p,0,0,P::Scene);}
 assert(contents(path+"unsupported.gpu.csv")=="frame,view,pass,status,gpu_ms\n");
 assert(contents(path+"unsupported.stats.csv").find("passes,1,0,0,0,0,0,0,1,0,0,0,0")!=std::string::npos);
 {fl::Profiler p(path+"off",fl::GpuMode::Off,1,&api);S s(&p,0,0,P::View);}
 assert(begins==before+1);
 {fl::Profiler p(path+"active-disjoint",fl::GpuMode::Passes,1,&api);{S s(&p,0,0,P::Scene);disjoint=true;p.poll();}}
 assert(contents(path+"active-disjoint.gpu.csv").find("0,0,scene,disjoint,\n")!=std::string::npos);
 assert(contents(path+"view.stats.csv").find(",0,2,2\n")!=std::string::npos);
 assert(contents(path+"unsupported.stats.csv").find(",0,2,1\n")!=std::string::npos);
 {fl::Profiler p(path+"overflow",fl::GpuMode::Off,1,&api);for(int i=0;i<40;++i){S s(&p,0,0,P::View);}}
 assert(contents(path+"overflow.stats.csv").find(",8,2,32\n")!=std::string::npos);
 bool rejected=false;try{fl::Profiler p(path,fl::GpuMode::Off,0,&api);}catch(const std::runtime_error&){rejected=true;}assert(rejected);
 std::cout<<"query lifecycle, bounded pool, availability, disjoint, pending, cadence, nonnesting, misuse, unwind, unsupported/off passed\n";
}
