#pragma once
#include "terrain.hpp"
#include "quaternion.hpp"
#include <limits>
#include <iostream>
namespace fl {
struct Controls {double steer=0,drive=0;bool jump=false;};
// Source groundStep, airborne spin/flip completion and landing alignment.
// Grabs, uber tricks, rails, combos and reactive audio are subsequent slices.
class Game {
public:
 Terrain terrain;
 Vec3 position{},velocity{},board_normal{0,1,0};
 Quat orientation{},takeoff_orientation{};
 double yaw=0,charge=0,crouch=0,distance=0,time=0;
 double air_time=0,spin=0,flip=0,spin_velocity=0,flip_velocity=0,bail_time=0;
 int score=0,last_points=0;
 bool airborne=false,tricked=false;
 explicit Game(uint32_t seed):terrain(seed){reset();}
 void reset(){place(4);distance=0;time=0;score=0;last_points=0;}
 void place(double z){
  position={Terrain::center(z),terrain.height(Terrain::center(z),z),z};
  yaw=std::atan2(55.0/310*std::cos(z/310)+22.0/127*std::cos(z/127+2.1)+8.0/53*std::cos(z/53+.4),1);
  board_normal=terrain.normal(position.x,z);Vec3 f{std::sin(yaw),0,std::cos(yaw)};
  velocity=norm(f-board_normal*dot(f,board_normal))*6;orientation=frameq(f,board_normal);
  airborne=false;charge=0;crouch=0;previous_jump=false;bail_time=0;
 }
 void takeoff(){
  airborne=true;air_time=spin=flip=spin_velocity=flip_velocity=0;tricked=false;
  spin_goal=flip_goal=std::numeric_limits<double>::quiet_NaN();takeoff_orientation=orientation;air_normal={0,1,0};
  flip_axis=norm(cross(Vec3{0,1,0},Vec3{velocity.x,0,velocity.z}));
 }
 void step(Controls input,double dt){
  time+=dt;bool release=previous_jump&&!input.jump;previous_jump=input.jump;
  if(bail_time>0){bail_time-=dt;velocity=velocity*std::exp(-dt*2);velocity.y-=13*dt;position=position+velocity*dt;position.y=std::max(position.y,terrain.height(position.x,position.z));orientation=qmul(qaxis({1,0,0},dt*4),orientation);if(bail_time<=0)place(position.z+4);return;}
  if(!airborne){
   Vec3 n=terrain.normal(position.x,position.z);board_normal=n;
   if(position.y+velocity.y*dt-.5*20.8*dt*dt>terrain.height(position.x+velocity.x*dt,position.z+velocity.z*dt)+.002)takeoff();
   else {
    Vec3 forward{std::sin(yaw),0,std::cos(yaw)};
    Vec3 f=norm(forward-n*dot(forward,n)),s=cross(n,f),g{0,-13,0};
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
    orientation=frameq({std::sin(yaw),0,std::cos(yaw)},n);
    if(release){velocity=velocity+n*(4.2+3.6*charge);charge=0;takeoff();}
   }
  }else {
   air_time+=dt;velocity.y-=13*dt;velocity=velocity*(1-.0006*length(velocity)*dt);
   const double pi=3.141592653589793,tau=2*pi;
   if(std::abs(input.steer)>.3){spin_goal=std::numeric_limits<double>::quiet_NaN();spin_velocity+=(-std::copysign(7.2,input.steer)-spin_velocity)*std::min(1.0,dt*9);tricked=true;}
   else {if(std::isnan(spin_goal)){double k=spin/pi,fr=k-std::floor(k);spin_goal=(spin_velocity>.5&&fr>.25?std::ceil(k):spin_velocity<-.5&&fr<.75?std::floor(k):std::floor(k+.5))*pi;}spin_velocity+=(clamp((spin_goal-spin)*7,-7,7)-spin_velocity)*std::min(1.0,dt*10);}
   if(std::abs(input.drive)>.3 && air_time>.08){flip_goal=std::numeric_limits<double>::quiet_NaN();flip_velocity+=(std::copysign(6.0,input.drive)-flip_velocity)*std::min(1.0,dt*8);tricked=true;}
   else {if(std::isnan(flip_goal)){double k=flip/tau,fr=k-std::floor(k);flip_goal=(flip_velocity>.5&&fr>.22?std::ceil(k):flip_velocity<-.5&&fr<.78?std::floor(k):std::floor(k+.5))*tau;}flip_velocity+=(clamp((flip_goal-flip)*6,-6,6)-flip_velocity)*std::min(1.0,dt*9);}
   spin+=spin_velocity*dt;flip+=flip_velocity*dt;
   double above=position.y-terrain.height(position.x,position.z);Vec3 nb=above<6?terrain.normal(position.x+velocity.x*.15,position.z+velocity.z*.15):Vec3{0,1,0};air_normal=norm(air_normal+(nb-air_normal)*(1-std::exp(-dt*6)));
   Vec3 nose=qrot(takeoff_orientation,{1,0,0});Quat up=frameq(norm({nose.x,0,nose.z}),air_normal),base=qslerp(takeoff_orientation,up,1-std::exp(-air_time*2.5));
   orientation=qnorm(qmul(qaxis({0,1,0},spin),qmul(qaxis(flip_axis,flip),base)));crouch=lerp(crouch,.3,1-std::exp(-dt*10));
  }
  position=position+velocity*dt;double ground=terrain.height(position.x,position.z);
  if(position.y<=ground || !airborne){
   position.y=ground;Vec3 n=terrain.normal(position.x,position.z);double vn=dot(velocity,n);if(vn<0)velocity=velocity-n*vn;
   if(airborne)land(n);else board_normal=n;
  }
  distance=std::max(distance,position.z-4);auto p=terrain.params(position.z);
  out_of_bounds=std::abs(position.x-Terrain::center(position.z))>p.W/2+p.R+16?out_of_bounds+dt:0;
  stuck=!airborne&&length(velocity)<1.5?stuck+dt:0;
  if(out_of_bounds>1.2 || stuck>3){place(position.z+4);out_of_bounds=stuck=0;}
 }
private:
 bool previous_jump=false;
 Vec3 flip_axis{1,0,0},air_normal{0,1,0};
 double spin_goal=0,flip_goal=0,out_of_bounds=0,stuck=0;
 void land(Vec3 n){
  airborne=false;Vec3 x=qrot(orientation,{1,0,0}),y=qrot(orientation,{0,1,0});Vec3 horizontal=velocity-n*dot(velocity,n),vh=length(horizontal)>1e-9?norm(horizontal):x,xp=norm(x-n*dot(x,n));
  double up=dot(y,n),alignment=dot(xp,vh),speed=length(velocity);
  if(!(air_time<.3&&!tricked) && (up<.28 || (speed>4 && std::abs(alignment)<.36))){bail_time=1.4;last_points=0;std::cout<<"BAIL "<<(up<.28?"upside down":"sideways")<<'\n';return;}
  if((up<.65||std::abs(alignment)<.75)&&speed>4){Vec3 d=xp*(alignment<0?-1:1);velocity=d*(std::max(0.0,dot(velocity,d))*.8);}
  yaw=std::atan2(xp.x,xp.z);orientation=frameq(xp,n);board_normal=n;
  int spins=int(std::floor(std::abs(spin)/3.141592653589793+.5)),flips=int(std::floor(std::abs(flip)/(2*3.141592653589793)+.5));
  double points=spins*125*(1+.25*spins)+(flips?flips*700*(1+.5*(flips-1)):0);if(air_time>.9)points+=160*(air_time-.9);if(spins&&flips)points*=1.3;if(up>.94&&std::abs(alignment)>.95)points*=1.2;
  last_points=(spins||flips)&&points>=80?int(std::floor(points/5+.5))*5:0;score+=last_points;
  std::cout<<"LAND spin="<<spins*180<<" flip="<<flips<<" points="<<last_points<<" score="<<score<<'\n';
 }
};
}
