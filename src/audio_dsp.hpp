#pragma once
// Self-contained numerical equivalents of the source WebAudio primitives.
// Setup allocates buffers; process methods use only preallocated numeric state.
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdint>
#include <vector>

namespace fl::audio_detail {
constexpr double pi=3.14159265358979323846;
inline double clamp(double x, double lo, double hi) { return std::max(lo,std::min(hi,x)); }
struct Random {
 uint32_t state=1;
 double next() { state^=state<<13; state^=state>>17; state^=state<<5; return state/4294967296.0; }
 int pick(int n) { return static_cast<int>(next()*n); }
};
struct Sines {
 std::array<double,4097> data{};
 Sines() { for(size_t i=0;i<data.size();++i) data[i]=std::sin(2*pi*i/4096); }
 double at(double phase) const { phase-=std::floor(phase); double x=phase*4096; auto i=static_cast<size_t>(x); return data[i]+(data[i+1]-data[i])*(x-i); }
};
inline double blep(double t, double dt) {
 if(t<dt) { t/=dt; return t+t-t*t-1; }
 if(t>1-dt) { t=(t-1)/dt; return t*t+t+t+1; }
 return 0;
}
inline double oscillator(double& phase,double frequency,int waveform,double rate,const Sines& table) {
 double signed_dt=clamp(frequency/rate,-0.45,0.45), dt=std::max(0.0000001,std::abs(signed_dt)), v;
 if(waveform==0) v=table.at(phase);
 else if(waveform==1) v=phase<0.25?4*phase:(phase<0.75?2-4*phase:4*phase-4); // triangle, lower alias energy
 else if(waveform==2) v=2*phase-1-blep(phase,dt);
 else v=(phase<0.5?1:-1)+blep(phase,dt)-blep(std::fmod(phase+0.5,1.0),dt);
 phase+=signed_dt; phase-=std::floor(phase); return v;
}
enum class FilterType { Low, High, Band };
struct Biquad {
 double b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
 void set(FilterType type,double frequency,double q,double rate) {
  // WebAudio low/high-pass Q is resonance in dB; bandpass Q is linear.
  if(type!=FilterType::Band) q=std::pow(10.0,q/20);
  double w=2*pi*clamp(frequency,10,rate*0.49)/rate, c=std::cos(w), a=std::sin(w)/(2*q), norm=1/(1+a);
  if(type==FilterType::Low) { b0=(1-c)*0.5*norm; b1=(1-c)*norm; b2=b0; }
  else if(type==FilterType::High) { b0=(1+c)*0.5*norm; b1=-(1+c)*norm; b2=b0; }
  else { b0=a*norm; b1=0; b2=-b0; }
  a1=-2*c*norm; a2=(1-a)*norm;
 }
 double process(double x) { double y=b0*x+z1; z1=b1*x-a1*y+z2; z2=b2*x-a2*y; return y; }
};
// AudioParam-style piecewise exponential/constant/linear automation. Knots are
// converted to sample counts at voice creation; segment advancement is bounded.
struct Envelope {
 struct Knot { uint64_t sample; double value; bool exponential; };
 std::array<Knot,8> knots{};
 size_t count=0,segment=0;
 double value=0.0001,mul=1,add=0;
 void reset() { count=segment=0; value=0.0001; mul=1; add=0; }
 void point(double seconds,double v,bool exponential,double rate) {
  knots[count++]={static_cast<uint64_t>(std::max(0.0,seconds)*rate+0.5),v,exponential};
 }
 void segment_coeff() {
  if(segment+1>=count) { mul=1; add=0; return; }
  const auto& a=knots[segment]; const auto& b=knots[segment+1]; double n=double(std::max<uint64_t>(1,b.sample-a.sample));
  mul=b.exponential?std::pow(std::max(0.0000001,b.value)/std::max(0.0000001,a.value),1/n):1;
  add=b.exponential?0:(b.value-a.value)/n;
 }
 double tick(uint64_t age) {
  if(age==0) { value=knots[0].value; segment_coeff(); }
  while(segment+1<count && age>=knots[segment+1].sample) { ++segment; value=knots[segment].value; segment_coeff(); }
  double out=value; value=value*mul+add; return out;
 }
 void simple(double attack,double peak,double decay,double rate) {
  reset(); point(0,0.0001,false,rate); point(attack,std::max(peak,0.0002),true,rate); point(attack+decay,0.0001,true,rate);
 }
};
struct FFT {
 size_t n=0;
 std::vector<size_t> reverse;
 std::vector<std::complex<float>> roots;
 void init(size_t size) {
  n=size; reverse.resize(n); roots.resize(n/2);
  for(size_t i=0;i<n;++i) { size_t r=0,x=i; for(size_t bit=1;bit<n;bit<<=1) { r=(r<<1)|(x&1); x>>=1; } reverse[i]=r; }
  for(size_t i=0;i<n/2;++i) roots[i]={float(std::cos(-2*pi*i/n)),float(std::sin(-2*pi*i/n))};
 }
 void run(std::complex<float>* x,bool inverse=false) const {
  for(size_t i=0;i<n;++i) if(i<reverse[i]) std::swap(x[i],x[reverse[i]]);
  for(size_t len=2;len<=n;len*=2) for(size_t base=0;base<n;base+=len) for(size_t j=0;j<len/2;++j) {
   auto root=roots[j*n/len]; if(inverse) root=std::conj(root);
   auto a=x[base+j],b=x[base+j+len/2]*root; x[base+j]=a+b; x[base+j+len/2]=a-b;
  }
  if(inverse) for(size_t i=0;i<n;++i) x[i]/=float(n);
 }
};
// Uniform partitioned overlap-add, stereo IR / stereo input. One partition of
// explicit wet latency; O(partitions * FFT-size) per block, not per sample.
struct Reverb {
 static constexpr size_t block=1024, n=block*2;
 FFT fft;
 size_t partitions=0,head=0,pos=0;
 std::vector<std::complex<float>> impulse[2],history[2];
 std::array<std::complex<float>,n> scratch{},acc{};
 std::array<float,block> input[2]{},output[2]{},overlap[2]{};
 void init(int rate,Random& rng) {
  fft.init(n); size_t length=static_cast<size_t>(rate*2.6); partitions=(length+block-1)/block;
  for(int ch=0;ch<2;++ch) { history[ch].assign(partitions*n,{}); input[ch].fill(0); }
  head=pos=0;
  std::vector<float> ir[2]; double power=0;
  for(int ch=0;ch<2;++ch) { ir[ch].resize(length); for(size_t i=0;i<length;++i) { double v=(rng.next()*2-1)*std::pow(1-double(i)/length,3.4); ir[ch][i]=float(v); power+=v*v; } }
  // WebAudio ConvolverNode normalize default: RMS calibration and sample-rate correction.
  double scale=0.00125/std::max(0.000125,std::sqrt(power/(length*2)))*44100/rate;
  for(int ch=0;ch<2;++ch) {
   output[ch].fill(0); overlap[ch].fill(0); impulse[ch].resize(partitions*n);
   for(size_t p=0;p<partitions;++p) { scratch.fill({}); for(size_t j=0;j<block && p*block+j<length;++j) scratch[j]=ir[ch][p*block+j]*float(scale); fft.run(scratch.data()); std::copy(scratch.begin(),scratch.end(),impulse[ch].begin()+p*n); }
  }
 }
 void process(double in_left,double in_right,double& left,double& right) {
  left=output[0][pos]; right=output[1][pos]; input[0][pos]=float(in_left); input[1][pos]=float(in_right);
  if(++pos<block) return;
  pos=0;
  for(int ch=0;ch<2;++ch) {
   scratch.fill({}); for(size_t j=0;j<block;++j) scratch[j]=input[ch][j]; fft.run(scratch.data());
   std::copy(scratch.begin(),scratch.end(),history[ch].begin()+head*n);
   acc.fill({});
   for(size_t p=0;p<partitions;++p) { const auto* h=impulse[ch].data()+p*n; const auto* in=history[ch].data()+((head+partitions-p)%partitions)*n; for(size_t j=0;j<n;++j) acc[j]+=h[j]*in[j]; }
   fft.run(acc.data(),true);
   for(size_t j=0;j<block;++j) { output[ch][j]=acc[j].real()+overlap[ch][j]; overlap[ch][j]=acc[j+block].real(); }
  }
  head=(head+1)%partitions;
 }
};
}
