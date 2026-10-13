// API common to base0ff0876 and buffered candidate. Raw float/trace identity,
// not WAV quantization, with the same seed and numeric command sequence.
#include "audio.hpp"
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
int main(int argc,char** argv){
 if(argc!=2)return 2;
 fl::AudioEngine engine;if(!engine.prepare_offline(48000,19))return 3;
 std::ofstream out(argv[1],std::ios::binary);std::array<float,2048> block{};
 constexpr size_t frames=48000*90;
 for(size_t at=0;at<frames;at+=1024){
  double t=double(at)/48000;fl::RideSound ride;
  ride.carve=t>5&&t<12?.8:0;ride.wind=t>12&&t<18?.9:0;ride.sustained_air=t>13&&t<18;ride.grinding=t>24&&t<28;
  engine.set_ride(ride);
  auto crossed=[&](double when){return t>=when&&t<when+1024./48000;};
  auto event=[&](double when,fl::GameEventKind kind,double amount=0,bool flag=false){if(crossed(when))engine.trigger({kind,amount,flag,0,{}});};
  event(12,fl::GameEventKind::Pop);event(18,fl::GameEventKind::Land,.7,true);event(30,fl::GameEventKind::Bail);event(50,fl::GameEventKind::Uber);event(65,fl::GameEventKind::GroteskOn);event(80,fl::GameEventKind::GroteskOff);event(85,fl::GameEventKind::Restart);
  size_t n=std::min(size_t(1024),frames-at);engine.render_offline(block.data(),n);
  for(size_t i=0;i<n*2;++i)if(!std::isfinite(block[i]))return 4;
  out.write(reinterpret_cast<const char*>(block.data()),std::streamsize(n*2*sizeof(float)));
 }
 std::array<fl::AudioTrace,4096> trace{};auto count=engine.take_trace(trace.data(),trace.size());
 for(size_t i=0;i<count;++i){const auto& e=trace[i];std::cout<<e.sample<<','<<e.step<<','<<int(e.voice)<<','<<int(e.section)<<','<<e.note<<','<<std::hexfloat<<e.velocity<<'\n';}
 return out?0:5;
}
