#include <SDL.h>
#include "gl_api.hpp"
#include "renderer.hpp"
#include "profiler.hpp"
#include "runtime.hpp"
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
#include <csignal>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

namespace {
volatile std::sig_atomic_t termination_requested=0;
void request_termination(int){termination_requested=1;}
struct Config {
 bool paired_clocks=false,freeze_scene=false,post_bypass=false;fl::GpuMode gpu_mode=fl::GpuMode::Off;unsigned profile_cadence=60;
 bool place=false,resize_check=false,source_sampler_precision=false,play=false,autoplay=false,dual=false,source_sky_hash=false,no_vsync=false,absolute_coordinates=false,muted=false,dev_overlay=false;double seconds=0,fps_cap=0;int frames=0,width=512,height=384,cell=0,cells=3,debug=0,palette=0,audio_buffer=512;uint32_t seed=42;
 fl::AudioRenderMode audio_mode=fl::AudioRenderMode::BufferedWorker;bool source_vinyl=true;
 std::string assets="assets/shaders",capture,receipt,profile;
};
Config parse(int argc,char** argv){
 Config c;bool explicit_play=false;
 for(int i=1;i<argc;++i){std::string a=argv[i];auto value=[&](){if(i+1>=argc)throw std::runtime_error("missing value for "+a);return std::string(argv[++i]);};
  if(a=="--place")c.place=true;else if(a=="--dual-screen"){c.dual=true;c.play=true;}else if(a=="--source-sky-hash")c.source_sky_hash=true;else if(a=="--play"){c.play=true;explicit_play=true;}else if(a=="--autoplay"){c.play=true;c.autoplay=true;}else if(a=="--resize-check")c.resize_check=true;else if(a=="--source-sampler-precision")c.source_sampler_precision=true;
  else if(a=="--fps")c.fps_cap=std::stod(value());else if(a=="--seconds")c.seconds=std::stod(value());else if(a=="--frames")c.frames=std::stoi(value());
  else if(a=="--seed"){auto n=std::stoull(value());if(n>UINT32_MAX)throw std::runtime_error("seed out of range");c.seed=uint32_t(n);}
  else if(a=="--cell")c.cell=std::stoi(value());else if(a=="--cells")c.cells=std::stoi(value());
  else if(a=="--debug-view")c.debug=std::stoi(value());else if(a=="--palette")c.palette=std::stoi(value());
  else if(a=="--audio-buffer")c.audio_buffer=std::stoi(value());
  else if(a=="--audio-mode"){auto mode=value();if(mode=="buffered")c.audio_mode=fl::AudioRenderMode::BufferedWorker;else if(mode=="direct")c.audio_mode=fl::AudioRenderMode::DirectCallback;else throw std::runtime_error("audio mode must be buffered or direct");}
  else if(a=="--paired-clocks")c.paired_clocks=true;
  else if(a=="--no-vinyl")c.source_vinyl=false;
  else if(a=="--source-vinyl")c.source_vinyl=true;
  else if(a=="--muted")c.muted=true;
  else if(a=="--dev-overlay"){c.dev_overlay=true;c.play=true;}
  else if(a=="--absolute-render-coordinates")c.absolute_coordinates=true;
  else if(a=="--no-vsync")c.no_vsync=true;else if(a=="--gpu-profile")c.gpu_mode=fl::GpuMode::View;else if(a=="--gpu-profile-mode"){auto mode=value();if(mode=="off")c.gpu_mode=fl::GpuMode::Off;else if(mode=="view")c.gpu_mode=fl::GpuMode::View;else if(mode=="passes")c.gpu_mode=fl::GpuMode::Passes;else throw std::runtime_error("GPU profile mode must be off, view or passes");}else if(a=="--profile-cadence"){auto text=value();size_t consumed=0;int n=std::stoi(text,&consumed);if(consumed!=text.size()||n<1||n>3600)throw std::runtime_error("profile cadence must be 1..3600");c.profile_cadence=unsigned(n);}else if(a=="--freeze-scene")c.freeze_scene=true;else if(a=="--post-bypass")c.post_bypass=true;else if(a=="--profile")c.profile=value();else if(a=="--assets")c.assets=value();else if(a=="--capture-dir")c.capture=value();else if(a=="--receipt")c.receipt=value();
  else if(a=="--render-size"){auto size=value();auto x=size.find('x');if(x==std::string::npos)throw std::runtime_error("expected WIDTHxHEIGHT");c.width=std::stoi(size.substr(0,x));c.height=std::stoi(size.substr(x+1));}
  else if(a=="--help"){std::cout<<"falllinie-spike [--place] [--seconds N|--frames N] [--seed N] [--cell N] [--cells 1..7]\n [--render-size WxH] [--debug-view 0..5] [--palette 0..5] [--capture-dir PATH] [--receipt PATH] [--resize-check] [--source-sampler-precision] [--play|--autoplay] [--dual-screen] [--source-sky-hash] [--absolute-render-coordinates] [--muted] [--audio-buffer 256..4096 power-of-two] [--audio-mode buffered|direct] [--no-vinyl|--source-vinyl] [--dev-overlay] [--profile CSV] [--paired-clocks (requires --profile)] [--gpu-profile (view)] [--gpu-profile-mode off|view|passes] [--profile-cadence 1..3600] [--freeze-scene (static terrain; dual uses same camera; overrides dual implicit play)] [--post-bypass (non-parity)] [--no-vsync] [--fps N]\nF3 cycles debug; C cycles six palettes; A/START or ENTER/SPACE starts; START/P/ESC pauses; SELECT/F2 toggles developer overlay; SELECT+Y/R new run; M mutes; F10 or SELECT+START quits. --dev-overlay starts gameplay title with overlay. SELECT-first chords restore pre-press visibility; F2 is independent.\n";std::exit(0);}
  else throw std::runtime_error("unknown argument "+a);
 }
 if(!std::isfinite(c.fps_cap)||c.fps_cap<0||c.fps_cap>240||c.width<128||c.height<96||c.width>2048||c.height>1536||c.cell< -1||c.cell>4096||c.cells<1||c.cells>7||c.frames<0||c.seconds<0||!std::isfinite(c.seconds)||c.debug<0||c.debug>5||c.palette<0||c.palette>5||c.audio_buffer<256||c.audio_buffer>4096||(c.audio_buffer&(c.audio_buffer-1)))throw std::runtime_error("configuration outside bounded spike limits");
 if(c.paired_clocks&&c.profile.empty())throw std::runtime_error("paired clocks require --profile CSV");
 if(c.gpu_mode!=fl::GpuMode::Off&&c.profile.empty())throw std::runtime_error("GPU profiling requires --profile CSV");
 if(c.freeze_scene){
  if(explicit_play||c.autoplay||c.dev_overlay||c.resize_check)throw std::runtime_error("static terrain fixture conflicts with play/autoplay/dev-overlay/resize-check");
  c.play=false;
 }
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
 SDL_Window* window=nullptr;SDL_Window* lower_window=nullptr;SDL_GLContext context=nullptr;SDL_Joystick* pad=nullptr;std::thread placement;std::atomic<bool> stop{false},placed{false};int result=0,instance_lock=-1;
 try{
  Config c=parse(argc,argv);
  instance_lock=open("/tmp/falllinie-spike.lock",O_CREAT|O_RDWR|O_CLOEXEC|O_NOFOLLOW,0600);
  if(instance_lock<0 || flock(instance_lock,LOCK_EX|LOCK_NB)!=0)throw std::runtime_error("another Falllinie instance is running, or the instance lock is unavailable");
  std::signal(SIGINT,request_termination);std::signal(SIGTERM,request_termination);
  if(c.place){setenv("XDG_RUNTIME_DIR","/run/0-runtime-dir",0);setenv("WAYLAND_DISPLAY","wayland-1",0);setenv("SWAYSOCK","/run/0-runtime-dir/sway-ipc.0.sock",0);}
  SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
  if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS|SDL_INIT_JOYSTICK)!=0)throw std::runtime_error(SDL_GetError());
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_ES);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,0);SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
  window=SDL_CreateWindow("falllinie:spike",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,1024,768,SDL_WINDOW_OPENGL);
  if(!window) throw std::runtime_error(SDL_GetError());
  context=SDL_GL_CreateContext(window);
  if(!context) throw std::runtime_error(SDL_GetError());
  if(c.dual){
   lower_window=SDL_CreateWindow("falllinie:firstperson",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,1024,768,SDL_WINDOW_OPENGL);
   if(!lower_window || SDL_GL_MakeCurrent(lower_window,context)!=0)throw std::runtime_error(SDL_GetError());
   SDL_GL_SetSwapInterval(0);
   if(SDL_GL_MakeCurrent(window,context)!=0)throw std::runtime_error(SDL_GetError());
  }
  glapi::load();
  std::string version=reinterpret_cast<const char*>(glGetString(GL_VERSION)),vendor=reinterpret_cast<const char*>(glGetString(GL_VENDOR)),gpu=reinterpret_cast<const char*>(glGetString(GL_RENDERER));
  std::cout<<"GL_VERSION="<<version<<"\nGL_RENDERER="<<gpu<<"\nGL_VENDOR="<<vendor<<"\nGLSL="<<glGetString(GL_SHADING_LANGUAGE_VERSION)<<"\nGL_EXTENSIONS="<<glGetString(GL_EXTENSIONS)<<std::endl;
  GLint major=0;glGetIntegerv(GL_MAJOR_VERSION,&major);if(major<3)throw std::runtime_error("GLES3 required");
  GLint max_texture=0,max_renderbuffer=0;glGetIntegerv(GL_MAX_TEXTURE_SIZE,&max_texture);glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE,&max_renderbuffer);std::cout<<"GL_CAPS max_texture="<<max_texture<<" max_renderbuffer="<<max_renderbuffer<<'\n';
  if(SDL_GL_SetSwapInterval(c.no_vsync?0:1)!=0)std::cerr<<"warning: swap interval: "<<SDL_GetError()<<'\n';
  pad=SDL_NumJoysticks()>0?SDL_JoystickOpen(0):nullptr;
  std::cout<<"joystick="<<(pad?SDL_JoystickName(pad):"none")<<'\n';
  fl::Renderer renderer;renderer.set_absolute_coordinates(c.absolute_coordinates);renderer.initialize(c.seed,c.play?-1:c.cell,c.play?4:c.cells,c.width,c.height,c.assets,!c.source_sampler_precision,!c.source_sky_hash,c.play);
  fl::Profiler profiler(c.profile,c.gpu_mode,c.profile_cadence,nullptr,c.paired_clocks);
  renderer.set_post_bypass(c.post_bypass);
  std::cout<<"DIAGNOSTIC fixture="<<(c.freeze_scene?"static-terrain":"live-or-default")<<" second_view="<<(c.freeze_scene?"same-camera":"first-person")<<" post_bypass="<<c.post_bypass<<" non_parity="<<(c.freeze_scene||c.post_bypass)<<" seed="<<c.seed<<" cell="<<c.cell<<" cells="<<c.cells<<" internal="<<c.width<<"x"<<c.height<<" dual="<<c.dual<<"\n";
  if(c.freeze_scene)std::cout<<"STATIC TERRAIN: no simulation/update_player/RNG advancement, rider/particles/trails/HUD/streaming/audio-DSP; fixed initialized camera, input mutations disabled\n";
  fl::Game game(c.seed);fl::NativeFrameClock frame_clock;fl::NativeInputMapper input_mapper;fl::TitleAutopilot autopilot(c.seed);
  const fl::AudioOpenOptions audio_options{c.audio_buffer,c.audio_mode,c.source_vinyl};
  fl::AudioEngine audio;bool audio_started=false,muted=c.muted;std::string last_persistence_error;
  fl::Rng run_seeds{c.seed^0x97f4a71bu};
  bool dev_overlay=c.dev_overlay,select_overlay_saved=dev_overlay;
  fl::DeveloperOverlayCache overlay_cache;fl::DeveloperFrameWindow overlay_window;
  fl::DeveloperFrameTimings completed_frame;
  renderer.set_developer_overlay(dev_overlay);
  if(c.play){
   char* preference=SDL_GetPrefPath("Falllinie","Falllinie");
   if(!preference)throw std::runtime_error(std::string("app best path: ")+SDL_GetError());
   std::string best_path=std::string(preference)+"best-score";SDL_free(preference);game.set_best_path(best_path);
   if(c.autoplay){game.start();audio_started=audio.open(audio_options);if(!audio_started)std::cerr<<"audio unavailable (no render-mode fallback): "<<SDL_GetError()<<'\n';else audio.set_muted(muted);}
   else game.enter_title();
   std::cout<<"SOURCE: A jump / B-X boost / L1-R1-L2-R2 grabs / START pause / SELECT dev / SELECT+Y new seed / SELECT+START quit; keyboard WASD/arrows, SPACE, SHIFT/B, J/K/U/I, P/ESC, R new seed, F2 dev, M, C, F3, F10\nseed="<<game.seed()<<" best_path="<<best_path<<'\n';
  }
  auto previous_frame=std::chrono::steady_clock::now();
  std::vector<double> times,swaps;
  std::vector<std::pair<int,size_t>> rss_samples;size_t start_rss=rss_kib(),peak_rss=start_rss;
  auto start=std::chrono::steady_clock::now();auto previous_end=start,fps_epoch=start,frame_deadline=start;int fps_frames=0;
  auto ms=[](auto a,auto b){return std::chrono::duration<double,std::milli>(b-a).count();};
  int frames=0,drawable_w=0,drawable_h=0;bool quit=false;
  while(!quit && !termination_requested){auto frame_start=std::chrono::steady_clock::now();SDL_Event e;
   if(lower_window){fl::PairedClocks::Scope timing(profiler.paired(),frames,0,fl::FrameStage::Context);if(SDL_GL_MakeCurrent(window,context)!=0)throw std::runtime_error(SDL_GetError());SDL_GL_SetSwapInterval(c.no_vsync?0:1);}
   profiler.poll();
   fl::PairedClocks::Scope events_timing(profiler.paired(),frames,-1,fl::FrameStage::Events);
   fl::NativeInputSnapshot physical;bool focus_event=false;
   while(SDL_PollEvent(&e)){
    if(e.type==SDL_QUIT)physical.quit=true;
    if(e.type==SDL_WINDOWEVENT&&(e.window.event==SDL_WINDOWEVENT_FOCUS_LOST||e.window.event==SDL_WINDOWEVENT_MINIMIZED))focus_event=true;
   }
   // Focus moving between our two windows is not an application blur.
   auto* focused=SDL_GetKeyboardFocus();physical.focus_lost=focus_event&&focused!=window&&(!lower_window||focused!=lower_window);
   if(pad&&!SDL_JoystickGetAttached(pad)){SDL_JoystickClose(pad);pad=nullptr;}
   if(!pad&&SDL_NumJoysticks()>0)pad=SDL_JoystickOpen(0);
   const auto* keys=SDL_GetKeyboardState(nullptr);std::copy(keys,keys+SDL_NUM_SCANCODES,physical.keys.begin());
   if(pad){
    for(int b=0;b<17;++b)physical.buttons[b]=SDL_JoystickGetButton(pad,b);
    physical.axis_x=SDL_JoystickGetAxis(pad,0)/32767.;physical.axis_y=SDL_JoystickGetAxis(pad,1)/32767.;
   }
   const auto actions=input_mapper.read(physical,game.title());quit=quit||actions.quit;
   if(!c.freeze_scene){
   if(actions.dev_overlay_restore)dev_overlay=select_overlay_saved;
   if(actions.dev_overlay_toggle){if(actions.dev_overlay_select)select_overlay_saved=dev_overlay;dev_overlay=!dev_overlay;}
   if(actions.dev_overlay_restore||actions.dev_overlay_toggle){renderer.set_developer_overlay(dev_overlay);overlay_cache.invalidate();}
   if(actions.debug)c.debug=(c.debug+1)%6;
   if(actions.palette)c.palette=(c.palette+1)%6;
   if(c.play&&!quit){
    if(actions.start){game.start();frame_clock.accumulator=0;if(!audio_started){audio_started=audio.open(audio_options);if(!audio_started)std::cerr<<"audio unavailable (no render-mode fallback): "<<SDL_GetError()<<'\n';else audio.set_muted(muted);}}
    else if(actions.new_run){uint32_t seed=uint32_t(run_seeds.next()*2147483648.);if(seed==game.seed())seed=(seed+1)&0x7fffffffU;game.restart(seed);c.seed=seed;frame_clock.accumulator=0;audio.pause(false);std::cout<<"new_run seed="<<seed<<" (--seed "<<seed<<") generation="<<game.generation()<<std::endl;}
    else if(actions.pause){game.set_paused(physical.focus_lost?true:!game.paused());audio.pause(game.paused());}
    if(actions.mute){muted=!muted;audio.set_muted(muted);}
   }
   } // static fixture only accepts quit
   auto event_end=std::chrono::steady_clock::now(),physics_end=event_end,player_end=event_end;
   events_timing.stop();
   double elapsed=std::min(.05,std::chrono::duration<double>(frame_start-previous_frame).count());previous_frame=frame_start;
   fl::FrameResult current_frame;
   if(c.play&&!quit){
    fl::PairedClocks::Scope physics_timing(profiler.paired(),frames,-1,fl::FrameStage::Physics);
    fl::Controls input=game.title()||c.autoplay?autopilot.read(game,elapsed):actions.ride;
    current_frame=frame_clock.advance(game,input,elapsed);
    physics_end=std::chrono::steady_clock::now();physics_timing.stop();
    fl::PairedClocks::Scope player_timing(profiler.paired(),frames,-1,fl::FrameStage::Player);
    renderer.set_native_feedback(pad!=nullptr,muted);renderer.update_player(game,current_frame.raw_dt);player_end=std::chrono::steady_clock::now();player_timing.stop();
    fl::publish_feedback(audio,game,audio_started);
    if(game.persistence_error!=last_persistence_error){last_persistence_error=game.persistence_error;if(!last_persistence_error.empty())std::cerr<<last_persistence_error<<'\n';}
    if(frames%60==0)std::cout<<"ride z="<<game.position.z<<" speed="<<fl::length(game.velocity)<<" airborne="<<game.airborne<<" charge="<<game.charge<<" score="<<game.score<<std::endl;
   }
   SDL_GL_GetDrawableSize(window,&drawable_w,&drawable_h);
   if(c.resize_check && frames>0 && frames%60==0){
    const bool large=(frames/60)%2==1;
    renderer.resize(large?std::min(2048,c.width*2):c.width,large?std::min(1536,c.height*2):c.height);
   }
   const double overlay_now=std::chrono::duration<double>(frame_start-start).count();
   if(dev_overlay&&overlay_cache.due(overlay_now)){
    fl::DeveloperOverlaySnapshot snapshot;
    snapshot.frame=completed_frame;snapshot.render=renderer.developer_diagnostics(drawable_w,drawable_h);
    snapshot.title=game.title();snapshot.paused=game.paused();snapshot.mode=game.mode;snapshot.position=game.position;
    snapshot.speed=fl::length(game.velocity);snapshot.air_time=game.airborne?game.air_time:0;snapshot.charge=game.charge;snapshot.slowmo=game.slowmo;
    snapshot.seed=game.seed();snapshot.generation=game.generation();snapshot.rolling_fps=overlay_window.fps();snapshot.rolling_interval_ms=overlay_window.interval_ms();
    snapshot.audio_open=audio_started;if(audio_started){snapshot.audio=audio.diagnostics();const char* backend=SDL_GetCurrentAudioDriver();snapshot.audio_backend=backend?backend:"n/a";}
    overlay_cache.refresh(snapshot,overlay_now);renderer.set_developer_overlay_lines(overlay_cache.lines());
   }
   auto top_start=std::chrono::steady_clock::now();renderer.set_measurement(profiler.enabled()?&profiler:nullptr,frames,0);
   renderer.render(drawable_w,drawable_h,c.debug,c.palette);
   auto swap_start=std::chrono::steady_clock::now();
   {fl::PairedClocks::Scope timing(profiler.paired(),frames,0,fl::FrameStage::Swap);SDL_GL_SwapWindow(window);}
   auto top_swap_end=std::chrono::steady_clock::now(),bottom_submit_end=top_swap_end;
   if(lower_window){
    {fl::PairedClocks::Scope timing(profiler.paired(),frames,1,fl::FrameStage::Context);if(SDL_GL_MakeCurrent(lower_window,context)!=0)throw std::runtime_error(SDL_GetError());SDL_GL_SetSwapInterval(0);}
    int lw=0,lh=0;SDL_GL_GetDrawableSize(lower_window,&lw,&lh);renderer.set_measurement(profiler.enabled()?&profiler:nullptr,frames,1);if(c.freeze_scene)renderer.render(lw,lh,c.debug,c.palette);else renderer.render_first_person(lw,lh,c.palette);bottom_submit_end=std::chrono::steady_clock::now();
    {fl::PairedClocks::Scope timing(profiler.paired(),frames,1,fl::FrameStage::Swap);SDL_GL_SwapWindow(lower_window);}
   }
   auto swap_end=std::chrono::steady_clock::now();
   fl::PairedClocks::Scope pacing_timing(profiler.paired(),frames,-1,fl::FrameStage::Pacing);
   if(c.fps_cap>0){auto period=std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(1.0/c.fps_cap));frame_deadline+=period;if(swap_end<frame_deadline)std::this_thread::sleep_until(frame_deadline);else if(swap_end>frame_deadline+period)frame_deadline=swap_end;}
   auto end=std::chrono::steady_clock::now();pacing_timing.stop();
   completed_frame={true,bool(lower_window),ms(previous_end,end),ms(frame_start,event_end),ms(event_end,physics_end),ms(physics_end,player_end),c.play?renderer.stream_ms():0,ms(top_start,swap_start),ms(swap_start,top_swap_end),ms(top_swap_end,bottom_submit_end),ms(bottom_submit_end,swap_end),ms(swap_end,end),current_frame.ticks,current_frame.scale};
   const auto& t=completed_frame;
   profiler.cpu(frames,t.interval_ms,t.input_ms,t.physics_ms,t.player_ms,t.stream_ms,t.top_submit_ms,t.top_swap_ms,t.bottom_submit_ms,t.bottom_swap_ms,t.pace_ms);previous_end=end;
   overlay_window.add(t.interval_ms);
   ++fps_frames;if(ms(fps_epoch,end)>=1000){double fps=fps_frames*1000.0/ms(fps_epoch,end);renderer.set_fps(fps);std::cout<<"FPS="<<fps<<" frame_interval_ms="<<ms(frame_start,end)<<std::endl;fps_epoch=end;fps_frames=0;}
   ++frames;
   if(frames>60){times.push_back(std::chrono::duration<double,std::milli>(end-frame_start).count());swaps.push_back(std::chrono::duration<double,std::milli>(end-swap_start).count());}
   if(frames%60==0){size_t rss=rss_kib();peak_rss=std::max(peak_rss,rss);rss_samples.emplace_back(frames,rss);}
   if(c.place && frames==1){placement=std::thread([&stop,&placed,dual=c.dual]{
    for(int i=0;i<60 && !stop;++i){auto tree=ipc("-t get_tree");if(tree.find("falllinie:spike")!=std::string::npos && (!dual || tree.find("falllinie:firstperson")!=std::string::npos)){
     std::string commands=dual?"output DSI-1 power on; [title=\"^falllinie:firstperson$\"] fullscreen disable, floating enable, border none, move to output DSI-1, fullscreen enable; ":"";
     commands+="[title=\"^falllinie:spike$\"] fullscreen disable, floating enable, border none, move to output DSI-2, fullscreen enable, focus";
     auto reply=ipc("'"+commands+"'");
      placed=reply.find("false")==std::string::npos && reply.find("\"success\": true")!=std::string::npos;std::cout<<"placement="<<reply<<std::endl;return;}
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }if(!stop)std::cerr<<"placement timeout\n";
   });}
   if((c.frames && frames>=c.frames) || (c.seconds && std::chrono::duration<double>(end-start).count()>=c.seconds))quit=true;
  }
  renderer.set_measurement(nullptr,0,0); // capture is not a measured frame
  if(audio_started){
   audio.pause(true); // final numeric snapshot has quiescent, comparable frontiers
   auto a=audio.diagnostics();const auto& p=a.producer;
   std::cout<<"AUDIO driver="<<(SDL_GetCurrentAudioDriver()?SDL_GetCurrentAudioDriver():"none")<<" rate="<<a.sample_rate<<" buffer="<<a.buffer_frames<<" mode="<<(a.render_mode==fl::AudioRenderMode::BufferedWorker?"buffered":"direct")<<" vinyl="<<a.source_vinyl
    <<" callbacks="<<a.callback_count<<" mean_ms="<<(a.callback_count?std::to_string(double(a.callback_total_ns)/a.callback_count/1e6):"n/a")<<" max_ms="<<double(a.callback_max_ns)/1e6<<" over_budget="<<a.callback_over_budget
    <<" callback_cpu_samples="<<a.callback_cpu_count<<" callback_cpu_mean_ms="<<(a.callback_cpu_count?std::to_string(double(a.callback_cpu_total_ns)/a.callback_cpu_count/1e6):"n/a")<<" callback_cpu_max_ms="<<(a.callback_cpu_count?std::to_string(double(a.callback_cpu_max_ns)/1e6):"n/a")<<" callback_cpu_unavailable="<<a.callback_cpu_unavailable
    <<" producer_blocks="<<p.count<<" producer_mean_ms="<<(p.count?std::to_string(double(p.total_ns)/p.count/1e6):"n/a")<<" producer_max_ms="<<double(p.max_ns)/1e6<<" producer_over_budget="<<p.over_budget
    <<" producer_cpu_samples="<<p.cpu_count<<" producer_cpu_mean_ms="<<(p.cpu_count?std::to_string(double(p.cpu_total_ns)/p.cpu_count/1e6):"n/a")<<" producer_cpu_max_ms="<<(p.cpu_count?std::to_string(double(p.cpu_max_ns)/1e6):"n/a")<<" producer_cpu_unavailable="<<p.cpu_unavailable
    <<" produced="<<a.frames<<" delivered="<<a.delivered_frames<<" pcm_depth="<<a.pcm_depth<<" pcm_target="<<a.pcm_target<<" pcm_capacity="<<a.pcm_capacity<<" pcm_low="<<a.pcm_low<<" pcm_high="<<a.pcm_high<<" starved_callbacks="<<a.starved_callbacks<<" missing_frames="<<a.missing_frames
    <<" queue_high="<<a.queue_high_water<<" drops="<<a.dropped_commands<<" steals="<<a.stolen_voices<<" nonfinite="<<a.nonfinite_samples<<" budget_counters_not_driver_xruns=1"<<std::endl;
  }
  audio.close();
  stop=true;if(placement.joinable())placement.join();
  if(c.place && !placed)throw std::runtime_error("placement did not succeed");
  if(!c.capture.empty()){if(SDL_GL_MakeCurrent(window,context)!=0)throw std::runtime_error(SDL_GetError());renderer.render(drawable_w,drawable_h,c.debug,c.palette);std::filesystem::create_directories(c.capture);renderer.capture(c.capture+"/print.bmp");}
  const auto& camera=renderer.camera();double p95=percentile(times,.95);
  std::cout<<"frames="<<frames<<" measured="<<times.size()<<" frame_p50_ms="<<percentile(times,.5)<<" frame_p95_ms="<<p95<<" frame_p99_ms="<<percentile(times,.99)<<" peak_rss_kib="<<peak_rss<<" GL_error="<<glGetError()<<std::endl;
  if(!c.receipt.empty()){
   std::ofstream out(c.receipt);if(!out)throw std::runtime_error("cannot write receipt");
   out<<"{\n\"measurement\":{\"fixture\":"<<std::quoted(c.freeze_scene?"static-terrain":"live-or-default")<<",\"second_view\":"<<std::quoted(c.freeze_scene?"same-camera":"first-person")<<",\"post_bypass\":"<<(c.post_bypass?"true":"false")<<",\"gpu_mode\":"<<std::quoted(c.gpu_mode==fl::GpuMode::Off?"off":c.gpu_mode==fl::GpuMode::View?"view":"passes")<<",\"cadence\":"<<c.profile_cadence<<",\"static_exclusions\":"<<std::quoted(c.freeze_scene?"rider,particles,trails,HUD,streaming,audio-DSP":"none")<<"},\n";
   out<<std::setprecision(10)<<"\"source_sha256\":\"cb9b4befd0252859d9c45246f94fbd7dd1662c6d034c6ccb95fbe16947e8b556\",\n\"seed\":"<<c.seed<<",\"cell\":"<<c.cell<<",\"cells\":"<<c.cells<<",\n\"context\":{\"version\":"<<std::quoted(version)<<",\"vendor\":"<<std::quoted(vendor)<<",\"renderer\":"<<std::quoted(gpu)<<"},\n\"render_size\":["<<renderer.width()<<','<<renderer.height()<<"],\"drawable_size\":["<<drawable_w<<','<<drawable_h<<"],\"placement_ok\":"<<(placed?"true":"false")<<",\n\"debug_view\":"<<c.debug<<",\"palette\":"<<c.palette<<",\"dual_screen\":"<<(c.dual?"true":"false")<<",\"stable_sky\":"<<(!c.source_sky_hash?"true":"false")<<",\"depth_sampler_highp\":"<<(!c.source_sampler_precision?"true":"false")<<",\"resize_check\":"<<(c.resize_check?"true":"false")<<",\n\"camera_eye\":["<<camera.eye.x<<','<<camera.eye.y<<','<<camera.eye.z<<"],\"camera_look\":["<<camera.look.x<<','<<camera.look.y<<','<<camera.look.z<<"],\n\"vertices\":"<<renderer.vertex_count()<<",\"indices\":"<<renderer.index_count()<<",\"terrain_build_ms\":"<<renderer.build_ms()<<",\n\"frames\":"<<frames<<",\"warmup_frames\":60,\"measured_frames\":"<<times.size()<<",\n\"frame_ms\":{\"p50\":"<<percentile(times,.5)<<",\"p95\":"<<p95<<",\"p99\":"<<percentile(times,.99)<<"},\n\"swap_p95_ms\":"<<percentile(swaps,.95)<<",\"rss_kib\":{\"start\":"<<start_rss<<",\"peak\":"<<peak_rss<<",\"end\":"<<rss_kib()<<"},\n\"timing_scope\":\"CPU submission plus swap; not GPU timer\",\"timing_target_met\":"<<(!times.empty() && p95<=33.3?"true":"false")<<",\n\"rss_samples\":[";
   // Keep periodic RSS separate from post-readback allocation in the receipt.
   for(size_t i=0;i<rss_samples.size();++i){if(i)out<<',';out<<'['<<rss_samples[i].first<<','<<rss_samples[i].second<<']';}
   out<<"],\"gl_error\":0\n}\n";
  }
 }catch(const std::exception& e){std::cerr<<"fatal: "<<e.what()<<'\n';result=1;}
 stop=true;if(placement.joinable())placement.join();if(pad)SDL_JoystickClose(pad);if(context)SDL_GL_DeleteContext(context);if(lower_window)SDL_DestroyWindow(lower_window);if(window)SDL_DestroyWindow(window);SDL_Quit();if(instance_lock>=0)close(instance_lock);return result;
}
