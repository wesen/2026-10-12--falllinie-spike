#include "audio.hpp"
#include <SDL.h>
#include <iostream>
#include <stdexcept>
#include <cstring>
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void wait_clock(fl::AudioEngine& audio,uint64_t after){
 for(int i=0;i<100&&audio.diagnostics().frames<=after;++i)SDL_Delay(5);
 require(audio.diagnostics().frames>after,"dummy callback did not advance");
}
}
int main(){try{
 // This test must never silently select a physical backend.
 require(SDL_getenv("SDL_AUDIODRIVER")&&std::strcmp(SDL_getenv("SDL_AUDIODRIVER"),"dummy")==0,"requires SDL_AUDIODRIVER=dummy");
 require(SDL_Init(0)==0,SDL_GetError());
 {
  fl::AudioEngine audio;require(audio.open(),SDL_GetError());
  require(std::strcmp(SDL_GetCurrentAudioDriver(),"dummy")==0,"not dummy audio backend");
  require(SDL_WasInit(SDL_INIT_VIDEO)==0,"audio test opened video");
  wait_clock(audio,0);audio.pause(true);auto clock=audio.diagnostics().frames;SDL_Delay(30);require(audio.diagnostics().frames==clock,"pause did not join/freeze callback");
  require(audio.open(),"idempotent open");require(audio.diagnostics().frames==clock,"open restarted paused song");
  audio.set_muted(true);audio.pause(false);wait_clock(audio,clock);require(audio.diagnostics().muted,"resume lost mute command");
  audio.close();clock=audio.diagnostics().frames;SDL_Delay(30);require(audio.diagnostics().frames==clock,"callback after close");
  require(SDL_WasInit(SDL_INIT_AUDIO)==0,"owned audio subsystem not released");
  require(audio.open(),"reopen failed");wait_clock(audio,0);audio.close();audio.close();
 }
 // Caller-owned audio subsystem survives engine destruction.
 require(SDL_InitSubSystem(SDL_INIT_AUDIO)==0,SDL_GetError());
 {fl::AudioEngine audio;require(audio.open(),SDL_GetError());wait_clock(audio,0);}
 require(SDL_WasInit(SDL_INIT_AUDIO)!=0,"engine destroyed caller-owned subsystem");SDL_Quit();
 std::cout<<"dummy SDL audio: open/pause/resume/mute/close/reopen/destructor/subsystem ownership passed; no video, not physical qualification\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';SDL_Quit();return 1;}}
