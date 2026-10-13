#include "runtime.hpp"
#include "gameplay_test_support.hpp"
using namespace fl;using namespace gameplay_test;
int main(){try{
 NativeInputMapper mapper;NativeInputSnapshot physical;
 physical.buttons[8]=physical.buttons[9]=true;auto a=mapper.read(physical,false);
 require(a.quit&&!a.pause&&!a.new_run&&!a.start,"quit chord precedes action edges");
 physical={};mapper.read(physical,false);physical.buttons[9]=true;a=mapper.read(physical,false);require(a.pause&&!a.new_run,"START pause edge");require(!mapper.read(physical,false).pause,"held START not repeated");
 physical={};mapper.read(physical,true);physical.buttons[0]=true;require(mapper.read(physical,true).start,"A starts title");
 physical={};physical.buttons[0]=physical.buttons[1]=physical.buttons[2]=true;for(int b=4;b<=7;++b)physical.buttons[b]=true;
 a=mapper.read(physical,false);require(a.ride.jump&&a.ride.boost&&a.ride.grabs==15&&!a.debug&&!a.palette,"measured source pad controls, no X/Y debug");
 physical={};physical.buttons[13]=physical.buttons[16]=true;a=mapper.read(physical,false);require(a.ride.steer==1&&a.ride.drive==1,"measured dpad up/right");
 physical={};for(auto k:{SDL_SCANCODE_W,SDL_SCANCODE_D,SDL_SCANCODE_SPACE,SDL_SCANCODE_RSHIFT,SDL_SCANCODE_J,SDL_SCANCODE_K,SDL_SCANCODE_U,SDL_SCANCODE_I})physical.keys[k]=1;
 a=mapper.read(physical,false);require(a.ride.steer==1&&a.ride.drive==1&&a.ride.jump&&a.ride.boost&&a.ride.grabs==15,"source keyboard mapping");
 physical={};physical.axis_x=.19;physical.axis_y=-.19;a=mapper.read(physical,false);require(a.ride.steer==0&&a.ride.drive==0,"source .2 axis deadzone");
 physical.focus_lost=true;require(mapper.read(physical,false).pause,"focus-loss pause");
 Game g(42);NativeFrameClock frame;
 auto f=frame.advance(g,{},.5);close(f.raw_dt,.05,"raw dt clamp");require(f.ticks==6,"120Hz fixed ticks");close(g.time,.05,"one simulation");
 g.slowmo=.45;double before=g.time;f=frame.advance(g,{},.05);require(f.ticks==2,"slowmo accumulator");close(f.scale,.4,"pre-tick scale");close(g.slowmo,.4,"begin_frame exactly once");close(g.time-before,2./120,"slowmo physics only");
 g.set_paused(true);before=g.time;double acc=frame.accumulator;frame.advance(g,{},.05);close(g.time,before,"pause freezes ticks");close(frame.accumulator,acc,"pause does not accumulate");close(g.slowmo,.4,"pause freezes slowmo");
 g.set_paused(false);frame.accumulator=1;f=frame.advance(g,{},.05);require(f.ticks==12&&frame.accumulator==0,"tick cap discards overflow");
 // combo_tick belongs to step, not the native frame caller.
 g.restart(42);g.slowmo=0;frame.accumulator=0;g.add_to_combo("fixture",100);g.combo.timer=1;frame.advance(g,{},1./120);close(g.combo.timer,1-1./120,"single combo tick");
 // Original newRun keeps edges and feedback transients; placeOnCourse is not resetRider.
 g.step({0,0,true,false,4},0);g.slowmo=.4;g.flash=.12;g.shake=.7;g.hud.banner="kept";auto generation=g.generation();g.restart(43);
 require(g.generation()==generation+1&&g.seed()==43,"restart invalidates generation");close(g.slowmo,.4,"restart retains source slowmo");close(g.flash,.12,"restart retains flash");require(g.hud.banner=="kept","restart retains source HUD");
 g.events.clear();g.step({},0);require(events(g,GameEventKind::Pop)==1,"restart retains held-jump release edge");
 g.restart(42);g.charge=.75;g.crouch=.8;g.grab_amount=.6;g.place(4);close(g.charge,.75,"place retains charge");close(g.crouch,.8,"place retains crouch");close(g.grab_amount,.6,"place retains grab amount");
 Game demo(42);demo.enter_title();TitleAutopilot autopilot(42);require(demo.title(),"native title state");double demo_time=demo.time;bool charged=false,popped=false;
 for(int i=0;i<240;++i){auto input=autopilot.read(demo,.05);charged|=input.jump;frame.advance(demo,input,.05);popped|=events(demo,GameEventKind::Pop)>0;demo.events.clear();}
 require(demo.time>demo_time&&charged&&popped,"source title autopilot runs and charged pops");generation=demo.generation();demo.start();require(demo.playing()&&!demo.paused()&&demo.generation()==generation+1,"start invalidates title caches");
 AudioEngine audio;require(audio.prepare_offline(8000,3),"offline single audio engine");std::array<float,1024> samples{};
 g.events.clear();g.grotesk=30;g.sound.grotesk=true;g.events.push_back({GameEventKind::GroteskOn,0,false,0,{}});
 publish_feedback(audio,g,true);require(g.events.empty(),"events consumed and cleared once");audio.render_offline(samples.data(),512);auto d=audio.diagnostics();
 require(d.grotesk&&d.voices_started[size_t(AudioVoice::Impact)]==1,"snapshot precedes one event landing reaction");
 g.do_bail("latched test");frame.advance(g,{},.01);publish_feedback(audio,g,true);audio.render_offline(samples.data(),512);d=audio.diagnostics();require(d.grotesk,"bail snapshot does not cancel event-latched GROTESK");
 publish_feedback(audio,g,true);audio.render_offline(samples.data(),512);require(audio.diagnostics().voices_started[size_t(AudioVoice::Impact)]==1,"second view/frame cannot duplicate consumed reaction");
 auto music_clock=audio.diagnostics().frames;g.restart(42);publish_feedback(audio,g,true);audio.render_offline(samples.data(),512);require(!audio.diagnostics().grotesk&&audio.diagnostics().frames==music_clock+512,"new run turns latch off without restarting song");
 audio.pause(true);music_clock=audio.diagnostics().frames;audio.render_offline(samples.data(),512);require(audio.diagnostics().frames==music_clock,"pause freezes audio clock");audio.pause(false);
 std::cout<<"integration: measured controls/chord, fixed frame order/cap, source transients/title demo and single audio/event lifecycle passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
