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
 // grotesk is a UI/state diagnostic; audio is latched exclusively by On/Off
 // events so bail/pause snapshots cannot cancel or duplicate source reactions.
 bool sustained_air=false,grinding=false,grotesk=false;
};
}
