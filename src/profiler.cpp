#include "profiler.hpp"
#include "gl_api.hpp"
#include <iostream>
#include <iomanip>
namespace fl {
static constexpr GLenum elapsed=0x88BF,available=0x8867,result=0x8866,disjoint=0x8FBB;
Profiler::Profiler(const std::string& path,bool gpu){
 if(path.empty())return;
 cpu_.open(path);gpu_.open(path+".gpu.csv");if(!cpu_||!gpu_)throw std::runtime_error("cannot open profile logs");
 cpu_<<"frame,interval_ms,events_ms,physics_ms,player_ms,stream_ms,top_submit_ms,top_swap_ms,bottom_submit_ms,bottom_swap_ms\n";gpu_<<"frame,view,gpu_ms,disjoint\n";cpu_<<std::setprecision(9);gpu_<<std::setprecision(9);
 if(!gpu){std::cout<<"GPU queries disabled for unperturbed CPU timing\n";return;}
 auto ext=reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
 if(!ext||std::string(ext).find("GL_EXT_disjoint_timer_query")==std::string::npos){std::cout<<"GPU elapsed queries unsupported\n";return;}
 gen_=reinterpret_cast<Gen>(SDL_GL_GetProcAddress("glGenQueriesEXT"));del_=reinterpret_cast<Delete>(SDL_GL_GetProcAddress("glDeleteQueriesEXT"));begin_=reinterpret_cast<Begin>(SDL_GL_GetProcAddress("glBeginQueryEXT"));end_=reinterpret_cast<End>(SDL_GL_GetProcAddress("glEndQueryEXT"));get_=reinterpret_cast<Get>(SDL_GL_GetProcAddress("glGetQueryObjectuivEXT"));get64_=reinterpret_cast<Get64>(SDL_GL_GetProcAddress("glGetQueryObjectui64vEXT"));
 supported_=gen_&&del_&&begin_&&end_&&get_&&get64_;
 if(supported_)for(auto& frame:queries_)for(auto& q:frame)gen_(1,&q.id);
 std::cout<<"GPU elapsed queries="<<supported_<<'\n';
}
void Profiler::poll(){
 if(!supported_) return;
 GLint changed=0;glGetIntegerv(disjoint,&changed);
 if(changed)for(auto& frame:queries_)for(auto& q:frame)if(q.pending)q.invalid=true;
 for(auto& frame:queries_)for(int view=0;view<2;++view){auto& q=frame[view];if(!q.pending)continue;GLuint ready=0;get_(q.id,available,&ready);if(!ready)continue;GLuint64 ns=0;get64_(q.id,result,&ns);if(ns>500000000ULL)q.invalid=true;gpu_<<q.frame<<','<<view<<','<<double(ns)/1e6<<','<<q.invalid<<'\n';q.pending=false;}
}
void Profiler::begin_gpu(uint64_t frame,int view){if(!supported_ || frame%60!=0)return;auto& q=queries_[frame%8][view];if(q.pending)return;q.frame=frame;q.invalid=false;q.active=true;begin_(elapsed,q.id);}
void Profiler::end_gpu(uint64_t frame,int view){if(!supported_)return;auto& q=queries_[frame%8][view];if(!q.active)return;end_(elapsed);q.active=false;q.pending=true;}
void Profiler::cpu(uint64_t f,double i,double e,double p,double u,double s,double t,double ts,double b,double bs){if(enabled())cpu_<<f<<','<<i<<','<<e<<','<<p<<','<<u<<','<<s<<','<<t<<','<<ts<<','<<b<<','<<bs<<'\n';}
Profiler::~Profiler(){if(supported_){poll();for(auto& frame:queries_)for(auto& q:frame)del_(1,&q.id);}}
}
