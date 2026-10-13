#pragma once
#include "audio.hpp"
#include "audio_pcm.hpp"
#include <SDL.h>
#if defined(__linux__)
#include <time.h>
#endif

namespace fl::audio_detail {
inline uint64_t thread_cpu_ns() noexcept {
#if defined(__linux__) && defined(CLOCK_THREAD_CPUTIME_ID)
 timespec t{};
 if(clock_gettime(CLOCK_THREAD_CPUTIME_ID,&t)==0) return uint64_t(t.tv_sec)*1000000000+uint64_t(t.tv_nsec);
#endif
 return 0; // unavailable; never infer CPU time from wall time
}
// One writer per counter set. Readers get advisory cumulative atomic snapshots.
class AudioTimingCounters {
 std::atomic<uint64_t> count_{0},total_{0},max_{0},over_{0};
 std::atomic<uint64_t> cpu_count_{0},cpu_total_{0},cpu_max_{0},unavailable_{0};
public:
 void reset(){count_=0;total_=0;max_=0;over_=0;cpu_count_=0;cpu_total_=0;cpu_max_=0;unavailable_=0;}
 void record(uint64_t wall,uint64_t cpu_start,uint64_t cpu_end,size_t frames,int rate) noexcept {
  count_.fetch_add(1,std::memory_order_relaxed);total_.fetch_add(wall,std::memory_order_relaxed);
  max_.store(std::max(max_.load(std::memory_order_relaxed),wall),std::memory_order_relaxed);
  if(double(wall)>double(frames)*1e9/rate)over_.fetch_add(1,std::memory_order_relaxed);
  if(cpu_start && cpu_end>=cpu_start){
   const auto ns=cpu_end-cpu_start;cpu_count_.fetch_add(1,std::memory_order_relaxed);cpu_total_.fetch_add(ns,std::memory_order_relaxed);
   cpu_max_.store(std::max(cpu_max_.load(std::memory_order_relaxed),ns),std::memory_order_relaxed);
  }else unavailable_.fetch_add(1,std::memory_order_relaxed);
 }
 AudioTimingDiagnostics snapshot() const {
  return {count_.load(),total_.load(),max_.load(),over_.load(),cpu_count_.load(),cpu_total_.load(),cpu_max_.load(),unavailable_.load()};
 }
};
inline uint64_t wall_elapsed_ns(uint64_t start,uint64_t frequency) noexcept {
 return uint64_t(double(SDL_GetPerformanceCounter()-start)*1e9/double(frequency));
}
// This is the entire buffered callback path, also invoked directly by tests.
// No producer notification: only copies, silence, numeric atomics and clocks.
template<size_t Capacity> class PcmConsumer {
 std::atomic<uint64_t> starved_{0},missing_{0};
 std::atomic<uint32_t> low_{Capacity};
public:
 AudioTimingCounters timing;
 void reset(){starved_=0;missing_=0;low_=Capacity;timing.reset();}
 void callback(PcmRing<Capacity>& ring,void* output,size_t bytes,int rate,uint64_t frequency) noexcept {
  const auto wall=SDL_GetPerformanceCounter(),cpu=thread_cpu_ns();
  const size_t frames=bytes/(2*sizeof(float)),copied=ring.consume(output,bytes);
  if(copied<frames){starved_.fetch_add(1,std::memory_order_relaxed);missing_.fetch_add(frames-copied,std::memory_order_relaxed);}
  const auto depth=uint32_t(ring.depth());low_.store(std::min(low_.load(std::memory_order_relaxed),depth),std::memory_order_relaxed);
  const auto end_cpu=thread_cpu_ns();timing.record(wall_elapsed_ns(wall,frequency),cpu,end_cpu,frames,rate);
 }
 uint64_t starved()const{return starved_.load();}
 uint64_t missing()const{return missing_.load();}
 uint32_t low()const{return low_.load();}
};
}
