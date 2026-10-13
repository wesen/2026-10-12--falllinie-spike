#include "audio.hpp"
#include <SDL.h>
#include <array>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <new>
#include <stdexcept>
namespace {
std::atomic<bool> fail_allocation{false};
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void wait_clock(fl::AudioEngine& audio,uint64_t after){
 for(int i=0;i<200&&audio.diagnostics().frames<=after;++i)SDL_Delay(5);
 require(audio.diagnostics().frames>after,"dummy audio owner did not advance");
}
void wait_delivered(fl::AudioEngine& audio,uint64_t after){
 for(int i=0;i<200&&audio.diagnostics().delivered_frames<=after;++i)SDL_Delay(5);
 require(audio.diagnostics().delivered_frames>after,"dummy consumer did not advance");
}
void mode_lifecycle(fl::AudioRenderMode mode,int buffer){
 fl::AudioEngine audio;
 fl::AudioOpenOptions options{buffer,mode,false};
 require(audio.open(options),SDL_GetError());
 require(std::strcmp(SDL_GetCurrentAudioDriver(),"dummy")==0,"not dummy audio backend");
 require(SDL_WasInit(SDL_INIT_VIDEO)==0,"audio test opened video");
 require(!audio.prepare_offline(),"offline preparation while live");
 std::array<float,16> probe{};probe.fill(77);audio.render_offline(probe.data(),8);
 for(auto x:probe)require(x==77,"offline rendered while live");
 wait_delivered(audio,0);audio.pause(true);auto paused=audio.diagnostics();
 require(paused.render_mode==mode&&!paused.source_vinyl,"configuration not observable");
 if(mode==fl::AudioRenderMode::BufferedWorker){
  require(paused.pcm_target==uint32_t(((buffer+1023)/1024+1)*1024),"request-aware lead");
  require(paused.pcm_depth<=paused.pcm_target&&paused.pcm_target<paused.pcm_capacity,"filled maximum instead of target");
  require(paused.frames==paused.delivered_frames+paused.pcm_depth,"pause skipped unread PCM");
  require(paused.producer.count>0&&paused.callback_count>0,"timing owners missing");
  require(paused.producer.cpu_count+paused.producer.cpu_unavailable==paused.producer.count,"producer CPU availability");
 }
 require(paused.callback_cpu_count+paused.callback_cpu_unavailable==paused.callback_count,"consumer CPU availability");
 SDL_Delay(40);auto frozen=audio.diagnostics();
 require(frozen.frames==paused.frames&&frozen.delivered_frames==paused.delivered_frames&&frozen.pcm_depth==paused.pcm_depth,"pause did not freeze/retain both frontiers");
 require(audio.open(fl::AudioOpenOptions{4096,fl::AudioRenderMode::DirectCallback,true}),"idempotent open");
 require(audio.diagnostics().frames==paused.frames&&audio.diagnostics().render_mode==mode&&!audio.diagnostics().source_vinyl,"open restarted/reconfigured paused song");
 audio.set_muted(true);audio.pause(false);wait_clock(audio,paused.frames);
 require(audio.diagnostics().muted,"resume lost mute command");
 wait_delivered(audio,paused.delivered_frames);audio.pause(true);auto resumed=audio.diagnostics();
 require(resumed.frames>=paused.frames&&resumed.delivered_frames>paused.delivered_frames,"resume reset frontier");
 if(mode==fl::AudioRenderMode::BufferedWorker)require(resumed.frames==resumed.delivered_frames+resumed.pcm_depth,"resume discarded retained PCM");
 audio.close();auto stopped=audio.diagnostics();SDL_Delay(30);
 require(audio.diagnostics().frames==stopped.frames&&audio.diagnostics().delivered_frames==stopped.delivered_frames,"owners after close");
 require(SDL_WasInit(SDL_INIT_AUDIO)==0,"owned audio subsystem not released");
 require(audio.open(options),"reopen failed");audio.pause(true);auto reopened=audio.diagnostics();
 require(reopened.frames<resumed.frames&&reopened.starved_callbacks==0&&reopened.missing_frames==0,"reopen did not reset transport");audio.close();audio.close();
}
}
void* operator new(std::size_t n){if(fail_allocation.exchange(false))throw std::bad_alloc();if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
int main(){try{
 require(SDL_getenv("SDL_AUDIODRIVER")&&std::strcmp(SDL_getenv("SDL_AUDIODRIVER"),"dummy")==0,"requires SDL_AUDIODRIVER=dummy");
 require(SDL_Init(0)==0,SDL_GetError());
 {
  fl::AudioEngine audio;require(!audio.open(123),"invalid audio buffer accepted");
  require(!audio.open(fl::AudioOpenOptions{512,static_cast<fl::AudioRenderMode>(99),true}),"invalid mode accepted");
  fail_allocation=true;require(!audio.open(),"preparing allocation failure accepted");require(!fail_allocation,"failure not exercised");
  require(SDL_WasInit(SDL_INIT_AUDIO)==0,"preparing failure leaked owned SDL");
  require(audio.open(),"retry after preparing failure");audio.close();
 }
 for(int buffer:{256,512,1024,2048,4096})mode_lifecycle(fl::AudioRenderMode::BufferedWorker,buffer);
 mode_lifecycle(fl::AudioRenderMode::DirectCallback,1024);
 // Stop while refill/consumer are active; destructor while paused at target lead.
 for(int i=0;i<6;++i){fl::AudioEngine audio;require(audio.open(i%2?256:4096),"shutdown stress open");if(i%2)audio.pause(true);}
 require(SDL_InitSubSystem(SDL_INIT_AUDIO)==0,SDL_GetError());
 {fl::AudioEngine audio;require(audio.open(),SDL_GetError());wait_clock(audio,0);audio.pause(true);}
 require(SDL_WasInit(SDL_INIT_AUDIO)!=0,"engine destroyed caller-owned subsystem");SDL_Quit();
 std::cout<<"dummy SDL audio: buffered 256..4096/direct; prefill/pause retained PCM/frozen frontiers/mute/close/reopen/destructor/preparing failure/offline exclusion/SDL ownership passed; no physical qualification\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';SDL_Quit();return 1;}}
