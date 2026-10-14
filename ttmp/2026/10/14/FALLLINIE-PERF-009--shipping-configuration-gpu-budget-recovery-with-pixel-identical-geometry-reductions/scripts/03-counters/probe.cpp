// Separate diagnostic process using Arm libGPUCounters; no GL or game changes.
#include <hwcpipe/counter_database.hpp>
#include <hwcpipe/gpu.hpp>
#include <hwcpipe/sampler.hpp>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <time.h>
#include <unistd.h>
struct Counter { hwcpipe_counter id; const char* key; };
#define C(x) {x,#x}
// PERF-009 copy of the PERF-004 probe: "base" is the original 13-counter set;
// "geometry" adds primitive culling, varying-shading and quad counters.
static const Counter base_set[]={C(MaliGPUActiveCy),C(MaliCoreActiveCy),C(MaliFragActiveCy),C(MaliNonFragActiveCy),C(MaliTilerActiveCy),C(MaliExtBusRdBy),C(MaliExtBusWrBy),C(MaliFragUtil),C(MaliNonFragUtil),C(MaliTexUtil),C(MaliALUUtil),C(MaliExtBusRdStallRate),C(MaliExtBusWrStallRate)};
static const Counter geometry_set[]={C(MaliGPUActiveCy),C(MaliFragActiveCy),C(MaliNonFragActiveCy),C(MaliGeomTotalPrim),C(MaliGeomVisiblePrim),C(MaliGeomFaceXYPlaneCullPrim),C(MaliGeomZPlaneCullPrim),C(MaliGeomSampleCullPrim),C(MaliTilerPosShadThread),C(MaliTilerVarShadThread),C(MaliFragRastQd),C(MaliFragThread),C(MaliFragEZSKillQd),C(MaliFragFPKKillQd),C(MaliExtBusRdBy),C(MaliExtBusWrBy)};
uint64_t now(){timespec t{};if(clock_gettime(CLOCK_MONOTONIC,&t))std::abort();return uint64_t(t.tv_sec)*1000000000ULL+t.tv_nsec;}
int main(int argc,char**argv){
 if(argc!=2&&argc!=3){std::cerr<<"usage: mali-counters SECONDS (1..120) [base|geometry]\n";return 2;}
 const std::string set=argc==3?argv[2]:"base";if(set!="base"&&set!="geometry"){std::cerr<<"unknown counter set\n";return 2;}
 const Counter* wanted=set=="base"?base_set:geometry_set;const size_t wanted_count=set=="base"?sizeof(base_set)/sizeof(*base_set):sizeof(geometry_set)/sizeof(*geometry_set);
 char* end=nullptr;long seconds=strtol(argv[1],&end,10);if(!end||*end||seconds<1||seconds>120)return 2;
 hwcpipe::gpu gpu(0);if(!gpu){std::cerr<<"GPU0 unavailable\n";return 3;}
 std::cerr<<"schema=1 set="<<set<<" pid="<<getpid()<<" scope=whole-device cores="<<gpu.num_shader_cores()<<" bus_width="<<gpu.bus_width()<<" period_ms=100 seconds="<<seconds<<"\n";
 hwcpipe::counter_database db;hwcpipe::counter_metadata meta{};
 for(auto id:db.counters_for_gpu(gpu)){auto ec=db.describe_counter(id,meta);if(!ec)std::cerr<<"available="<<int(id)<<" name="<<meta.name<<" units="<<meta.units<<"\n";}
 hwcpipe::sampler_config config(gpu);std::vector<Counter> selected;
 for(size_t k=0;k<wanted_count;++k){const auto c=wanted[k];auto ec=config.add_counter(c.id);if(ec){std::cerr<<"unsupported="<<c.key<<" error="<<ec.message()<<"\n";}else{selected.push_back(c);std::cerr<<"selected="<<c.key<<"\n";}}
 if(selected.empty())return 4;
 hwcpipe::sampler<> sampler(config);auto ec=sampler.start_sampling();if(ec){std::cerr<<"start_error="<<ec.message()<<"\n";return 5;}
 unsigned failed=0,rows=0,nonfinite=0;
 auto start=std::chrono::steady_clock::now();
 std::cout<<"sequence,request_ns,complete_ns,counter_timestamp_ns,counter,value,status,gpu_freq_snapshot_hz\n"<<std::setprecision(17);
 for(long i=0;i<seconds*10;++i){
  std::this_thread::sleep_until(start+std::chrono::milliseconds((i+1)*100));
  uint64_t request=now();ec=sampler.sample_now();uint64_t complete=now();
  uint64_t freq=0;std::ifstream frequency("/sys/class/devfreq/fde60000.gpu/cur_freq");frequency>>freq;
  if(ec){++failed;std::cerr<<"sample_error sequence="<<i<<" error="<<ec.message()<<"\n";continue;} // never reuse stale buffer
  for(auto c:selected){hwcpipe::counter_sample sample;auto e=sampler.get_counter_value(c.id,sample);
   if(e){++failed;std::cerr<<"counter_error sequence="<<i<<" counter="<<c.key<<" error="<<e.message()<<"\n";continue;}
   std::cout<<i<<','<<request<<','<<complete<<','<<sample.timestamp<<','<<c.key<<',';
   if(sample.type==hwcpipe::counter_sample::type::uint64){std::cout<<sample.value.uint64<<",valid";}
   else if(sample.type==hwcpipe::counter_sample::type::float64&&std::isfinite(sample.value.float64)){std::cout<<sample.value.float64<<",valid";}
   else{std::cout<<",nonfinite";++nonfinite;}
   std::cout<<','<<freq<<'\n';++rows;
  }
 }
 ec=sampler.stop_sampling();std::cout.flush();
 std::cerr<<"final rows="<<rows<<" errors="<<failed<<" nonfinite="<<nonfinite<<" stop="<<(ec?ec.message():"ok")<<"\n";
 return (ec||failed||!std::cout)?6:0;
}
