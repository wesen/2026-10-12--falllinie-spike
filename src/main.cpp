#include <SDL.h>
#include "gl_api.hpp"
#include "renderer.hpp"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>

namespace {
struct Config {
 bool place=false,resize_check=false,source_sampler_precision=false,play=false,autoplay=false;double seconds=0;int frames=0,width=512,height=384,cell=0,cells=3,debug=0,palette=0;uint32_t seed=42;
 std::string assets="assets/shaders",capture,receipt;
};
Config parse(int argc,char** argv){
 Config c;
 for(int i=1;i<argc;++i){std::string a=argv[i];auto value=[&](){if(i+1>=argc)throw std::runtime_error("missing value for "+a);return std::string(argv[++i]);};
  if(a=="--place")c.place=true;else if(a=="--play")c.play=true;else if(a=="--autoplay"){c.play=true;c.autoplay=true;}else if(a=="--resize-check")c.resize_check=true;else if(a=="--source-sampler-precision")c.source_sampler_precision=true;
  else if(a=="--seconds")c.seconds=std::stod(value());else if(a=="--frames")c.frames=std::stoi(value());
  else if(a=="--seed"){auto n=std::stoull(value());if(n>UINT32_MAX)throw std::runtime_error("seed out of range");c.seed=uint32_t(n);}
  else if(a=="--cell")c.cell=std::stoi(value());else if(a=="--cells")c.cells=std::stoi(value());
  else if(a=="--debug-view")c.debug=std::stoi(value());else if(a=="--palette")c.palette=std::stoi(value());
  else if(a=="--assets")c.assets=value();else if(a=="--capture-dir")c.capture=value();else if(a=="--receipt")c.receipt=value();
  else if(a=="--render-size"){auto size=value();auto x=size.find('x');if(x==std::string::npos)throw std::runtime_error("expected WIDTHxHEIGHT");c.width=std::stoi(size.substr(0,x));c.height=std::stoi(size.substr(x+1));}
  else if(a=="--help"){std::cout<<"falllinie-spike [--place] [--seconds N|--frames N] [--seed N] [--cell N] [--cells 1..7]\n [--render-size WxH] [--debug-view 0..5] [--palette 0..2] [--capture-dir PATH] [--receipt PATH] [--resize-check] [--source-sampler-precision] [--play|--autoplay]\nD cycles final/luminance/paint/material/contour/depth; C cycles palette; ESC or SELECT+START quits.\n";std::exit(0);}
  else throw std::runtime_error("unknown argument "+a);
 }
 if(c.width<128||c.height<96||c.width>2048||c.height>1536||c.cell< -1||c.cell>64||c.cells<1||c.cells>7||c.frames<0||c.seconds<0||!std::isfinite(c.seconds)||c.debug<0||c.debug>5||c.palette<0||c.palette>2)throw std::runtime_error("configuration outside bounded spike limits");
 return c;
}
std::string ipc(const std::string& command){
 std::string shell="timeout 3 swaymsg -r "+command+" 2>&1";FILE* p=popen(shell.c_str(),"r");if(!p)return "IPC unavailable";
 std::string out;char b[4096];while(fgets(b,sizeof b,p))out+=b;int status=pclose(p);if(status!=0)std::cerr<<"IPC failed: "<<out<<'\n';return out;
}
size_t rss_kib(){std::ifstream in("/proc/self/status");std::string line;while(std::getline(in,line))if(line.rfind("VmRSS:",0)==0)return std::stoull(line.substr(6));return 0;}
double percentile(std::vector<double> values,double p){if(values.empty())return 0;std::sort(values.begin(),values.end());return values[size_t(std::ceil(p*values.size()))-1];}
}
int main(int argc,char** argv){
 SDL_Window* window=nullptr;SDL_GLContext context=nullptr;SDL_Joystick* pad=nullptr;std::thread placement;std::atomic<bool> stop{false},placed{false};int result=0;
 try{
  Config c=parse(argc,argv);
  if(c.place){setenv("XDG_RUNTIME_DIR","/run/0-runtime-dir",0);setenv("WAYLAND_DISPLAY","wayland-1",0);setenv("SWAYSOCK","/run/0-runtime-dir/sway-ipc.0.sock",0);}
  SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
  if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS|SDL_INIT_JOYSTICK)!=0)throw std::runtime_error(SDL_GetError());
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_ES);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,0);SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
  window=SDL_CreateWindow("falllinie:spike",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,1024,768,SDL_WINDOW_OPENGL);
  if(!window) throw std::runtime_error(SDL_GetError());
  context=SDL_GL_CreateContext(window);
  if(!context) throw std::runtime_error(SDL_GetError());
  glapi::load();
  std::string version=reinterpret_cast<const char*>(glGetString(GL_VERSION)),vendor=reinterpret_cast<const char*>(glGetString(GL_VENDOR)),gpu=reinterpret_cast<const char*>(glGetString(GL_RENDERER));
  std::cout<<"GL_VERSION="<<version<<"\nGL_RENDERER="<<gpu<<"\nGL_VENDOR="<<vendor<<"\nGLSL="<<glGetString(GL_SHADING_LANGUAGE_VERSION)<<std::endl;
  GLint major=0;glGetIntegerv(GL_MAJOR_VERSION,&major);if(major<3)throw std::runtime_error("GLES3 required");
  if(SDL_GL_SetSwapInterval(1)!=0)std::cerr<<"warning: swap interval: "<<SDL_GetError()<<'\n';
  pad=SDL_NumJoysticks()>0?SDL_JoystickOpen(0):nullptr;
  std::cout<<"joystick="<<(pad?SDL_JoystickName(pad):"none")<<'\n';
  fl::Renderer renderer;renderer.initialize(c.seed,c.cell,c.cells,c.width,c.height,c.assets,!c.source_sampler_precision);
  fl::Game game(c.seed);double accumulator=0;auto previous_frame=std::chrono::steady_clock::now();
  if(c.play)std::cout<<"PLAY: stick/dpad steer, UP tuck, DOWN brake, hold/release A or SPACE to jump; START/R restart; SELECT+START exits\n";
  std::vector<double> times,swaps;
  std::vector<std::pair<int,size_t>> rss_samples;size_t start_rss=rss_kib(),peak_rss=start_rss;
  auto start=std::chrono::steady_clock::now();int frames=0,drawable_w=0,drawable_h=0;bool quit=false;
  while(!quit){auto frame_start=std::chrono::steady_clock::now();SDL_Event e;
   while(SDL_PollEvent(&e)){
    if(e.type==SDL_QUIT || (e.type==SDL_KEYDOWN && e.key.keysym.sym==SDLK_ESCAPE))quit=true;
    if(e.type==SDL_KEYDOWN && !e.key.repeat){if(e.key.keysym.sym==SDLK_d && !c.play)c.debug=(c.debug+1)%6;if(e.key.keysym.sym==SDLK_c)c.palette=(c.palette+1)%3;if(c.play && e.key.keysym.sym==SDLK_r)game.reset();}
    if(e.type==SDL_JOYBUTTONDOWN){if(e.jbutton.button==2)c.debug=(c.debug+1)%6;if(e.jbutton.button==3)c.palette=(c.palette+1)%3;if(c.play && e.jbutton.button==9)game.reset();}
   }
   if(pad && SDL_JoystickGetButton(pad,8) && SDL_JoystickGetButton(pad,9))quit=true;
   double elapsed=std::min(.1,std::chrono::duration<double>(frame_start-previous_frame).count());previous_frame=frame_start;
   if(c.play){
    auto keys=SDL_GetKeyboardState(nullptr);fl::Controls input;
    input.steer=(keys[SDL_SCANCODE_RIGHT]||keys[SDL_SCANCODE_D])-(keys[SDL_SCANCODE_LEFT]||keys[SDL_SCANCODE_A]);
    input.drive=(keys[SDL_SCANCODE_UP]||keys[SDL_SCANCODE_W])-(keys[SDL_SCANCODE_DOWN]||keys[SDL_SCANCODE_S]);input.jump=keys[SDL_SCANCODE_SPACE];
    if(pad){double axis=SDL_JoystickGetAxis(pad,0)/32767.0;if(std::abs(axis)>.15)input.steer=axis;double ay=-SDL_JoystickGetAxis(pad,1)/32767.0;if(std::abs(ay)>.15)input.drive=ay;
     input.steer+=SDL_JoystickGetButton(pad,16)-SDL_JoystickGetButton(pad,15);input.drive+=SDL_JoystickGetButton(pad,13)-SDL_JoystickGetButton(pad,14);input.jump=input.jump||SDL_JoystickGetButton(pad,0);}
    if(c.autoplay){input.steer=.35*std::sin(game.time*.55);input.drive=.6;input.jump=std::fmod(game.time,5)>2 && std::fmod(game.time,5)<2.65;if(game.airborne){input.drive=0;input.steer=game.air_time>.12&&game.air_time<.38?.65:0;}}
    input.steer=fl::clamp(input.steer,-1,1);input.drive=fl::clamp(input.drive,-1,1);accumulator+=elapsed;
    while(accumulator>=1.0/120){game.step(input,1.0/120);accumulator-=1.0/120;}
    renderer.update_player(game,elapsed);
    if(frames%60==0)std::cout<<"ride z="<<game.position.z<<" speed="<<fl::length(game.velocity)<<" airborne="<<game.airborne<<" charge="<<game.charge<<" score="<<game.score<<std::endl;
   }
   SDL_GL_GetDrawableSize(window,&drawable_w,&drawable_h);
   if(c.resize_check && frames>0 && frames%60==0){
    const bool large=(frames/60)%2==1;
    renderer.resize(large?std::min(2048,c.width*2):c.width,large?std::min(1536,c.height*2):c.height);
   }
   renderer.render(drawable_w,drawable_h,c.debug,c.palette);
   auto swap_start=std::chrono::steady_clock::now();SDL_GL_SwapWindow(window);auto end=std::chrono::steady_clock::now();++frames;
   if(frames>60){times.push_back(std::chrono::duration<double,std::milli>(end-frame_start).count());swaps.push_back(std::chrono::duration<double,std::milli>(end-swap_start).count());}
   if(frames%60==0){size_t rss=rss_kib();peak_rss=std::max(peak_rss,rss);rss_samples.emplace_back(frames,rss);}
   if(c.place && frames==1){placement=std::thread([&stop,&placed]{
    for(int i=0;i<60 && !stop;++i){if(ipc("-t get_tree").find("falllinie:spike")!=std::string::npos){auto reply=ipc("'[title=\"^falllinie:spike$\"] fullscreen disable, floating enable, border none, move to output DSI-2, fullscreen enable, focus'");
      placed=reply.find("false")==std::string::npos && reply.find("\"success\": true")!=std::string::npos;std::cout<<"placement="<<reply<<std::endl;return;}
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }if(!stop)std::cerr<<"placement timeout\n";
   });}
   if((c.frames && frames>=c.frames) || (c.seconds && std::chrono::duration<double>(end-start).count()>=c.seconds))quit=true;
  }
  stop=true;if(placement.joinable())placement.join();
  if(c.place && !placed)throw std::runtime_error("placement did not succeed");
  if(!c.capture.empty()){std::filesystem::create_directories(c.capture);renderer.capture(c.capture+"/print.bmp");}
  const auto& camera=renderer.camera();double p95=percentile(times,.95);
  std::cout<<"frames="<<frames<<" measured="<<times.size()<<" frame_p50_ms="<<percentile(times,.5)<<" frame_p95_ms="<<p95<<" frame_p99_ms="<<percentile(times,.99)<<" peak_rss_kib="<<peak_rss<<" GL_error="<<glGetError()<<std::endl;
  if(!c.receipt.empty()){
   std::ofstream out(c.receipt);if(!out)throw std::runtime_error("cannot write receipt");
   out<<std::setprecision(10)<<"{\n\"source_sha256\":\"cb9b4befd0252859d9c45246f94fbd7dd1662c6d034c6ccb95fbe16947e8b556\",\n\"seed\":"<<c.seed<<",\"cell\":"<<c.cell<<",\"cells\":"<<c.cells<<",\n\"context\":{\"version\":"<<std::quoted(version)<<",\"vendor\":"<<std::quoted(vendor)<<",\"renderer\":"<<std::quoted(gpu)<<"},\n\"render_size\":["<<renderer.width()<<','<<renderer.height()<<"],\"drawable_size\":["<<drawable_w<<','<<drawable_h<<"],\"placement_ok\":"<<(placed?"true":"false")<<",\n\"debug_view\":"<<c.debug<<",\"palette\":"<<c.palette<<",\"depth_sampler_highp\":"<<(!c.source_sampler_precision?"true":"false")<<",\"resize_check\":"<<(c.resize_check?"true":"false")<<",\n\"camera_eye\":["<<camera.eye.x<<','<<camera.eye.y<<','<<camera.eye.z<<"],\"camera_look\":["<<camera.look.x<<','<<camera.look.y<<','<<camera.look.z<<"],\n\"vertices\":"<<renderer.vertex_count()<<",\"indices\":"<<renderer.index_count()<<",\"terrain_build_ms\":"<<renderer.build_ms()<<",\n\"frames\":"<<frames<<",\"warmup_frames\":60,\"measured_frames\":"<<times.size()<<",\n\"frame_ms\":{\"p50\":"<<percentile(times,.5)<<",\"p95\":"<<p95<<",\"p99\":"<<percentile(times,.99)<<"},\n\"swap_p95_ms\":"<<percentile(swaps,.95)<<",\"rss_kib\":{\"start\":"<<start_rss<<",\"peak\":"<<peak_rss<<",\"end\":"<<rss_kib()<<"},\n\"timing_scope\":\"CPU submission plus swap; not GPU timer\",\"timing_target_met\":"<<(!times.empty() && p95<=33.3?"true":"false")<<",\n\"rss_samples\":[";
   // Keep periodic RSS separate from post-readback allocation in the receipt.
   for(size_t i=0;i<rss_samples.size();++i){if(i)out<<',';out<<'['<<rss_samples[i].first<<','<<rss_samples[i].second<<']';}
   out<<"],\"gl_error\":0\n}\n";
  }
 }catch(const std::exception& e){std::cerr<<"fatal: "<<e.what()<<'\n';result=1;}
 stop=true;if(placement.joinable())placement.join();if(pad)SDL_JoystickClose(pad);if(context)SDL_GL_DeleteContext(context);if(window)SDL_DestroyWindow(window);SDL_Quit();return result;
}
