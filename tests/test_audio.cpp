#include "audio.hpp"
#include "audio_dsp.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <thread>
#include <vector>

namespace {
std::atomic<bool> watch_allocations{false};
std::atomic<size_t> allocations{0};
void require(bool ok,const char* message) { if(!ok) { std::cerr<<"FAIL: "<<message<<'\n'; std::exit(1); } }
size_t ix(fl::AudioVoice k) { return static_cast<size_t>(k); }
fl::GameEvent event(fl::GameEventKind kind,double amount=0,bool flag=false) { return {kind,amount,flag,0,{}}; }
struct Harness {
 fl::AudioEngine audio;
 std::array<float,1024> buffer{};
 double peak=0,energy=0;
 uint64_t samples=0;
 explicit Harness(int rate=8000,uint32_t seed=19) { require(audio.prepare_offline(rate,seed),"offline prepare"); }
 void frames(size_t count) {
  while(count) {
   size_t n=std::min(count,buffer.size()/2);
   watch_allocations.store(true); audio.render_offline(buffer.data(),n); watch_allocations.store(false);
   for(size_t i=0;i<n*2;++i) { require(std::isfinite(buffer[i]),"finite output"); require(std::abs(buffer[i])<=0.850001,"waveshaper/output bounds"); peak=std::max(peak,double(std::abs(buffer[i]))); energy+=buffer[i]*buffer[i]; ++samples; }
   count-=n;
  }
 }
 void seconds(double count) { frames(size_t(count*audio.diagnostics().sample_rate+0.5)); }
};
void test_saturation_2x(){
 using namespace fl::audio_detail;
 Saturation2x effect;effect.init();double dc=0;
 for(int i=0;i<256;++i)dc=effect.process(.3);
 require(std::abs(dc-Saturation2x::shape(.3))<1e-12,"2x original curve, unity DC");
 effect.init();double previous=0;
 for(int i=0;i<256;++i){double y=effect.process(-.3);require(std::isfinite(y),"2x finite startup");previous=y;}
 require(std::abs(previous+dc)<1e-12,"2x odd curve/stereo-independent state");
 // The third harmonic of .21875 Fs folds to .34375 Fs without oversampling.
 effect.init();constexpr int count=8192;constexpr double tone=.21875,alias=.34375;
 double native_c=0,native_s=0,up_c=0,up_s=0;
 watch_allocations.store(true);
 for(int i=0;i<count+256;++i){
  double x=.8*std::sin(2*pi*tone*i),y=effect.process(x);
  if(i<256)continue;
  double c=std::cos(2*pi*alias*i),s=std::sin(2*pi*alias*i),native=Saturation2x::shape(x);
  native_c+=native*c;native_s+=native*s;up_c+=y*c;up_s+=y*s;
 }
 watch_allocations.store(false);
 double attenuation=20*std::log10(std::hypot(up_c,up_s)/std::hypot(native_c,native_s));
 require(attenuation<-25,"2x anti-alias suppresses folded nonlinear third harmonic >25dB");
 effect.init();double impulse_peak=0;int peak_sample=-1;
 for(int i=0;i<128;++i){double y=std::abs(effect.process(i==0?1e-6:0));if(y>impulse_peak){impulse_peak=y;peak_sample=i;}}
 require(peak_sample==24,"2x interpolation/decimation total group delay 24 samples");
 effect.init();Saturation2x fresh;fresh.init();
 for(int i=0;i<512;++i)require(effect.process(std::sin(i))==fresh.process(std::sin(i)),"2x reset/block independent fixed state");
 std::cout<<"final saturation: preallocated 2x, original DC curve, 24-sample latency, folded third harmonic "<<attenuation<<" dB vs 1x\n";
}
void test_fft_and_source_ir() {
 using namespace fl::audio_detail;
 Sines sine; double phase=0;
 oscillator(phase,-100,0,48000,sine);
 require(oscillator(phase,-100,0,48000,sine)<0,"negative instantaneous FM frequency reverses phase");
 FFT fft; fft.init(32); std::array<std::complex<float>,32> a{},original{};
 for(size_t i=0;i<a.size();++i) a[i]=original[i]={float(std::sin(i)),float(std::cos(i))};
 fft.run(a.data()); fft.run(a.data(),true);
 for(size_t i=0;i<a.size();++i) require(std::abs(a[i]-original[i])<1e-5,"FFT roundtrip");
 Random seed; seed.state=7; Reverb verb; verb.init(8000,seed);
 Random reference; reference.state=7; constexpr size_t length=20800;
 std::vector<double> ir[2]; double power=0;
 for(auto& channel:ir) { channel.resize(length); for(size_t i=0;i<length;++i) { channel[i]=(reference.next()*2-1)*std::pow(1-double(i)/length,3.4); power+=channel[i]*channel[i]; } }
 double scale=0.00125/std::sqrt(power/(length*2))*44100/8000;
 double error=0,late=0;
 for(size_t i=0;i<length+Reverb::block*2;++i) {
  double l=0,r=0; verb.process(i==0?1:0,i==0?0.5:0,l,r);
  double want_l=0,want_r=0;
  if(i>=Reverb::block && i<length+Reverb::block) { want_l=ir[0][i-Reverb::block]*scale; want_r=ir[1][i-Reverb::block]*scale*0.5; }
  error=std::max(error,std::max(std::abs(l-want_l),std::abs(r-want_r)));
  if(i>Reverb::block+16000) late+=l*l+r*r;
 }
 require(error<1e-6,"partition convolution retains all 2.6s source-shaped taps with fixed latency");
 require(late>1e-8,"IR tail retained past two seconds");
 // Non-impulse inputs exercise the complex cross terms and Hermitian rebuild.
 seed.state=7;verb.init(8000,seed);std::array<double,127> signal{};
 for(size_t i=0;i<signal.size();++i)signal[i]=std::sin(i*.173)*.3+std::cos(i*.071)*.1;
 double signal_error=0;
 for(size_t i=0;i<5000;++i){double l=0,r=0;verb.process(i<signal.size()?signal[i]:0,i<signal.size()?-signal[i]*.7:0,l,r);double want_l=0,want_r=0;if(i>=Reverb::block){size_t t=i-Reverb::block;for(size_t j=0;j<signal.size()&&j<=t;++j){want_l+=signal[j]*ir[0][t-j]*scale;want_r-=signal[j]*.7*ir[1][t-j]*scale;}}signal_error=std::max(signal_error,std::max(std::abs(l-want_l),std::abs(r-want_r)));}
 require(signal_error<1e-6,"half-spectrum convolution matches direct source IR for non-impulse stereo signal");
 require(verb.impulse_real[0].size()==verb.partitions*Reverb::bins,"only independent real-signal bins stored");
 std::cout<<"convolution signal max error "<<signal_error<<'\n';
 std::cout<<"source IR: 20800 taps/channel, max error "<<error<<", late energy "<<late<<'\n';
}
void test_arrangement() {
 Harness h;
 constexpr double section_seconds=fl::AudioEngine::sixteenth*128;
 std::array<fl::AudioTrace,4096> events{};
 std::array<bool,5> seen{};
 fl::AudioSection previous=fl::AudioSection::Intro;
 uint64_t kicks=0,snares=0;
 for(size_t section=0;section<20;++section) {
  // Scheduler looks 140ms ahead, initial step deadline is 100ms.
  double deadline=0.1+section*section_seconds;
  uint64_t desired=uint64_t((deadline+0.05)*8000);
  uint64_t current=h.audio.diagnostics().frames;
  if(desired>current) h.frames(size_t(desired-current));
  auto d=h.audio.diagnostics();
  require(d.sections==section+1,"eight-bar section boundary");
  if(section<3) require(size_t(d.section)==section,"intro / verse / hook order");
  if(section>=3) require(d.section!=previous && d.section!=fl::AudioSection::Intro,"weighted transitions exclude repeats and intro");
  previous=d.section; seen[size_t(d.section)]=true;
  size_t n=h.audio.take_trace(events.data(),events.size());
  for(size_t j=0;j<n;++j) {
   const auto& e=events[j]; double start=double(e.sample)/8000;
   double ideal=0.1+e.step*fl::AudioEngine::sixteenth;
   if(e.voice==fl::AudioVoice::Kick && (e.velocity==1 || e.velocity==0.85) && (e.step%16==0 || e.step%16==10)) {
    require(std::abs(start-ideal)<=0.0031,"kick sample deadline and jitter without accumulated rounding"); ++kicks;
   }
   if(e.voice==fl::AudioVoice::Snare && (e.step%16==4 || e.step%16==12) && e.velocity==0.9) {
    require(std::abs(start-ideal-0.012)<=0.0031,"backbeat offset/jitter"); ++snares;
   }
   if(e.voice==fl::AudioVoice::Rhodes && e.velocity==0.9) {
    require(e.step%16==0,"bar-start source Rhodes chord pattern");
    require(e.note>=52 && e.note<=75,"source Rhodes progression range");
    if(j+3<n && (j==0 || events[j-1].voice!=fl::AudioVoice::Rhodes || events[j-1].step!=e.step || events[j-1].velocity!=0.9)) {
     constexpr int source_chords[][4]={{61,65,68,72},{55,61,65,72},{60,63,67,70},{60,65,68,75},{52,58,63,67},{56,60,63,67},{55,60,63,68},{55,61,65,70},{60,63,67,72}};
     bool source_chord=false;
     for(const auto& chord:source_chords) {
      bool match=true; for(size_t k=0;k<4;++k) match=match && events[j+k].voice==fl::AudioVoice::Rhodes && events[j+k].step==e.step && events[j+k].note==chord[k];
      source_chord=source_chord || match;
     }
     require(source_chord,"four-note voicing belongs to original PROG tables");
     for(size_t k=0;k<4;++k) require(std::abs(double(events[j+k].sample-e.sample)/8000-k*0.011)<0.00013,"source 11ms chord strum");
    }
   }
   if(e.section==fl::AudioSection::Intro) require(e.voice!=fl::AudioVoice::Kick && e.voice!=fl::AudioVoice::Snare && e.voice!=fl::AudioVoice::Bass && e.voice!=fl::AudioVoice::Chop,"intro flags");
   if(e.voice==fl::AudioVoice::Hat && e.step%16%2 && e.velocity==0.2) require(std::abs(start-ideal-fl::AudioEngine::sixteenth*0.16)<=0.0031,"source offbeat 16 percent swing");
  }
 }
 for(bool s:seen) require(s,"all five source section types reached");
 auto d=h.audio.diagnostics();
 require(kicks>100 && snares>100,"source drum patterns present");
 for(auto kind:{fl::AudioVoice::Kick,fl::AudioVoice::KickClick,fl::AudioVoice::Snare,fl::AudioVoice::SnareTone,fl::AudioVoice::Hat,fl::AudioVoice::Rhodes,fl::AudioVoice::Bass,fl::AudioVoice::Pad,fl::AudioVoice::Chop,fl::AudioVoice::Blip}) require(d.voices_started[ix(kind)]>0,"source instruments scheduled");
 require(d.steps==uint64_t(std::ceil((double(d.frames)/8000+0.14-0.1)/fl::AudioEngine::sixteenth)),"fractional 86 BPM sample clock");
 require(d.stolen_voices==0 && d.dropped_notes==0 && d.nonfinite_samples==0,"arrangement stays within state budgets");
 require(h.energy/h.samples>1e-5,"non-silent soundtrack");
 std::cout<<"arrangement: "<<d.sections<<" sections, "<<d.steps<<" steps, peak voices "<<d.peak_voices<<", output peak "<<h.peak<<'\n';
}
void test_reactions() {
 Harness h(48000);
 fl::RideSound ride; ride.carve=1.11; ride.wind=0.75; ride.grinding=true;
 h.audio.set_ride(ride); h.seconds(1);
 auto d=h.audio.diagnostics();
 require(std::abs(d.carve_gain-(33.0/30)*0.14)<1e-6,"source carve quantization and smoothing");
 require(std::abs(d.wind_gain-(23.0/30)*0.12)<0.001,"source wind quantization and smoothing");
 require(std::abs(d.grind_gain-0.09)<1e-6,"source grind target");
 ride.sustained_air=true; h.audio.set_ride(ride); h.seconds(2);
 d=h.audio.diagnostics(); require(d.sustained_air && std::abs(d.drums_gain-0.08)<1e-6 && std::abs(d.music_cutoff-1150)<30,"sustained-air source attenuation and lowpass");
 h.audio.trigger(event(fl::GameEventKind::Pop));
 h.audio.trigger(event(fl::GameEventKind::Land,0.6,true)); h.seconds(0.5);
 d=h.audio.diagnostics(); require(!d.sustained_air && d.music_cutoff>8990 && d.drums_gain>0.999,"landing restores air buses");
 require(d.voices_started[ix(fl::AudioVoice::Pop)]==1 && d.voices_started[ix(fl::AudioVoice::Impact)]==1 && d.voices_started[ix(fl::AudioVoice::LandNoise)]==1 && d.voices_started[ix(fl::AudioVoice::Chop)]==3,"pop / big trick landing original reactions");
 h.audio.trigger(event(fl::GameEventKind::Uber)); h.audio.trigger(event(fl::GameEventKind::Bail)); h.seconds(0.35);
 d=h.audio.diagnostics(); require(d.music_cutoff<241,"bail exponential sweep to 240Hz");
 require(d.voices_started[ix(fl::AudioVoice::UberNoise)]==1 && d.voices_started[ix(fl::AudioVoice::BailNoise)]==1 && d.voices_started[ix(fl::AudioVoice::Chop)]==4,"uber chord chop / bail noise");
 h.seconds(2.5); require(h.audio.diagnostics().music_cutoff>8950,"bail delayed recovery");
 h.audio.trigger(event(fl::GameEventKind::GroteskOn)); h.seconds(2);
 d=h.audio.diagnostics(); require(d.grotesk && d.voices_started[ix(fl::AudioVoice::Acid)]>0 && d.voices_started[ix(fl::AudioVoice::Impact)]==2 && d.voices_started[ix(fl::AudioVoice::Chop)]==7,"GROTESK source landing plus acid reaction");
 h.audio.set_ride({});h.frames(1);require(h.audio.diagnostics().grotesk,"bail/zero snapshot cannot cancel source GROTESK latch");
 fl::RideSound steady;steady.grotesk=true;h.audio.set_ride(steady);h.audio.trigger(event(fl::GameEventKind::GroteskOn));h.frames(1);
 require(h.audio.diagnostics().voices_started[ix(fl::AudioVoice::Impact)]==2,"steady snapshot plus duplicate event cannot duplicate GROTESK landing");
 h.audio.trigger(event(fl::GameEventKind::GroteskOff)); h.audio.trigger(event(fl::GameEventKind::GrindOff)); h.seconds(0.5);
 d=h.audio.diagnostics(); require(!d.grotesk && d.grind_gain<1e-6,"Grotesk/grind off");
 auto before=d.voices_started;
 for(auto kind:{fl::GameEventKind::ComboAdded,fl::GameEventKind::ComboBanked,fl::GameEventKind::Recovery,fl::GameEventKind::Restart}) h.audio.trigger(event(kind,1,true));
 h.frames(1); require(before==h.audio.diagnostics().voices_started,"no invented combo/recovery/restart jingles");
 uint64_t frame=h.audio.diagnostics().frames,step=h.audio.diagnostics().steps;
 h.audio.pause(true);
 for(int i=0;i<600;++i)h.audio.set_ride({}); // ten seconds at 60FPS, callback suspended
 h.frames(4096);require(h.audio.diagnostics().dropped_commands==0,"unchanged paused snapshots cannot fill suspended queue");
 require(h.audio.diagnostics().frames==frame && h.audio.diagnostics().steps==step,"pause freezes sample/section clock");
 for(float value:h.buffer) require(value==0,"pause outputs zero");
 h.audio.pause(false); h.frames(32); require(h.audio.diagnostics().frames==frame+32,"resume continues rather than restarts");
 h.audio.set_muted(true); h.seconds(1);
 require(h.audio.diagnostics().muted,"mute command");
 for(float value:h.buffer) require(std::abs(value)<1e-6,"mute smooths output while music clock continues");
 require(h.audio.diagnostics().frames>frame+32,"mute does not freeze sample clock");
 std::cout<<"reactivity: air/land/pop/uber/bail/GROTESK/grind/pause/mute/restart passed\n";
}
void test_limits_and_spsc() {
 Harness h;
 for(size_t i=0;i<300;++i) h.audio.trigger(event(fl::GameEventKind::Pop));
 auto d=h.audio.diagnostics(); require(d.dropped_commands==44 && d.queue_high_water==256,"bounded drop-newest queue diagnostics");
 h.frames(512); d=h.audio.diagnostics(); require(d.voices_started[ix(fl::AudioVoice::Pop)]==256 && d.stolen_voices>0 && d.peak_voices==128,"bounded deterministic voice stealing under overload");
 std::atomic<bool> done{false};
 std::thread producer([&] { fl::RideSound ride; for(size_t i=0;i<10000;++i) { ride.carve=(i%30)/30.0; ride.wind=(i%30)/30.0; h.audio.set_ride(ride); } done.store(true,std::memory_order_release); });
 // Do not use the allocation watcher here: thread runtime is not callback code.
 do { h.audio.render_offline(h.buffer.data(),512); } while(!done.load(std::memory_order_acquire));
 producer.join(); h.frames(512); d=h.audio.diagnostics();
 require(d.queue_high_water<=256 && d.nonfinite_samples==0,"SPSC producer/consumer numeric bounded output");
 require(!h.audio.prepare_offline(7999) && !h.audio.prepare_offline(192001),"invalid negotiated rates rejected");
 std::cout<<"limits: queue high water "<<d.queue_high_water<<", drops "<<d.dropped_commands<<", steals "<<d.stolen_voices<<'\n';
}
void test_block_independence() {
 fl::AudioEngine a,b; require(a.prepare_offline(44100,5) && b.prepare_offline(44100,5),"returned-rate preparation");
 std::array<float,10000> x{},y{};
 a.render_offline(x.data(),5000); size_t offset=0;
 while(offset<5000) { size_t n=std::min(size_t(137),5000-offset); b.render_offline(y.data()+offset*2,n); offset+=n; }
 require(x==y,"sample-clock scheduler/DSP independent of callback block size");
 auto d=a.diagnostics(); require(d.sample_rate==44100 && d.frames==5000,"non-default sample rate honored");
}
void write_u32(std::ostream& out,uint32_t value) { char b[4]; for(int i=0;i<4;++i) b[i]=char(value>>(i*8)); out.write(b,4); }
void write_u16(std::ostream& out,uint16_t value) { char b[2]={char(value),char(value>>8)}; out.write(b,2); }
void render_evidence(const char* path,double seconds) {
 require(seconds>=1 && seconds<=600,"bounded offline evidence duration");
 Harness h(48000,19); std::ofstream wav(path,std::ios::binary); require(bool(wav),"open offline evidence path");
 uint32_t frames=uint32_t(seconds*48000),bytes=frames*4;
 wav.write("RIFF",4); write_u32(wav,bytes+36); wav.write("WAVEfmt ",8); write_u32(wav,16); write_u16(wav,1); write_u16(wav,2); write_u32(wav,48000); write_u32(wav,48000*4); write_u16(wav,4); write_u16(wav,16); wav.write("data",4); write_u32(wav,bytes);
 fl::RideSound ride;
 std::vector<double> costs; costs.reserve((frames+511)/512);
 std::array<int16_t,1024> pcm{};
 for(size_t at=0;at<frames;at+=512) {
  double t=double(at)/48000; ride.carve=t>5 && t<12?0.8:0; ride.wind=t>12 && t<18?0.9:0; ride.sustained_air=t>13 && t<18; ride.grinding=t>24 && t<28; ride.grotesk=t>65 && t<80;
  h.audio.set_ride(ride);
  auto crossed=[&](double when) { return t>=when && t<double(when)+512.0/48000; };
  if(crossed(12)) h.audio.trigger(event(fl::GameEventKind::Pop));
  if(crossed(18)) h.audio.trigger(event(fl::GameEventKind::Land,0.7,true));
  if(crossed(30)) h.audio.trigger(event(fl::GameEventKind::Bail));
  if(crossed(50)) h.audio.trigger(event(fl::GameEventKind::Uber));
  if(crossed(65)) h.audio.trigger(event(fl::GameEventKind::GroteskOn));
  if(crossed(80)) h.audio.trigger(event(fl::GameEventKind::GroteskOff));
  size_t n=std::min(size_t(512),size_t(frames)-at);
  auto start=std::chrono::steady_clock::now();
  watch_allocations.store(true); h.audio.render_offline(h.buffer.data(),n); watch_allocations.store(false);
  auto end=std::chrono::steady_clock::now(); costs.push_back(std::chrono::duration<double,std::milli>(end-start).count());
  for(size_t i=0;i<n*2;++i) { require(std::isfinite(h.buffer[i]),"offline evidence finite"); h.peak=std::max(h.peak,double(std::abs(h.buffer[i]))); h.energy+=h.buffer[i]*h.buffer[i]; ++h.samples; pcm[i]=int16_t(h.buffer[i]*32767); write_u16(wav,uint16_t(pcm[i])); }
 }
 require(bool(wav),"offline evidence write complete");
 auto d=h.audio.diagnostics(); std::sort(costs.begin(),costs.end());
 double sum=0; for(double cost:costs) sum+=cost;
 std::cout<<"offline render: "<<seconds<<"s, 48000Hz stereo, peak "<<h.peak<<", RMS "<<std::sqrt(h.energy/h.samples)<<"\n512-frame cost ms: mean "<<sum/costs.size()<<", p95 "<<costs[size_t(costs.size()*0.95)]<<", max "<<costs.back()<<", deadline "<<512.0/48<<" (host only)\n";
 std::cout<<"diagnostics: steps "<<d.steps<<", sections "<<d.sections<<", peak voices "<<d.peak_voices<<", queue high water "<<d.queue_high_water<<", command drops "<<d.dropped_commands<<", note drops "<<d.dropped_notes<<", steals "<<d.stolen_voices<<", nonfinite "<<d.nonfinite_samples<<'\n';
 for(size_t i=0;i<d.voices_started.size();++i) std::cout<<"voice["<<i<<"]="<<d.voices_started[i]<<'\n';
}
}
void* operator new(std::size_t n) { if(watch_allocations.load()) allocations.fetch_add(1); if(void* p=std::malloc(n?n:1)) return p; throw std::bad_alloc(); }
void* operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p,std::size_t) noexcept { std::free(p); }
void operator delete[](void* p,std::size_t) noexcept { std::free(p); }
int main(int argc,char** argv) {
 if(argc>=3 && std::string(argv[1])=="--render") { render_evidence(argv[2],argc>3?std::stod(argv[3]):90); }
 else { for(double x:{-20.,-3.,-2.,-.5,0.,.5,2.,3.,20.})require(std::abs(fl::audio_detail::acid_shape(x)-std::tanh(std::max(-1.,std::min(1.,x*.5))*3))<1e-12,"acid source endpoint clamp"); test_saturation_2x(); test_fft_and_source_ir(); test_arrangement(); test_reactions(); test_limits_and_spsc(); test_block_independence(); }
 require(allocations.load()==0,"no callback/render allocation after preparation");
 std::cout<<"PASS: source audio CPU checks; callback allocations "<<allocations.load()<<'\n';
}
