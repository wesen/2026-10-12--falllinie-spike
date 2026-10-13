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
// Source WaveShaper curve spans [-1,1]; inputs outside it use endpoint values.
inline double acid_shape(double bus) { return std::tanh(clamp(bus*.5,-1,1)*3); }
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
// 49-tap Blackman half-band FIR: symmetric odd taps plus exact 0.5 centre.
// Polyphase zeros/symmetry reduce each high-rate sample to 12 paired MACs.
// Both interpolation and decimation have fixed storage, including at 192kHz.
struct HalfBand {
 static constexpr size_t taps=49, delay=24;
 std::array<double,12> coefficients{};
 std::array<double,64> history{};
 size_t cursor=0;
 void init(){
  history.fill(0);cursor=0;double sum=0;
  for(size_t j=0;j<coefficients.size();++j){
   double k=double(2*j+1),m=k-delay;
   double window=.42-.5*std::cos(2*pi*k/(taps-1))+.08*std::cos(4*pi*k/(taps-1));
   coefficients[j]=std::sin(pi*.5*m)/(pi*m)*window;sum+=2*coefficients[j];
  }
  // Preserve exact unity DC and the exact zero taps of the half-band.
  for(auto& c:coefficients)c*=.5/sum;
 }
 double process(double x){
  history[cursor]=x;double out=.5*history[(cursor+64-delay)&63];
  for(size_t j=0;j<coefficients.size();++j){size_t k=2*j+1;out+=coefficients[j]*(history[(cursor+64-k)&63]+history[(cursor+64-(taps-1-k))&63]);}
  cursor=(cursor+1)&63;return out;
 }
};
struct Saturation2x {
 HalfBand interpolate,decimate;
 void init(){interpolate.init();decimate.init();}
 static double shape(double x){return std::tanh(clamp(x,-1,1)*1.5)/std::tanh(1.5);}
 double process(double x){
  // Zero-insert, reconstruct at 2x, apply the original nonlinear curve there,
  // anti-alias, then decimate. 24 base-rate samples total fixed group delay.
  double out=decimate.process(shape(2*interpolate.process(x)));
  decimate.process(shape(2*interpolate.process(0)));
  return out;
 }
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
// Numeric FFT inputs are finite and bounded. Explicit real arithmetic avoids
// general-purpose complex overflow recovery and permits SIMD in convolution.
inline std::complex<float> complex_product(std::complex<float> a,std::complex<float> b){
 return {a.real()*b.real()-a.imag()*b.imag(),a.real()*b.imag()+a.imag()*b.real()};
}
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((optimize("O3")))
#endif
inline void planar_accumulate(float* __restrict out_r,float* __restrict out_i,const float* __restrict ar,const float* __restrict ai,const float* __restrict br,const float* __restrict bi,size_t count){
 // Separate planes let ARM NEON process four independent bins per vector,
 // without interleaved complex shuffles. No fast-math/reassociation required.
 for(size_t j=0;j<count;++j){out_r[j]+=ar[j]*br[j]-ai[j]*bi[j];out_i[j]+=ar[j]*bi[j]+ai[j]*br[j];}
}
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
   auto a=x[base+j],b=complex_product(x[base+j+len/2],root); x[base+j]=a+b; x[base+j+len/2]=a-b;
  }
  if(inverse) for(size_t i=0;i<n;++i) x[i]/=float(n);
 }
};
// Uniform partitioned overlap-add, stereo IR / stereo input. One partition of
// explicit wet latency; O(partitions * FFT-size) per block, not per sample.
struct Reverb {
 static constexpr size_t block=1024, n=block*2, bins=n/2+1;
 FFT fft;
 size_t partitions=0,head=0,pos=0,next_tail=1;
 std::vector<float> impulse_real[2],impulse_imag[2],history_real[2],history_imag[2];
 std::array<std::complex<float>,n> scratch{},acc{};
 std::array<float,bins> tail_real[2]{},tail_imag[2]{};
 std::array<float,block> input[2]{},output[2]{},overlap[2]{};
 void init(int rate,Random& rng) {
  fft.init(n); size_t length=static_cast<size_t>(rate*2.6); partitions=(length+block-1)/block;
  for(int ch=0;ch<2;++ch) { history_real[ch].assign(partitions*bins,0);history_imag[ch].assign(partitions*bins,0); input[ch].fill(0); }
  head=pos=0;next_tail=1;for(auto& t:tail_real)t.fill(0);for(auto& t:tail_imag)t.fill(0);
  std::vector<float> ir[2]; double power=0;
  for(int ch=0;ch<2;++ch) { ir[ch].resize(length); for(size_t i=0;i<length;++i) { double v=(rng.next()*2-1)*std::pow(1-double(i)/length,3.4); ir[ch][i]=float(v); power+=v*v; } }
  // WebAudio ConvolverNode normalize default: RMS calibration and sample-rate correction.
  double scale=0.00125/std::max(0.000125,std::sqrt(power/(length*2)))*44100/rate;
  for(int ch=0;ch<2;++ch) {
   output[ch].fill(0); overlap[ch].fill(0); impulse_real[ch].resize(partitions*bins);impulse_imag[ch].resize(partitions*bins);
   for(size_t p=0;p<partitions;++p) { scratch.fill({}); for(size_t j=0;j<block && p*block+j<length;++j) scratch[j]=ir[ch][p*block+j]*float(scale); fft.run(scratch.data()); for(size_t j=0;j<bins;++j){impulse_real[ch][p*bins+j]=scratch[j].real();impulse_imag[ch][p*bins+j]=scratch[j].imag();} }
  }
 }
 void process(double in_left,double in_right,double& left,double& right) {
  left=output[0][pos]; right=output[1][pos]; input[0][pos]=float(in_left); input[1][pos]=float(in_right);
  ++pos;
  // All delayed partitions depend only on already complete history blocks.
  // Spread that work across this block instead of bursting every other SDL
  // callback. Only the newest partition/FFT must wait for the final sample.
  // 64-sample groups retain locality for voices between convolution batches.
  const size_t due=(pos/64)*(partitions-1)/(block/64);
  while(next_tail<=due){size_t p=next_tail++;for(int ch=0;ch<2;++ch){size_t old=((head+partitions-p)%partitions)*bins;planar_accumulate(tail_real[ch].data(),tail_imag[ch].data(),impulse_real[ch].data()+p*bins,impulse_imag[ch].data()+p*bins,history_real[ch].data()+old,history_imag[ch].data()+old,bins);}}
  if(pos<block)return;
  pos=0;
  for(int ch=0;ch<2;++ch) {
   scratch.fill({}); for(size_t j=0;j<block;++j) scratch[j]=input[ch][j]; fft.run(scratch.data());
   for(size_t j=0;j<bins;++j){history_real[ch][head*bins+j]=scratch[j].real();history_imag[ch][head*bins+j]=scratch[j].imag();}
   planar_accumulate(tail_real[ch].data(),tail_imag[ch].data(),impulse_real[ch].data(),impulse_imag[ch].data(),history_real[ch].data()+head*bins,history_imag[ch].data()+head*bins,bins);
   for(size_t j=0;j<bins;++j)acc[j]={tail_real[ch][j],tail_imag[ch][j]};
   // Real input/IR spectra are Hermitian: the other half carries no new data.
   for(size_t j=1;j<n/2;++j)acc[n-j]=std::conj(acc[j]);
   fft.run(acc.data(),true);
   for(size_t j=0;j<block;++j) { output[ch][j]=acc[j].real()+overlap[ch][j]; overlap[ch][j]=acc[j+block].real(); }
  }
  head=(head+1)%partitions;next_tail=1;for(auto& t:tail_real)t.fill(0);for(auto& t:tail_imag)t.fill(0);
 }
};
}
