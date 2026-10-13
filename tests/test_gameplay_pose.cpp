#include "pose.hpp"
#include "gameplay_test_support.hpp"
#include "gameplay_oracle.hpp"
using namespace fl;using namespace gameplay_test;
int main(){try{
 RiderGeometry geometry;RiderMesh mesh;
 for(const auto& c:gameplay_oracle::poses){
  Game g(0x1234);g.position={};g.orientation=g.visual_orientation={};g.crouch=.65;g.lean=.2;g.steer_visual=-.3;g.grab_amount=c.grab?.8:0;g.mode=c.bail?RiderMode::Bail:c.grab||c.uber?RiderMode::Air:RiderMode::Ground;g.air.grab=c.grab;g.air.tweak=.6;if(c.uber){g.air.uber=uber_action(c.uber);g.air.uber->t=.37;g.air.uber->duration=1;}
  auto r=compute_rig(g);std::vector<double> actual;vector(actual,r.origin);quat(actual,r.orientation);vector(actual,r.pelvis);vector(actual,r.chest);vector(actual,r.neck);vector(actual,r.head);quat(actual,r.head_orientation);for(const auto& pair:{r.hip,r.knee,r.ankle,r.shoulder,r.elbow,r.hand})for(auto v:pair)vector(actual,v);vector(actual,r.board_position);quat(actual,r.board_orientation);compare(actual,c.rig,"source rig grab="+std::to_string(c.grab)+" uber="+std::to_string(c.uber));
  geometry.update(r,mesh);require(mesh.mesh.vertices.size()==size_t(c.vertices)&&mesh.mesh.indices.size()==size_t(c.indices),"original primitive topology counts");std::vector<int> groups;for(auto group:mesh.groups)for(auto value:group)groups.push_back(int(value));require(groups==c.groups,"original material/draw groups");
  for(size_t j=0;j<c.samples.size();j+=12){const auto& v=mesh.mesh.vertices[size_t(c.samples[j])];std::vector<double> sample={v.px,v.py,v.pz,v.nx,v.ny,v.nz,v.albedo,v.lip,v.stripe,v.lateral,v.relative_height};std::vector<double> expected(c.samples.begin()+j+1,c.samples.begin()+j+12);compare(sample,expected,"source primitive transformed vertex",3e-6);}
 }
 auto zero=ik2({0,0,0},{0,0,0},.44,.45,{0,0,0});require(std::isfinite(zero.mid.x)&&std::isfinite(zero.end.y),"coincident IK finite");close(length(zero.end),.011,"source minimum reach");auto reach=ik2({0,0,0},{0,-3,0},.44,.45,{0,0,1});close(length(reach.end),.889,"source maximum reach");
 Game g(1);Rng rng{1};for(int i=0;i<20;++i)g.particles.spray(40,false,g.position,g.velocity,g.visual_orientation,rng);require(g.particles.snow.size()==700,"source particle cap");g.particles.step(1.1);require(g.particles.snow.empty(),"source particle lifetime");
 for(int i=0;i<710;++i)g.particles.add_trail({0,0,double(i)},{0,1,0},{0,0,6},i/120.);
 require(g.particles.trail.size()==700,"bounded trail");g.particles.break_trail();g.particles.add_trail({0,0,720},{0,1,0},{0,0,6},6);require(g.particles.trail.back().break_before,"trail transition break");
 std::cout<<"pose: 10 grabs, all 6 ubers, bail, primitive groups and sampled vertices match source\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
