#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace fl::audio_detail {
// Frame-addressed stereo SPSC. Exactly one producer and one consumer; reset
// requires both to be quiescent. Release cursors transfer PCM ownership.
template<size_t Capacity> class PcmRing {
 static_assert(Capacity && !(Capacity & (Capacity-1)), "power-of-two PCM capacity");
 static_assert(std::atomic<uint64_t>::is_always_lock_free, "PCM cursors must be lock-free");
 std::array<float,Capacity*2> pcm_{};
 alignas(64) std::atomic<uint64_t> written_{0};
 alignas(64) std::atomic<uint64_t> retired_{0};
public:
 static constexpr size_t capacity=Capacity;
 void reset() { written_.store(0); retired_.store(0); }
 // All-or-nothing publication, so an unpublished DSP block is never visible.
 bool push(const float* stereo,size_t frames) {
  const auto w=written_.load(std::memory_order_relaxed);
  const auto r=retired_.load(std::memory_order_acquire);
  if(frames>Capacity || frames>Capacity-size_t(w-r)) return false;
  const size_t at=size_t(w)&(Capacity-1),first=std::min(frames,Capacity-at);
  std::memcpy(pcm_.data()+at*2,stereo,first*2*sizeof(float));
  std::memcpy(pcm_.data(),stereo+first*2,(frames-first)*2*sizeof(float));
  written_.store(w+frames,std::memory_order_release); return true;
 }
 // Initialize every byte, including incomplete stereo frames. Shortage never
 // advances the musical frontier or replays data. At most two bounded copies.
 size_t consume(void* destination,size_t bytes) {
  std::memset(destination,0,bytes);
  const auto r=retired_.load(std::memory_order_relaxed);
  const auto w=written_.load(std::memory_order_acquire);
  const size_t frames=std::min(bytes/(2*sizeof(float)),size_t(w-r));
  const size_t at=size_t(r)&(Capacity-1),first=std::min(frames,Capacity-at);
  std::memcpy(destination,pcm_.data()+at*2,first*2*sizeof(float));
  std::memcpy(static_cast<unsigned char*>(destination)+first*2*sizeof(float),pcm_.data(),(frames-first)*2*sizeof(float));
  retired_.store(r+frames,std::memory_order_release); return frames;
 }
 uint64_t delivered() const { return retired_.load(std::memory_order_relaxed); }
 // Advisory snapshot; concurrent cursors can straddle a transfer. Clamp rather
 // than report an unsigned underflow as enormous latency.
 size_t depth() const {
  const auto r=retired_.load(std::memory_order_acquire);
  const auto w=written_.load(std::memory_order_acquire);
  return std::min(size_t(w-r),Capacity);
 }
};
}
