#include "runtime.hpp"
namespace fl {
NativeActions NativeInputMapper::read(const NativeInputSnapshot& s,bool title){
 NativeActions a;
 auto key=[&](SDL_Scancode k){return bool(s.keys[k]);};
 auto edge=[&](SDL_Scancode k){return key(k)&&!previous_.keys[k];};
 auto button_edge=[&](int k){return s.buttons[k]&&!previous_.buttons[k];};
 a.quit=s.quit||key(SDL_SCANCODE_F10)||(s.buttons[8]&&s.buttons[9]);
 const bool new_run_chord=s.buttons[8]&&s.buttons[3];
 const bool chord_edge=new_run_chord&&!(previous_.buttons[8]&&previous_.buttons[3]);
 // Final-snapshot chords precede SELECT. A later chord restores the visibility
 // saved by this SELECT press; immediate standalone press feedback is transient.
 if((a.quit||new_run_chord)&&select_overlay_owned_){a.dev_overlay_restore=true;select_overlay_owned_=false;}
 if(!s.buttons[8])select_overlay_owned_=false;
 if(!a.quit){
  if(edge(SDL_SCANCODE_F2)){a.dev_overlay_toggle=true;select_overlay_owned_=false;}
  else if(button_edge(8)&&!new_run_chord){a.dev_overlay_toggle=a.dev_overlay_select=true;select_overlay_owned_=true;}
  a.start=title&&(edge(SDL_SCANCODE_RETURN)||edge(SDL_SCANCODE_SPACE)||button_edge(0)||button_edge(9));
  a.pause=!title&&(edge(SDL_SCANCODE_ESCAPE)||edge(SDL_SCANCODE_P)||button_edge(9)||s.focus_lost);
  a.new_run=!title&&(edge(SDL_SCANCODE_R)||chord_edge);
  a.mute=edge(SDL_SCANCODE_M);a.palette=edge(SDL_SCANCODE_C);a.debug=edge(SDL_SCANCODE_F3);
  a.ride.steer=(key(SDL_SCANCODE_RIGHT)||key(SDL_SCANCODE_D))-(key(SDL_SCANCODE_LEFT)||key(SDL_SCANCODE_A));
  a.ride.drive=(key(SDL_SCANCODE_UP)||key(SDL_SCANCODE_W))-(key(SDL_SCANCODE_DOWN)||key(SDL_SCANCODE_S));
  a.ride.jump=key(SDL_SCANCODE_SPACE)||s.buttons[0];
  a.ride.boost=key(SDL_SCANCODE_LSHIFT)||key(SDL_SCANCODE_RSHIFT)||key(SDL_SCANCODE_B)||s.buttons[1]||s.buttons[2];
  for(const auto& grab:std::array<std::pair<SDL_Scancode,uint8_t>,4>{{{SDL_SCANCODE_J,1},{SDL_SCANCODE_K,2},{SDL_SCANCODE_U,4},{SDL_SCANCODE_I,8}}})if(key(grab.first))a.ride.grabs|=grab.second;
  for(int k=0;k<4;++k)if(s.buttons[4+k])a.ride.grabs|=uint8_t(1<<k);
  // Source deadzone and additive/clamped keyboard + measured raw pad input.
  if(std::abs(s.axis_x)>=.2)a.ride.steer+=s.axis_x;
  if(std::abs(s.axis_y)>=.2)a.ride.drive-=s.axis_y;
  a.ride.steer+=int(s.buttons[16])-int(s.buttons[15]);
  a.ride.drive+=int(s.buttons[13])-int(s.buttons[14]);
  a.ride.steer=clamp(a.ride.steer,-1,1);a.ride.drive=clamp(a.ride.drive,-1,1);
 }
 previous_=s;return a;
}
Controls TitleAutopilot::read(Game& g,double raw){
 time_+=raw;Controls input;double z=g.position.z+16,tx=Terrain::center(z)+std::sin(time_*.3)*7;
 double err=std::atan2(tx-g.position.x,16)-g.yaw;err=std::atan2(std::sin(err),std::cos(err));
 input.steer=clamp(-err*2.4,-1,1);input.drive=.3;
 if(g.mode==RiderMode::Air){
  if(!plan_){Plan p;p.spin=rng_.next()<.7?(rng_.next()<.5?-1:1):0;p.flip=rng_.next()<.3?(rng_.next()<.5?-1:1):0;constexpr uint8_t grabs[]={1,2,4,8,3,12};p.grab=grabs[int(rng_.next()*6)];plan_=p;}
  double agl=g.position.y-g.terrain.height(g.position.x,g.position.z);
  double t=agl>0?(g.velocity.y+std::sqrt(std::max(0.,g.velocity.y*g.velocity.y+2*13*agl)))/13:0;
  if(g.air_time>.15&&t>.45){input.steer=plan_->spin;input.drive=plan_->flip;if(t>.25)input.grabs=plan_->grab;}
  else {input.steer=0;input.drive=0;}
  if(g.velocity.y>=0&&g.air_time<.2)input.steer=0;
 }else{
  plan_.reset();hop_-=raw;
  if(hop_<.45&&g.mode==RiderMode::Ground)input.jump=true;
  if(hop_<0)hop_=2.5+rng_.next()*3;
 }
 return input;
}
FrameResult NativeFrameClock::advance(Game& g,Controls input,double raw){
 FrameResult f;f.raw_dt=std::isfinite(raw)?clamp(raw,0,.05):0;
 f.scale=g.begin_frame(f.raw_dt);
 if(!g.paused()){
  accumulator+=f.raw_dt*f.scale;
  while(accumulator>=1./120&&f.ticks<12){g.step(input,1./120);accumulator-=1./120;++f.ticks;}
  if(f.ticks>=12)accumulator=0;
 }
 g.real_time_feedback(input,f.raw_dt);return f;
}
void publish_feedback(AudioEngine& audio,Game& g,bool started){
 if(started){audio.set_ride(g.sound);for(const auto& e:g.events)audio.trigger(e);}
 g.events.clear();
}
}
