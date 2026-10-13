#include "audio.hpp"
#include "audio_dsp.hpp"
#include <SDL.h>
#include <atomic>
#include <cstring>
#include <limits>

namespace fl {
using namespace audio_detail;
namespace {
constexpr size_t kinds=static_cast<size_t>(AudioVoice::Count);
constexpr int progression[3][4][5]={
 {{46,61,65,68,72},{39,55,61,65,72},{44,60,63,67,70},{37,60,65,68,75}},
 {{46,61,65,68,72},{36,52,58,63,67},{41,56,60,63,67},{41,55,60,63,68}},
 {{37,60,65,68,75},{39,55,61,65,70},{41,56,60,63,67},{44,60,63,67,72}}};
constexpr const char* chops[]={"x..x..x...x.x...","x.x...x.x.x...x.","..x..x....x..xx.","x...x.x...x.x.x.","xx..x...xx..x..."};
constexpr double formants[5][3]={{800,1150,2900},{450,800,2830},{400,2000,2550},{325,700,2530},{300,2300,3000}}; // a,o,e,u,i
constexpr int vowels[4][3]={{0,1,-1},{2,0,1},{3,0,-1},{1,4,0}};
struct SectionFlags { bool drums,hats,keys,bass,chops,pad,glitch,pump; };
constexpr SectionFlags flags[]={
 {0,1,1,0,0,1,0,0},{1,1,1,1,0,0,0,0},{1,1,1,1,1,1,0,1},
 {0,0,1,0,1,1,0,0},{1,1,1,1,0,0,1,0}};
double mtof(int m) { return 440*std::pow(2.0,(m-69)/12.0); }
uint64_t bits(double x) { uint64_t b; std::memcpy(&b,&x,8); return b; }
double number(uint64_t b) { double x; std::memcpy(&x,&b,8); return x; }
}
struct AudioEngine::Impl {
 struct Command { uint8_t kind=0; double a=0,b=0; uint8_t flags=0; };
 std::array<Command,command_capacity> queue{};
 alignas(64) std::atomic<uint32_t> write{0};
 alignas(64) std::atomic<uint32_t> read{0};
 std::atomic<uint64_t> dropped{0};
 std::atomic<uint32_t> high_water{0};
 std::atomic<bool> paused{false};
 std::array<std::atomic<uint64_t>,32+kinds> published{};
 SDL_AudioDeviceID device=0;
 bool owns_subsystem=false,prepared=false;
 int rate=48000;
 Random rng{},noise_rng{};
 Sines sine;
 AudioDiagnostics d{};
 uint64_t clock=0,step=0,section_index=0;
 double next_time=0.1;
 AudioSection section=AudioSection::Intro;
 int prog=0,chop_pattern=0,vowel_set=0,chop_offset=0;
 bool air=false,grot=false,grind=false,muted=false;
 double drum=1,cutoff=9000,carve=0,wind=0,grind_gain=0,out_gain=0.85;
 double carve_target=0,wind_target=0,pump_gain=1,pump_delta=0;
 uint64_t pump_end=0;
 int64_t bail_start=-1;
 double lfo_phase=0;
 std::vector<float> noise,vinyl,delay;
 size_t noise_pos=0,vinyl_pos=0,delay_pos=0;
 size_t noise_offset[3]{};
 Biquad music_filter[2],delay_filter,carve_filter,wind_filter,grind_filter,vinyl_filter;
 Reverb reverb;
 Saturation2x saturation[2];
 double comp_envelope=0;
 std::array<AudioTrace,4096> trace{};
 size_t trace_begin=0,trace_count=0;
 struct Voice {
  bool active=false,crushed=false,open=false;
  AudioVoice kind=AudioVoice::Kick;
  uint64_t begin=0,age=0,length=0;
  int note=0,vowel=0;
  double velocity=1,duration=0,phase[3]{},frequency=0,pan=0,extra=0;
  double pitch=0,pitch_mul=1;
  size_t noise_begin=0;
  Envelope amp,aux;
  Biquad filter[3];
 };
 std::array<Voice,voice_capacity> voices{};
 struct Pump { bool active=false; uint64_t sample=0; double depth=0; };
 std::array<Pump,64> pumps{};
 void send(Command command) {
  uint32_t w=write.load(std::memory_order_relaxed),r=read.load(std::memory_order_acquire);
  if(w-r==command_capacity) { dropped.fetch_add(1,std::memory_order_relaxed); return; }
  queue[w%command_capacity]=command; write.store(w+1,std::memory_order_release);
  uint32_t depth=w-r+1,old=high_water.load(std::memory_order_relaxed);
  if(depth>old) high_water.store(depth,std::memory_order_relaxed);
 }
 void publish() {
  d.frames=clock; d.steps=step; d.sections=section_index; d.section=section; d.sample_rate=rate;
  d.sustained_air=air; d.grotesk=grot; d.muted=muted;
  d.drums_gain=drum; d.music_cutoff=cutoff; d.carve_gain=carve; d.wind_gain=wind; d.grind_gain=grind_gain;
  d.active_voices=0; for(const auto& v:voices) if(v.active) ++d.active_voices;
  d.peak_voices=std::max(d.peak_voices,d.active_voices);
  const uint64_t values[]={d.frames,d.steps,d.sections,d.stolen_voices,d.dropped_notes,d.nonfinite_samples,d.peak_voices,d.active_voices,uint64_t(rate),uint64_t(section),uint64_t(air),uint64_t(grot),uint64_t(muted),bits(drum),bits(cutoff),bits(carve),bits(wind),bits(grind_gain)};
  for(size_t i=0;i<18;++i) published[i].store(values[i],std::memory_order_relaxed);
  for(size_t i=0;i<kinds;++i) published[32+i].store(d.voices_started[i],std::memory_order_relaxed);
 }
 bool prepare(int sample_rate,uint32_t seed) {
  if(sample_rate<8000 || sample_rate>192000 || device) return false;
  rate=sample_rate; rng.state=seed?seed:1; noise_rng.state=(seed?seed:1)^0xa361b729U;
  if(!noise_rng.state) noise_rng.state=7;
  d={}; clock=step=section_index=0; next_time=0.1; section=AudioSection::Intro;
  prog=chop_pattern=vowel_set=chop_offset=0; air=grot=grind=muted=false;
  drum=1; cutoff=9000; carve=wind=grind_gain=carve_target=wind_target=0; out_gain=0.85;
  pump_gain=1; pump_delta=0; pump_end=0; bail_start=-1; lfo_phase=0; comp_envelope=0;
  voices={}; pumps={}; noise_pos=vinyl_pos=delay_pos=0; trace_begin=trace_count=0;
  read.store(0); write.store(0); dropped.store(0); high_water.store(0); paused.store(false);
  noise.resize(rate*2); for(auto& x:noise) x=float(noise_rng.next()*2-1);
  vinyl.assign(rate*6,0);
  for(size_t i=0;i<vinyl.size();++i) {
   if(noise_rng.next()<0.0009) { double a=(noise_rng.next()*2-1)*(noise_rng.next()<0.1?1:0.4); for(size_t k=0;k<18 && i+k<vinyl.size();++k) vinyl[i+k]+=float(a*std::exp(-double(k)/3)); }
   vinyl[i]+=float((noise_rng.next()*2-1)*0.012);
  }
  for(auto& offset:noise_offset) offset=size_t(noise_rng.next()*noise.size());
  delay.assign(size_t(sixteenth*3*rate+0.5),0);
  music_filter[0]={}; music_filter[1]={}; delay_filter={}; carve_filter={}; wind_filter={}; grind_filter={}; vinyl_filter={};
  for(auto& filter:music_filter) filter.set(FilterType::Low,9000,0.7,rate);
  delay_filter.set(FilterType::Low,2800,1,rate);
  carve_filter.set(FilterType::Band,650,0.6,rate); wind_filter.set(FilterType::Low,800,0.5,rate);
  grind_filter.set(FilterType::Band,3300,6,rate); vinyl_filter.set(FilterType::Band,3200,0.5,rate);
  reverb.init(rate,noise_rng);for(auto& s:saturation)s.init();prepared=true;publish();return true;
 }
 double now() const { return double(clock)/rate; }
 void set_air(bool on) { if(air==on) return; air=on; bail_start=-1; if(!on) { pump_gain=1; pump_delta=0; pump_end=0; for(auto& p:pumps) p.active=false; } }
 void add_pump(double t,double depth) {
  for(auto& p:pumps) if(!p.active) { p={true,uint64_t(std::max(0.0,t)*rate+0.5),depth}; return; }
  ++d.dropped_notes;
 }
 Voice& add(AudioVoice kind,double t,int note,double dur,double velocity,int vowel=0,bool special=false) {
  Voice* v=nullptr;
  for(auto& candidate:voices) if(!candidate.active) { v=&candidate; break; }
  if(!v) { v=&voices[0]; for(auto& candidate:voices) if(candidate.begin+candidate.length<v->begin+v->length) v=&candidate; ++d.stolen_voices; }
  *v={}; v->active=true; v->kind=kind; v->begin=uint64_t(std::max(now(),t)*rate+0.5); v->note=note; v->duration=dur; v->velocity=velocity; v->vowel=vowel; v->crushed=special; v->frequency=mtof(note); v->noise_begin=size_t(rng.next()*1.5*rate);
  auto length=[&](double seconds) { v->length=uint64_t(seconds*rate+0.5); };
  auto filter=[&](int slot,FilterType type,double f,double q=1) { v->filter[slot].set(type,f,q,rate); };
  switch(kind) {
  case AudioVoice::Kick:
   v->amp.simple(0.003,velocity,0.42,rate); length(0.5); v->pitch=130; v->pitch_mul=std::pow(44.0/130,1/(0.1*rate)); break;
  case AudioVoice::KickClick:
   v->amp.simple(0.001,velocity*0.25,0.015,rate); filter(0,FilterType::High,1500); length(0.07); break;
  case AudioVoice::Snare:
   v->amp.simple(0.002,velocity*0.75,0.19,rate); filter(0,FilterType::Band,1900,0.7); length(0.30); break;
  case AudioVoice::SnareTone:
   v->amp.simple(0.002,velocity*0.35,0.1,rate); v->pitch=special?200+rng.next()*400:200; v->pitch_mul=std::pow(150/v->pitch,1/(0.08*rate)); length(0.2); break;
  case AudioVoice::Hat:
   v->open=special; v->amp.simple(0.001,velocity*0.32,special?0.26:0.035,rate); filter(0,FilterType::High,7200); length(special?0.35:0.11); break;
  case AudioVoice::Rhodes:
   v->amp.reset(); v->amp.point(0,0.0001,false,rate); v->amp.point(0.006,0.15*velocity,true,rate); v->amp.point(std::min(0.9,dur*0.8),0.055*velocity,true,rate); v->amp.point(dur,0.055*velocity,false,rate); v->amp.point(dur+0.45,0.0001,true,rate);
   v->aux.simple(0.002,0.025*velocity,0.22,rate); v->pan=clamp((note-64)/24.0,-0.6,0.6); v->pitch=v->frequency*2.2*velocity; v->pitch_mul=std::pow(0.1/velocity,1/(0.5*rate)); length(dur+0.5); break;
  case AudioVoice::Bass:
   v->amp.reset(); v->amp.point(0,0.0001,false,rate); v->amp.point(0.012,0.5*velocity,true,rate); v->amp.point(dur,0.5*velocity,false,rate); v->amp.point(dur+0.09,0.0001,true,rate); filter(0,FilterType::Low,340); length(dur+0.12); break;
  case AudioVoice::Pad:
   v->amp.reset(); v->amp.point(0,0.0001,false,rate); v->amp.point(0.9,0.028,true,rate); v->amp.point(dur,0.028,false,rate); v->amp.point(dur+1.4,0.0001,true,rate); filter(0,FilterType::Low,750,0.9); length(dur+1.5); break;
  case AudioVoice::Chop:
   v->amp.reset(); v->amp.point(0,0.0001,false,rate); v->amp.point(0.005,0.9*velocity,true,rate); v->amp.point(dur,0.9*velocity,false,rate); v->amp.point(dur+0.04,0.0001,true,rate);
   for(int k=0;k<3;++k) filter(k,FilterType::Band,formants[vowel][k],k==0?8:(k==1?11:14));
   length(dur+0.08); break;
  case AudioVoice::Blip:
   v->frequency=800+rng.next()*3200; v->amp.simple(0.001,0.12,0.02+rng.next()*0.03,rate); length(0.08); break;
  case AudioVoice::Acid:
   v->extra=velocity; v->amp.simple(0.004,0.5,dur+0.05,rate); filter(0,FilterType::Low,220,15); length(dur+0.1); break;
  case AudioVoice::Pop:
   v->amp.simple(0.002,0.4,0.08,rate); filter(0,FilterType::Low,500); length(0.15); break;
  case AudioVoice::Impact:
   v->amp.simple(0.004,0.35+0.6*velocity,0.55,rate); v->pitch=75; v->pitch_mul=std::pow(30.0/75,1/(0.5*rate)); length(0.6); break;
  case AudioVoice::LandNoise:
   v->amp.simple(0.004,0.15*velocity,1.2,rate); filter(0,FilterType::High,4200); length(1.45); break;
  case AudioVoice::UberNoise:
   v->amp.simple(0.55,0.3,0.15,rate); filter(0,FilterType::Band,400,2); length(0.75); break;
  case AudioVoice::BailNoise:
   v->amp.simple(0.01,0.5,0.4,rate); filter(0,FilterType::Band,2600,3); length(0.5); break;
  case AudioVoice::Count: break;
  }
  ++d.voices_started[size_t(kind)];
  uint32_t active=0; for(const auto& candidate:voices) if(candidate.active) ++active;
  d.peak_voices=std::max(d.peak_voices,active);
  if(trace_count==trace.size()) { trace_begin=(trace_begin+1)%trace.size(); --trace_count; }
  trace[(trace_begin+trace_count++)%trace.size()]={v->begin,step,kind,section,note,velocity};
  return *v;
 }
 void kick(double t,double v) { add(AudioVoice::Kick,t,0,0,v); add(AudioVoice::KickClick,t,0,0,v); if(!air) add_pump(t,flags[size_t(section)].pump?0.7:0.45); }
 void snare(double t,double v,bool crushed=false) { add(AudioVoice::Snare,t,0,0,v,0,crushed); add(AudioVoice::SnareTone,t,0,0,v,0,crushed); }
 void hat(double t,double v,bool open=false) { add(AudioVoice::Hat,t,0,0,v,0,open); }
 void rhodes(double t,int m,double dur,double v) { add(AudioVoice::Rhodes,t,m,dur,v); }
 void bass(double t,int m,double dur,double v) { add(AudioVoice::Bass,t,m,dur,v); }
 void chop(double t,int m,double dur,int vw,double v) { add(AudioVoice::Chop,t,m,dur,v,vw); }
 const int* cur_chord() const { return progression[prog][(step/16)%4]; }
 void land(double big,bool trick) {
  set_air(false); double t=now()+0.005;
  if(big>0.05 || trick) { add(AudioVoice::Impact,t,0,0,big); if(big>0.3) add(AudioVoice::LandNoise,t,0,0,big); }
  if(trick) { const int* c=cur_chord(); for(int k=0;k<3;++k) chop(t+k*sixteenth,c[1+(k+1)%4]+12,sixteenth*0.8,k==1?1:0,0.8); }
 }
 void commands() {
  uint32_t r=read.load(std::memory_order_relaxed),w=write.load(std::memory_order_acquire);
  // Snapshot the write cursor: producer activity cannot make callback work unbounded.
  while(r!=w) {
   Command c=queue[r++%command_capacity];
   if(c.kind==0) { carve_target=std::round(c.a*30)/30*0.14; wind_target=std::round(c.b*30)/30*0.12; set_air(c.flags&1); grind=c.flags&2; /* GROTESK is source event-latched, never snapshot-owned. */ }
   else if(c.kind==1) {
    switch(static_cast<GameEventKind>(c.flags)) {
    case GameEventKind::Pop: add(AudioVoice::Pop,now(),0,0,1); break;
    case GameEventKind::Land: land(c.a,c.b!=0); break;
    case GameEventKind::Bail: set_air(false); bail_start=int64_t(clock); cutoff=9000; add(AudioVoice::BailNoise,now(),0,0,1); break;
    case GameEventKind::Uber: { add(AudioVoice::UberNoise,now(),0,0,1); const int* chord=cur_chord(); chop(now()+0.1,chord[4]+12,0.5,0,0.9); break; }
    case GameEventKind::GroteskOn: if(!grot){grot=true;land(1,true);} break;
    case GameEventKind::GroteskOff: grot=false; break;
    case GameEventKind::GrindOn: grind=true; break;
    case GameEventKind::GrindOff: grind=false; break;
    // Music has no combo, recovery or restart jingle/song reset.
    case GameEventKind::ComboAdded: case GameEventKind::ComboBanked: case GameEventKind::Recovery: case GameEventKind::Restart: break;
    }
   } else if(c.kind==2) muted=c.flags!=0;
  }
  read.store(r,std::memory_order_release);
 }
 void new_section() {
  AudioSection name;
  if(section_index<3) name=static_cast<AudioSection>(section_index);
  else {
   constexpr AudioSection choices[]={AudioSection::Verse,AudioSection::Hook,AudioSection::Hook,AudioSection::Break,AudioSection::Glitch,AudioSection::Verse,AudioSection::Glitch};
   // Same conditional distribution as source rejection sampling, bounded work.
   int count=0; for(auto choice:choices) if(choice!=section) ++count;
   int selected=rng.pick(count); name=AudioSection::Verse;
   for(auto choice:choices) if(choice!=section && selected--==0) { name=choice; break; }
  }
  section=name; ++section_index; vowel_set=rng.pick(4); prog=rng.pick(3); chop_pattern=rng.pick(5); chop_offset=rng.pick(3);
 }
 void schedule(uint64_t i,double t) {
  int pos=int(i%16),bar=int(i/16),sbar=bar%8;
  if(pos==0 && bar%8==0) new_section();
  const auto& s=flags[size_t(section)]; const int* ch=progression[prog][sbar%4]; const int* nx=progression[prog][(sbar+1)%4];
  double tt=t+(pos%2?sixteenth*0.16:0)+(rng.next()-0.5)*0.006; bool last=sbar==7;
  if(s.drums) {
   if(pos==0) kick(tt,1);
   if(pos==10) kick(tt,0.85);
   if(pos==7 && rng.next()<0.5) kick(tt,0.6);
   if(pos==14 && rng.next()<0.25) kick(tt,0.55);
   if(s.glitch && rng.next()<0.12) kick(tt+sixteenth/2,0.5);
   if(pos==4 || pos==12) snare(tt+0.012,0.9);
   if(pos==15 && rng.next()<0.3) snare(tt+0.01,0.22);
   if(pos==7 && rng.next()<0.15) snare(tt,0.18);
  }
  if(s.hats) { if(pos%2==0) hat(tt,pos%4==0?0.55:0.38,pos==14 && rng.next()<0.18); else if(rng.next()<0.35) hat(tt,0.2); if(grot) hat(tt+sixteenth/2,0.22); }
  if((s.glitch || (last && s.drums)) && pos>=12) {
   int n=s.glitch?2+rng.pick(4):(rng.next()<0.5?4:0);
   for(int k=0;k<n;++k) snare(tt+k*sixteenth/n,0.25+0.5*k/n,true);
   if(rng.next()<0.5) add(AudioVoice::Blip,tt+rng.next()*sixteenth,0,0,1);
  }
  if(s.glitch && rng.next()<0.18) { for(int k=0;k<3;++k) hat(tt+k*sixteenth/4,0.25); if(rng.next()<0.4) add(AudioVoice::Blip,tt,0,0,1); }
  if(s.pump && pos%4==0 && !air) add_pump(tt,0.72);
  if(s.keys) {
   if(pos==0) for(int k=0;k<4;++k) rhodes(tt+k*0.011,ch[1+k],sixteenth*11,0.9);
   if(pos==6 && rng.next()<0.55) for(int k=0;k<3;++k) rhodes(tt+k*0.009,ch[2+k],sixteenth*3,0.5);
   if((pos==10 || pos==11) && rng.next()<0.35) rhodes(tt,ch[1+rng.pick(4)]+12,sixteenth*2,0.45);
   if(pos==14 && rng.next()<0.3) for(int k=0;k<2;++k) rhodes(tt+k*0.01,nx[3+k],sixteenth*2,0.45);
  }
  if(s.bass) {
   if(pos==0) bass(tt,ch[0],sixteenth*5.5,1);
   if(pos==7 && rng.next()<0.5) bass(tt,ch[0]+12,sixteenth*1.6,0.7);
   if(pos==10 && rng.next()<0.7) bass(tt,ch[0]+(rng.next()<0.3?7:0),sixteenth*2.6,0.85);
   if(pos==15 && rng.next()<0.5) bass(tt,nx[0]+(rng.next()<0.5?-1:2),sixteenth*0.9,0.7);
  }
  if(s.pad && pos==0 && sbar%2==0) for(int k=1;k<=4;++k) add(AudioVoice::Pad,tt,ch[k],sixteenth*30,1);
  if(s.chops && chops[chop_pattern][pos]=='x') { int k=(pos+chop_offset)%4,nv=vowels[vowel_set][2]<0?2:3; chop(tt,ch[1+k]+12,sixteenth*(rng.next()<0.3?1.6:0.8),vowels[vowel_set][(pos>>1)%nv],0.8); }
  if(grot && rng.next()<0.65) { constexpr int scale[]={0,3,7,10,12,15,19}; int note=ch[0]+12+scale[rng.pick(7)]; double accent=rng.next()<0.3?1:0.3; add(AudioVoice::Acid,tt,note,sixteenth*0.8,accent); }
 }
 struct Buses { double drums=0,glitch=0,bass=0,keys[2]{},chop=0,acid=0,verb[2]{},delay=0,sfx=0; };
 void voice_sample(Voice& v,Buses& bus,double lfo) {
  if(!v.active || clock<v.begin) return;
  if(v.age>=v.length) { v.active=false; return; }
  double t=double(v.age)/rate,amp=v.amp.tick(v.age),x=0;
  auto osc=[&](int slot,double f,int type=0) { return oscillator(v.phase[slot],f,type,rate,sine); };
  auto nz=[&]() { return double(noise[(v.noise_begin+v.age)%noise.size()]); };
  auto detune=[&](double cents) { return v.frequency*std::pow(2.0,cents/1200)*lfo; };
  switch(v.kind) {
  case AudioVoice::Kick: x=osc(0,v.pitch)*amp; if(t<0.1) v.pitch*=v.pitch_mul; bus.drums+=x; break;
  case AudioVoice::KickClick: bus.drums+=v.filter[0].process(nz())*amp; break;
  case AudioVoice::Snare:
   x=v.filter[0].process(nz())*amp; if(v.crushed) bus.glitch+=x; else { bus.drums+=x; for(auto& send:bus.verb) send+=x*0.25*v.velocity; } break;
  case AudioVoice::SnareTone:
   x=osc(0,v.pitch,1)*amp; if(t<0.08) v.pitch*=v.pitch_mul; if(v.crushed) bus.glitch+=x; else bus.drums+=x; break;
  case AudioVoice::Hat: bus.drums+=v.filter[0].process(nz())*amp; break;
  case AudioVoice::Rhodes: {
   double mod=osc(2,v.frequency)*v.pitch; if(t<0.5) v.pitch*=v.pitch_mul;
   // WebAudio applies detune to the full frequency AudioParam, including FM.
   double fm=1+mod/v.frequency;
   x=osc(0,detune(-6)*fm)+osc(1,detune(6)*fm);
   // Source tine connects through amp (multiplicative amp envelope).
   x+=sine.at(t*v.frequency*4.02)*(t<0.3?v.aux.tick(v.age):0); x*=amp;
   double a=(v.pan+1)*pi/4, left=x*std::cos(a),right=x*std::sin(a);
   bus.keys[0]+=left; bus.keys[1]+=right; bus.verb[0]+=left*0.22; bus.verb[1]+=right*0.22; break;
  }
  case AudioVoice::Bass: bus.bass+=(osc(0,v.frequency)+v.filter[0].process(osc(1,v.frequency,1)*0.25))*amp; break;
  case AudioVoice::Pad:
   x=v.filter[0].process(osc(0,detune(-9),2)+osc(1,detune(9),2))*amp; bus.keys[0]+=x; bus.keys[1]+=x; break;
  case AudioVoice::Chop: {
   double source=osc(0,detune(-9),2)+osc(1,detune(9),2);
   x=(v.filter[0].process(source)+0.5*v.filter[1].process(source)+0.28*v.filter[2].process(source))*amp;
   bus.chop+=x; bus.delay+=x*0.45; for(auto& send:bus.verb) send+=x*0.3; break;
  }
  case AudioVoice::Blip: bus.glitch+=osc(0,v.frequency,3)*amp; break;
  case AudioVoice::Acid:
   if(v.age%16==0) { double f=t<0.02?220*std::pow((900+v.extra*2400)/220,t/0.02):(t<0.2?(900+v.extra*2400)*std::pow(240/(900+v.extra*2400),(t-0.02)/0.18):240); v.filter[0].set(FilterType::Low,f,15,rate); }
   bus.acid+=v.filter[0].process(osc(0,v.frequency,2))*amp; break;
  case AudioVoice::Pop: case AudioVoice::LandNoise: bus.sfx+=v.filter[0].process(nz())*amp; break;
  case AudioVoice::Impact: bus.sfx+=osc(0,v.pitch)*amp; if(t<0.5) v.pitch*=v.pitch_mul; break;
  case AudioVoice::UberNoise:
   if(v.age%16==0) v.filter[0].set(FilterType::Band,400*std::pow(5000.0/400,std::min(t/0.6,1.0)),2,rate);
   bus.sfx+=v.filter[0].process(nz())*amp; break;
  case AudioVoice::BailNoise:
   if(v.age%16==0) v.filter[0].set(FilterType::Band,2600*std::pow(180.0/2600,std::min(t/0.4,1.0)),3,rate);
   bus.sfx+=v.filter[0].process(nz())*amp; break;
  case AudioVoice::Count: break;
  }
  ++v.age;
 }
 void render(float* stereo,size_t frames) {
  if(!prepared || paused.load(std::memory_order_acquire)) { std::fill(stereo,stereo+frames*2,0); return; }
  commands();
  const double carve_s=1-std::exp(-1/(0.05*rate)),wind_s=1-std::exp(-1/(0.2*rate)),grind_s=1-std::exp(-1/(0.03*rate)),out_s=1-std::exp(-1/(0.05*rate));
  const double drum_s=1-std::exp(-1/((air?0.08:0.01)*rate)),cut_s=1-std::exp(-1/((air?0.35:0.02)*rate));
  const double comp_attack=std::exp(-1/(0.006*rate)),comp_release=std::exp(-1/(0.2*rate));
  for(size_t i=0;i<frames;++i) {
   while(next_time<now()+0.14) { schedule(step,next_time); next_time+=sixteenth; ++step; }
   for(auto& p:pumps) if(p.active && clock>=p.sample) { p.active=false; pump_gain=1-p.depth; pump_end=clock+uint64_t(0.26*rate); pump_delta=p.depth/(0.26*rate); }
   if(clock<pump_end) pump_gain=std::min(1.0,pump_gain+pump_delta);
   drum+=((air?0.08:1)-drum)*drum_s;
   if(bail_start>=0) {
    double t=double(int64_t(clock)-bail_start)/rate;
    if(t<0.3) cutoff=9000*std::pow(240.0/9000,t/0.3);
    else if(t<1.4) cutoff=240;
    else { cutoff+=(9000-cutoff)*(1-std::exp(-1/(0.25*rate))); if(t>4) bail_start=-1; }
   } else cutoff+=((air?1150:9000)-cutoff)*cut_s;
   if(clock%16==0) for(auto& f:music_filter) f.set(FilterType::Low,cutoff,0.7,rate);
   carve+=(carve_target-carve)*carve_s; wind+=(wind_target-wind)*wind_s; grind_gain+=((grind?0.09:0)-grind_gain)*grind_s;
   out_gain+=((muted?0:0.85)-out_gain)*out_s;
   Buses b; double lfo=std::pow(2.0,oscillator(lfo_phase,0.43,0,rate,sine)*8/1200);
   for(auto& v:voices) voice_sample(v,b,lfo);
   double delayed=delay[delay_pos]; delay[delay_pos]=float(b.delay+delay_filter.process(delayed)*0.38); delay_pos=(delay_pos+1)%delay.size();
   double vl=0,vr=0; reverb.process(b.verb[0],b.verb[1],vl,vr);
   // Preserve branch gains and sends from Music.init(); sfx/vinyl bypass music master.
   double drums=(b.drums+std::round(clamp(b.glitch*0.8,-1,1)*6)/6*0.6)*drum;
   double common=drums+b.bass*0.85+(b.chop*0.5+delayed*0.35)*pump_gain+std::tanh(b.acid*0.5*3)*0.35;
   double ride=carve_filter.process(noise[(noise_pos+noise_offset[0])%noise.size()])*carve+wind_filter.process(noise[(noise_pos+noise_offset[1])%noise.size()])*wind+grind_filter.process(noise[(noise_pos+noise_offset[2])%noise.size()])*grind_gain;
   noise_pos=(noise_pos+1)%noise.size(); double crackle=vinyl_filter.process(vinyl[vinyl_pos])*0.22; vinyl_pos=(vinyl_pos+1)%vinyl.size();
   double left=music_filter[0].process(common+b.keys[0]*0.55*pump_gain+vl*0.32)*0.7+(b.sfx+ride)*0.9+crackle;
   double right=music_filter[1].process(common+b.keys[1]*0.55*pump_gain+vr*0.32)*0.7+(b.sfx+ride)*0.9+crackle;
   double peak=std::max(std::abs(left),std::abs(right)),coef=peak>comp_envelope?comp_attack:comp_release;
   comp_envelope=peak+(comp_envelope-peak)*coef;
   double db=20*std::log10(std::max(comp_envelope,1e-12)),over=db+16,reduction=0;
   if(over>5) reduction=over*(1/3.5-1); else if(over>-5) reduction=(1/3.5-1)*(over+5)*(over+5)/20;
   double comp_gain=std::pow(10.0,reduction/20);
   // Source final WaveShaper oversample='2x', after compressor and before out.
   // The finite native output guard also bounds FIR ringing under overload.
   left=clamp(saturation[0].process(left*comp_gain),-1,1)*out_gain;
   right=clamp(saturation[1].process(right*comp_gain),-1,1)*out_gain;
   if(!std::isfinite(left) || !std::isfinite(right)) { ++d.nonfinite_samples; left=right=0; }
   stereo[i*2]=float(left); stereo[i*2+1]=float(right); ++clock;
  }
  publish();
 }
 static void callback(void* userdata,Uint8* stream,int bytes) {
  auto* self=static_cast<Impl*>(userdata);
  if(bytes<=0) return;
  std::memset(stream,0,size_t(bytes));
  self->render(reinterpret_cast<float*>(stream),size_t(bytes)/(sizeof(float)*2));
 }
};
static_assert(std::atomic<uint64_t>::is_always_lock_free && std::atomic<uint32_t>::is_always_lock_free && std::atomic<bool>::is_always_lock_free,
 "Audio commands and diagnostics require lock-free atomics");
AudioEngine::AudioEngine():impl_(new Impl) {}
AudioEngine::~AudioEngine() { close(); }
bool AudioEngine::open() {
 auto& a=*impl_; if(a.device) return true;
 bool initialized=(SDL_WasInit(SDL_INIT_AUDIO)&SDL_INIT_AUDIO)!=0;
 if(!initialized && SDL_InitSubSystem(SDL_INIT_AUDIO)!=0) return false;
 a.owns_subsystem=!initialized;
 SDL_AudioSpec desired{},obtained{}; desired.freq=48000; desired.format=AUDIO_F32SYS; desired.channels=2; desired.samples=512; desired.callback=Impl::callback; desired.userdata=&a;
 auto device=SDL_OpenAudioDevice(nullptr,0,&desired,&obtained,SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
 if(!device) { close(); return false; }
 if(obtained.format!=AUDIO_F32SYS || obtained.channels!=2) { SDL_CloseAudioDevice(device); close(); return false; }
 // SDL devices begin paused. All allocation happens before publishing device/unpause.
 if(!a.prepare(obtained.freq,1)) { SDL_CloseAudioDevice(device); close(); return false; }
 a.device=device; SDL_PauseAudioDevice(device,0); return true;
}
void AudioEngine::close() {
 auto& a=*impl_; if(a.device) { SDL_CloseAudioDevice(a.device); a.device=0; }
 if(a.owns_subsystem) { SDL_QuitSubSystem(SDL_INIT_AUDIO); a.owns_subsystem=false; }
}
void AudioEngine::pause(bool on) {
 auto& a=*impl_;
 if(on&&a.device)SDL_PauseAudioDevice(a.device,1); // join active callback first
 a.paused.store(on,std::memory_order_release);
 if(!on&&a.device)SDL_PauseAudioDevice(a.device,0); // publish resume before callback
}
void AudioEngine::set_ride(const RideSound& ride) {
 // Values are source gains, not raw speed/air_time. Simulation owns the >0.35s test.
 double carve=std::isfinite(ride.carve)?clamp(ride.carve,0,3):0;
 double wind=std::isfinite(ride.wind)?clamp(ride.wind,0,1):0;
 impl_->send({0,carve,wind,uint8_t((ride.sustained_air?1:0)|(ride.grinding?2:0))});
}
void AudioEngine::trigger(const GameEvent& event) {
 switch(event.kind) {
 case GameEventKind::Pop: case GameEventKind::Land: case GameEventKind::Bail: case GameEventKind::Uber:
 case GameEventKind::GroteskOn: case GameEventKind::GroteskOff: case GameEventKind::GrindOn: case GameEventKind::GrindOff:
  impl_->send({1,std::isfinite(event.amount)?clamp(event.amount,0,1):0,event.flag?1.0:0.0,uint8_t(event.kind)}); break;
 default: break;
 }
}
void AudioEngine::set_muted(bool on) { impl_->send({2,0,0,uint8_t(on)}); }
bool AudioEngine::prepare_offline(int rate,uint32_t seed) { return impl_->prepare(rate,seed); }
void AudioEngine::render_offline(float* stereo,size_t frames) { if(!impl_->device && stereo) impl_->render(stereo,frames); }
AudioDiagnostics AudioEngine::diagnostics() const {
 const auto& a=*impl_; AudioDiagnostics d;
 auto get=[&](size_t i) { return a.published[i].load(std::memory_order_relaxed); };
 d.frames=get(0); d.steps=get(1); d.sections=get(2); d.stolen_voices=get(3); d.dropped_notes=get(4); d.nonfinite_samples=get(5); d.peak_voices=uint32_t(get(6)); d.active_voices=uint32_t(get(7)); d.sample_rate=int(get(8)); d.section=AudioSection(get(9)); d.sustained_air=get(10); d.grotesk=get(11); d.muted=get(12); d.drums_gain=number(get(13)); d.music_cutoff=number(get(14)); d.carve_gain=number(get(15)); d.wind_gain=number(get(16)); d.grind_gain=number(get(17));
 d.dropped_commands=a.dropped.load(std::memory_order_relaxed); d.queue_high_water=a.high_water.load(std::memory_order_relaxed);
 for(size_t i=0;i<kinds;++i) d.voices_started[i]=get(32+i);
 return d;
}
size_t AudioEngine::take_trace(AudioTrace* into,size_t capacity) {
 auto& a=*impl_; if(a.device || !into) return 0;
 size_t n=std::min(capacity,a.trace_count); for(size_t i=0;i<n;++i) into[i]=a.trace[(a.trace_begin+i)%a.trace.size()]; a.trace_begin=(a.trace_begin+n)%a.trace.size(); a.trace_count-=n; return n;
}
}
