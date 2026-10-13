#pragma once
#include <string>
namespace fl {
// Main-thread gameplay feedback. Audio transfers numeric fields only; strings
// never enter its real-time callback. This is the integration contract.
enum class GameEventKind {
 Pop, Land, Bail, Uber, GroteskOn, GroteskOff, GrindOn, GrindOff,
 ComboAdded, ComboBanked, Recovery, Restart
};
struct GameEvent {
 GameEventKind kind;
 double amount=0;
 bool flag=false;
 int points=0;
 std::string text;
};
struct RideSound {
 double carve=0,wind=0,speed=0,air_time=0;
 bool sustained_air=false,grinding=false,grotesk=false;
};
}
