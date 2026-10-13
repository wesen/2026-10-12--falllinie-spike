#pragma once
#include "game.hpp"
#include "audio.hpp"
#include <array>
#include <SDL_scancode.h>

namespace fl {
// One physical snapshot per frame. SDL polling/lifecycle stays in main.
struct NativeInputSnapshot {
 std::array<uint8_t,SDL_NUM_SCANCODES> keys{};
 std::array<bool,17> buttons{};
 double axis_x=0,axis_y=0;
 bool quit=false,focus_lost=false;
};
struct NativeActions {
 Controls ride;
 bool quit=false,start=false,pause=false,new_run=false,mute=false,palette=false,debug=false;
};
class NativeInputMapper {
 NativeInputSnapshot previous_;
public:
 NativeActions read(const NativeInputSnapshot&,bool title);
};
// Source title demo, using a separate seeded RNG instead of Math.random.
class TitleAutopilot {
 Rng rng_;
 double time_=0,hop_=3;
 struct Plan {double spin=0,flip=0;uint8_t grab=0;};
 std::optional<Plan> plan_;
public:
 explicit TitleAutopilot(uint32_t seed):rng_{seed^0x416e79u}{}
 Controls read(Game&,double raw_dt);
};
struct FrameResult {double raw_dt=0,scale=1;int ticks=0;};
struct NativeFrameClock {
 double accumulator=0;
 FrameResult advance(Game&,Controls,double raw_dt);
};
// Sole producer, after all fixed ticks and renderer update, never per view.
void publish_feedback(AudioEngine&,Game&,bool audio_started);
}
