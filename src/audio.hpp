#pragma once
#include "feedback.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace fl {
enum class AudioSection : uint8_t { Intro, Verse, Hook, Break, Glitch };
enum class AudioVoice : uint8_t {
 Kick, KickClick, Snare, SnareTone, Hat, Rhodes, Bass, Pad, Chop,
 Blip, Acid, Pop, Impact, LandNoise, UberNoise, BailNoise, Count
};
struct AudioDiagnostics {
 uint64_t frames=0, steps=0, sections=0, dropped_commands=0;
 uint64_t stolen_voices=0, dropped_notes=0, nonfinite_samples=0;
 uint32_t queue_high_water=0, peak_voices=0, active_voices=0;
 int sample_rate=0,buffer_frames=0;
 uint32_t queue_depth=0;
 uint64_t callback_count=0,callback_total_ns=0,callback_max_ns=0,callback_over_budget=0;
 AudioSection section=AudioSection::Intro;
 double drums_gain=1, music_cutoff=9000, carve_gain=0, wind_gain=0, grind_gain=0;
 bool sustained_air=false, grotesk=false, muted=false;
 std::array<uint64_t, static_cast<size_t>(AudioVoice::Count)> voices_started{};
};
// A bounded source-event trace, for offline inspection only. Sample is the
// scheduled start, not a simulation tick. No strings enter the audio thread.
struct AudioTrace {
 uint64_t sample=0, step=0;
 AudioVoice voice=AudioVoice::Kick;
 AudioSection section=AudioSection::Intro;
 int note=0;
 double velocity=0;
};
class AudioEngine {
public:
 AudioEngine();
 ~AudioEngine();
 AudioEngine(const AudioEngine&)=delete;
 AudioEngine& operator=(const AudioEngine&)=delete;
 bool open();                         // main thread, paused device setup
 void close();                        // stops callback before releasing storage
 void pause(bool);
 void set_ride(const RideSound&);
 void trigger(const GameEvent&);
 void set_muted(bool);                 // source output gain target, not clock stop
 // Closed-device CPU test/evidence mode. Reinitializes the song explicitly;
 // gameplay Restart does not. Stereo interleaved output, no render allocation.
 bool prepare_offline(int sample_rate=48000, uint32_t seed=1);
 void render_offline(float* stereo, size_t frames);
 AudioDiagnostics diagnostics() const; // lock-free bounded numeric snapshot
 size_t take_trace(AudioTrace* into, size_t capacity); // offline only
 static constexpr double bpm=86;
 static constexpr double sixteenth=60.0/bpm/4.0;
 static constexpr size_t voice_capacity=128;
 static constexpr size_t command_capacity=256;
 static constexpr size_t reverb_partition=1024;
private:
 struct Impl;
 std::unique_ptr<Impl> impl_;
};
}
