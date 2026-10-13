#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <sys/resource.h>
using Clock=std::chrono::steady_clock;
inline double us(Clock::time_point a){return std::chrono::duration<double,std::micro>(Clock::now()-a).count();}
inline std::string load(const char* path){std::ifstream f(path);if(!f)throw std::runtime_error(path);std::ostringstream s;s<<f.rdbuf();return s.str();}
inline void require(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
inline long peakRSS(){rusage r{};getrusage(RUSAGE_SELF,&r);return r.ru_maxrss;}
inline long rss(){std::ifstream f("/proc/self/status");std::string k;while(f>>k){if(k=="VmRSS:"){long n;f>>n;return n;}std::string rest;std::getline(f,rest);}return -1;}
struct CopyState { std::array<double,749> values{};size_t count=0;double sum=0;
 bool copy(const void* data,size_t bytes){if(!bytes||bytes%8||bytes>sizeof(values))return false;std::array<double,749> candidate{};std::memcpy(candidate.data(),data,bytes);double s=0;for(size_t i=0;i<bytes/8;i++){if(!std::isfinite(candidate[i]))return false;s+=candidate[i];}if(!std::isfinite(s))return false;values=candidate;count=bytes/8;sum=s;return true;}
};
inline void distribution(const std::string& name,std::vector<double> t,int warm,double checksum){std::sort(t.begin(),t.end());double total=0;for(double x:t)total+=x;std::cout<<"bench "<<name<<" n="<<t.size()<<" warmup="<<warm<<" p50_us="<<t[(t.size()-1)*50/100]<<" p95_us="<<t[(t.size()-1)*95/100]<<" p99_us="<<t[(t.size()-1)*99/100]<<" max_us="<<t.back()<<" total_us="<<total<<" calls_per_second="<<t.size()*1e6/total<<" checksum="<<checksum<<'\n';}
// Engine is a small qualification adapter, not a proposed production abstraction.
template<class Engine> int probeMain(int argc,char** argv){try{
 require(argc==3||argc==4,"usage: probe pure.js probe.js [sustained_blocks:1..600]");
 int blocks=argc==4?std::stoi(argv[3]):300;require(blocks>=1&&blocks<=600,"block limit");
 auto begin=Clock::now();auto a=Clock::now();auto source=load(argv[1])+"\n"+load(argv[2]);double sourceReadUs=us(a);std::cout.precision(17);
 std::cout<<"source_read_us="<<sourceReadUs<<" source_bytes="<<source.size()<<'\n';
 std::cout<<"timing_contract=operation_end_before_logging_v2\n";
 std::cout<<"config "<<Engine::configuration()<<" sustained_blocks="<<blocks<<" heap_limit_is_not_RSS_limit=true\n";
 // Every cycle actually compiles/executes fixture+adapters and drops every handle.
 for(int cycle=0;cycle<8;cycle++){
  a=Clock::now();auto e=std::make_unique<Engine>();double create=us(a);
  a=Clock::now();e->compile(source);double compile=us(a);
  a=Clock::now();e->execute();double execute=us(a);
  require(e->number("1+1")==2,"lifecycle sanity");
  a=Clock::now();e.reset();double dispose=us(a);
  std::cout<<"lifecycle cycle="<<cycle<<" create_us="<<create<<" compile_us="<<compile<<" execute_us="<<execute<<" dispose_us="<<dispose<<" rss_kib="<<rss()<<'\n';
 }
 Engine e;e.compile(source);e.execute();std::cout<<"engine "<<e.version()<<'\n';
 for(const char* name:{"noop","batch","sourceRules","scalarBridge","batchedBridge","churn"}){
  e.cache(name);a=Clock::now();double cold=e.call(name);double coldUs=us(a);std::cout<<"cold "<<name<<" first_call_us="<<coldUs<<" checksum="<<cold<<'\n';
  for(int i=0;i<30;i++)e.call(name);
  std::vector<double> times;double sum=0;
  for(int i=0;i<120;i++){a=Clock::now();double n=e.call(name);times.push_back(us(a));require(std::isfinite(n),"nonfinite benchmark");sum+=n;}
  distribution(name,times,30,sum);
 }
 std::cout<<"oracle "<<e.text("JSON.stringify(oracle())")<<'\n';
 require(e.call("protocolTests")==19,"protocol checks");
 CopyState expectedCopy;const double accepted[]={1,2,3};require(expectedCopy.copy(accepted,sizeof(accepted)),"expected copy setup");
 const auto& actualCopy=e.copyState();require(actualCopy.values==expectedCopy.values&&actualCopy.count==expectedCopy.count&&actualCopy.sum==expectedCopy.sum,"overflow changed native copied values/count/sum");
 bool caught=false;try{e.call("intentionalException");}catch(const std::exception& ex){caught=std::string(ex.what()).find("intentional qualification exception")!=std::string::npos;std::cout<<"expected_exception "<<ex.what()<<'\n';}require(caught,"exception not observed");require(e.number("2+2")==4,"exception recovery");
 require(e.call("scheduleJob")==0&&e.call("jobsObserved")==0,"jobs ran implicitly");e.drainKnownJob();require(e.call("jobsObserved")==1,"controlled job drain");std::cout<<"protocol PASS copy/offset/type/size/nonfinite/malformed-output/exception/jobs/detach/overflow-state-preserved checks=19 copied_count=3 copied_sum=6 copied_values_preserved=true\n";
 require(e.call("retainState")==128,"retained state");e.memory("retained");
 std::vector<double> times;double sum=0;
 for(int i=0;i<blocks;i++){a=Clock::now();for(int j=0;j<8;j++)sum+=e.call("batch");for(int j=0;j<4;j++){sum+=e.call("sourceRules");sum+=e.call("churn");}times.push_back(us(a));if(i==0||i==blocks-1)e.memory("sustained");require(rss()<192*1024,"RSS safety threshold exceeded");}
 distribution("sustained_block_8row_4rules_4churn",times,0,sum);
 require(e.call("dropState")==0,"drop state");e.memory("before_explicit_gc");a=Clock::now();e.gc();double gcUs=us(a);std::cout<<"explicit_gc_wall_us="<<gcUs<<'\n';e.memory("after_explicit_gc");
 const long finalPeakRSS=peakRSS();require(finalPeakRSS<192*1024,"peak RSS safety threshold exceeded");
 double elapsedMs=us(begin)/1000;
 std::cout<<"process peak_rss_kib="<<finalPeakRSS<<" elapsed_ms="<<elapsedMs<<" elapsed_definition=source_read_through_final_checks_includes_logging_excludes_final_dispose_and_platform_init\nPROBE PASS\n";
 return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
