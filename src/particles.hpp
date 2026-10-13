#pragma once
#include "terrain.hpp"
#include "quaternion.hpp"
#include <optional>
namespace fl {
struct SnowParticle {Vec3 p,v;double life=0,max=0,size=0;};
struct TrailPoint {Vec3 p,side;double time=0;bool break_before=false;};
class Particles {
public:
 std::vector<SnowParticle> snow;std::vector<TrailPoint> trail;
 Particles(){snow.reserve(700);trail.reserve(700);}
 void clear();void spray(int count,bool sparks,Vec3 position,Vec3 velocity,Quat q,Rng& rng);
 void step(double dt);void add_trail(Vec3 position,Vec3 normal,Vec3 velocity,double time);void break_trail();
 CpuMesh snow_mesh(Vec3 eye,Vec3 right,Vec3 up)const;CpuMesh trail_mesh(double time)const;
private:
 std::optional<Vec3> last_;bool break_next_=false;
};
}
