#include "particles.hpp"
#include "world.hpp"
namespace fl {
void Particles::clear(){snow.clear();trail.clear();last_.reset();break_next_=false;}
void Particles::spray(int n,bool sparks,Vec3 p,Vec3 v,Quat q,Rng& rng){
 Vec3 x=qrot(q,{1,0,0}),hv{v.x,0,v.z};double l=length(hv);Vec3 back=(l>1e-9?hv*(1/l):Vec3{0,0,1})*-1;
 for(int i=0;i<n&&snow.size()<700;++i){Vec3 at=p+x*((rng.next()-.5)*1.2);double speed=sparks?.8:.35+rng.next()*.3;Vec3 vv=v*speed+Vec3{(rng.next()-.5)*3,1+rng.next()*3,(rng.next()-.5)*3};double max=.4+rng.next()*.6,size=(sparks?.05:.08)+rng.next()*.16;snow.push_back({at+back*.2,vv+back*1.5,0,max,size});}
}
void Particles::step(double dt){for(size_t i=0;i<snow.size();){auto& q=snow[i];q.life+=dt;if(q.life>q.max){snow.erase(snow.begin()+i);continue;}q.v.y-=9*dt;q.v=q.v*std::exp(-2.2*dt);q.p=q.p+q.v*dt;++i;}}
void Particles::add_trail(Vec3 p,Vec3 n,Vec3 v,double time){Vec3 c=p+n*.04;if(last_&&length(c-*last_)<.35)return;Vec3 hv{v.x,0,v.z};double l=length(hv);Vec3 side=norm(cross(n,l>1e-9?hv*(1/l):Vec3{0,0,1}));if(trail.size()>=700)trail.erase(trail.begin());trail.push_back({c,side,time,break_next_});last_=c;break_next_=false;}
void Particles::break_trail(){last_.reset();break_next_=true;}
CpuMesh Particles::snow_mesh(Vec3 eye,Vec3 right,Vec3 up)const{MeshBuilder mb;mb.mesh.vertices.reserve(snow.size()*4);mb.mesh.indices.reserve(snow.size()*6);for(const auto& q:snow){double s=std::min(q.size,.17)*(1-q.life/q.max*.5);Vec3 n=norm(eye-q.p);auto a=mb.vertex(q.p-right*s-up*s,n,1,0,0,-1,-1),b=mb.vertex(q.p+right*s-up*s,n,1,0,0,1,-1),c=mb.vertex(q.p+right*s+up*s,n,1,0,0,1,1),d=mb.vertex(q.p-right*s+up*s,n,1,0,0,-1,1);mb.quad(a,b,c,d);}return std::move(mb.mesh);}
CpuMesh Particles::trail_mesh(double time)const{MeshBuilder mb;int prev=-1;mb.mesh.vertices.reserve(trail.size()*2);mb.mesh.indices.reserve(trail.size()*6);for(const auto& t:trail){if(t.break_before)prev=-1;double al=clamp(.74+(time-t.time)*.03,.74,.97);auto a=mb.vertex(t.p+t.side*.11,{0,1,0},al),b=mb.vertex(t.p-t.side*.11,{0,1,0},al);if(prev>=0)mb.quad(prev,prev+1,b,a);prev=int(a);}return std::move(mb.mesh);}
}
