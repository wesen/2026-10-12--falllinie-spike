#include "audio_pcm.hpp"
#include "audio_timing.hpp"
#include <array>
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <new>
#include <thread>
namespace {
thread_local bool watch=false;
std::atomic<size_t> allocations{0};
void require(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
void deterministic(){
 fl::audio_detail::PcmRing<8> ring;
 std::array<float,16> input{},output{};
 for(size_t i=0;i<8;++i){input[i*2]=float(i+1);input[i*2+1]=-float(i+1);}
 output.fill(99);watch=true;require(ring.consume(output.data(),sizeof(output))==0,"empty copied frames");watch=false;
 for(float x:output)require(x==0,"empty shortage not silence");
 require(ring.push(input.data(),8),"fill");require(!ring.push(input.data(),1),"full overwrite");
 watch=true;require(ring.consume(output.data(),3*8)==3,"partial read");watch=false;
 for(size_t i=0;i<6;++i)require(output[i]==input[i],"stereo identity");
 require(ring.depth()==5,"partial depth");require(ring.push(input.data(),3),"wrapped refill");
 watch=true;require(ring.consume(output.data(),sizeof(output))==8,"wrapped read");watch=false;
 for(size_t i=0;i<16;++i)require(output[i]==input[(i+6)%16],"wrapped FIFO identity");
 std::array<unsigned char,37> malformed{};malformed.fill(0xff);
 require(ring.push(input.data(),2),"short publication");watch=true;
 require(ring.consume(malformed.data(),malformed.size())==2,"short oversized request");watch=false;
 require(std::memcmp(malformed.data(),input.data(),16)==0,"unaligned copy");
 for(size_t i=16;i<malformed.size();++i)require(malformed[i]==0,"malformed tail silence");
 require(ring.delivered()==13,"delivered excludes shortage");
 ring.reset();require(ring.depth()==0&&ring.delivered()==0,"reset");
 require(!ring.push(input.data(),9),"oversized producer rejected");
}
void callback_safety(){
 fl::audio_detail::PcmRing<8> ring;fl::audio_detail::PcmConsumer<8> consumer;
 std::array<float,6> input{1,-1,2,-2,3,-3};std::array<unsigned char,83> output{};
 require(ring.push(input.data(),3),"callback prefill");
 const auto frequency=SDL_GetPerformanceFrequency();watch=true;
 consumer.callback(ring,output.data(),output.size(),48000,frequency);
 consumer.callback(ring,output.data(),output.size(),48000,frequency);watch=false;
 require(consumer.starved()==2&&consumer.missing()==17,"distinct callback/missing-frame counts");
 for(auto b:output)require(b==0,"starved callback initialized bytes");
 require(ring.delivered()==3,"callback delivers only generated frames");
 auto timing=consumer.timing.snapshot();require(timing.count==2,"callback wall count");
 require(timing.cpu_count+timing.cpu_unavailable==2,"CPU clock availability counted");
 consumer.reset();require(consumer.starved()==0&&consumer.missing()==0,"callback counters reset");
}
void stress(){
 fl::audio_detail::PcmRing<1024> ring;constexpr size_t total=500000;
 std::thread producer([&]{std::array<float,62> block{};size_t at=0;while(at<total){size_t n=std::min(size_t(31),total-at);for(size_t i=0;i<n;++i){block[i*2]=float(at+i);block[i*2+1]=-float(at+i);}if(ring.push(block.data(),n))at+=n;else std::this_thread::yield();}});
 std::array<float,194> output{};size_t at=0;
 while(at<total){watch=true;size_t n=ring.consume(output.data(),std::min(size_t(97),total-at)*8);watch=false;for(size_t i=0;i<n;++i){require(output[i*2]==float(at+i)&&output[i*2+1]==-float(at+i),"concurrent FIFO");}at+=n;if(!n)std::this_thread::yield();}
 producer.join();require(ring.depth()==0&&ring.delivered()==total,"stress final cursors");
}
}
void* operator new(std::size_t n){if(watch)allocations.fetch_add(1);if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
int main(){deterministic();callback_safety();stress();require(allocations.load()==0,"consumer allocation");std::cout<<"PCM stereo/partial/wrap/full/empty/malformed shortage/FIFO stress passed; consumer allocations 0\n";}
