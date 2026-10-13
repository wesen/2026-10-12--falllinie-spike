#pragma once
#include "terrain.hpp"
namespace fl {
struct Controls {double steer=0,drive=0;bool jump=false;};
// First playable slice: source ground forces and charged pop; forgiving landings.
// Full trick/rail/bail state machines and reactive audio are not ported yet.
class Game {
public:
 Terrain terrain;
 Vec3 position{},velocity{};
 double yaw=0,charge=0,crouch=0,distance=0,time=0;
 Vec3 board_normal{0,1,0};
 bool airborne=false;
 explicit Game(uint32_t seed):terrain(seed){reset();}
 void reset(){double z=4;position={Terrain::center(z),terrain.height(Terrain::center(z),z),z};yaw=std::atan2(55.0/310*std::cos(z/310)+22.0/127*std::cos(z/127+2.1)+8.0/53*std::cos(z/53+.4),1);board_normal=terrain.normal(position.x,z);Vec3 f{std::sin(yaw),0,std::cos(yaw)};velocity=norm(f-board_normal*dot(f,board_normal))*6;airborne=false;charge=0;crouch=0;previous_jump=false;distance=0;time=0;}
 void step(Controls input,double dt){
  time+=dt;bool release=previous_jump&&!input.jump;previous_jump=input.jump;
  if(!airborne){
   Vec3 n=terrain.normal(position.x,position.z);board_normal=n;
   if(position.y+velocity.y*dt-.5*20.8*dt*dt>terrain.height(position.x+velocity.x*dt,position.z+velocity.z*dt)+.002)airborne=true;
   else {
    Vec3 f=norm(Vec3{std::sin(yaw),0,std::cos(yaw)}-n*dot({std::sin(yaw),0,std::cos(yaw)},n));Vec3 s=cross(n,f),g{0,-13,0};
    velocity=velocity+(g-n*dot(g,n))*dt;velocity=velocity-n*dot(velocity,n);
    double vf=dot(velocity,f),vs=dot(velocity,s),sp0=std::hypot(vf,vs);
    double brake=input.drive<-.2?-input.drive:0,tuck=input.drive>.2?input.drive:0;
    double nvs=vs*std::exp(-(7-3*brake)*dt),sp1=std::hypot(vf,nvs),target=lerp(sp1,sp0,.9-.55*brake);
    if(sp1>1e-5){vf*=target/sp1;vs=nvs*target/sp1;}
    double sp=std::hypot(vf,vs),next=std::max(0.0,sp-(.55+(.0042-.0016*tuck)*sp*sp+brake*9)*dt);
    if(sp>1e-4){vf*=next/sp;vs*=next/sp;}
    velocity=f*vf+s*vs;yaw-=input.steer*lerp(2.5,1.55,clamp(sp/30,0,1))*dt;
    if(input.jump)charge=std::min(1.0,charge+dt/.45);
    crouch=lerp(crouch,input.jump?.35+.65*charge:tuck*.55+brake*.2,1-std::exp(-dt*12));
    if(release){velocity=velocity+n*(4.2+3.6*charge);charge=0;airborne=true;}
   }
  }
  if(airborne){velocity.y-=13*dt;yaw-=input.steer*2*dt;}
  position=position+velocity*dt;
  double ground=terrain.height(position.x,position.z);
  if(position.y<=ground || !airborne){position.y=ground;Vec3 n=terrain.normal(position.x,position.z);double vn=dot(velocity,n);if(vn<0)velocity=velocity-n*vn;airborne=false;}
  distance=std::max(distance,position.z-4);
  auto p=terrain.params(position.z);
  if(std::abs(position.x-Terrain::center(position.z))>p.W/2+p.R+16){position.x=Terrain::center(position.z);position.y=terrain.height(position.x,position.z);velocity=norm({0,-.3,1})*7;yaw=0;airborne=false;}
 }
private:bool previous_jump=false;
};
}
