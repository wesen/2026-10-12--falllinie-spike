#include "gameplay_test_support.hpp"
#include "gameplay_oracle.hpp"
using namespace fl;using namespace gameplay_test;
int main(){try{
 Game g(0x1234);
 for(const auto& c:gameplay_oracle::worlds){
  const auto& rails=g.world.rails(c.cell);const auto& trees=g.world.trees(c.cell);require(rails.size()==c.rails.size()&&trees.size()==c.trees.size(),"source object counts");
  for(size_t i=0;i<rails.size();++i){std::vector<double> actual;vector(actual,rails[i].a);vector(actual,rails[i].b);vector(actual,rails[i].d);actual.push_back(rails[i].length);compare(actual,c.rails[i],"source rail definition");}
  for(size_t i=0;i<trees.size();++i){auto t=trees[i];compare({t.x,t.z,t.y,t.height,t.radius,double(t.grey),double(t.hazard)},c.trees[i],"source tree definition");}
  auto mesh=g.world.objects(c.cell);require(!mesh.vertices.empty()&&!mesh.indices.empty(),"source objects drawn");for(uint32_t i:mesh.indices)require(i<mesh.vertices.size(),"object indices valid");
 }
 // Contact source rails using their shared definitions, not test-only objects.
 Rail rail{};bool found=false;for(int c=1;c<10&&!found;++c)if(!g.world.rails(c).empty()){rail=g.world.rails(c).front();found=true;}require(found,"fixture rail exists");
 for(const auto& c:gameplay_oracle::rails){
  Game ride(0x1234);ride.position=rail.a+rail.d*(c.end?rail.length-.5:2);ride.velocity=rail.d*c.speed;ride.orientation=source_frame(c.parallel?rail.d:norm(cross(rail.d,{0,1,0})),{0,1,0});ride.takeoff(false);ride.air_time=.4;ride.step({0,0,c.release,false,0},0);ride.step({0,0,false,c.boost,0},c.dt);
  require((ride.mode==RiderMode::Rail?"rail":ride.mode==RiderMode::Air?"air":"other")==c.mode,"source rail mode");std::vector<double> state;vector(state,ride.position);vector(state,ride.velocity);quat(state,ride.orientation);state.insert(state.end(),{ride.crouch,ride.meter,double(ride.combo.count),double(ride.combo.sum),ride.combo.timer,ride.mode==RiderMode::Air?ride.air_time:0});compare(state,c.state,"source rail slide/pop/score");
 }
 g.position=rail.a+rail.d*2;g.velocity=rail.d*8;g.orientation=source_frame(rail.d,{0,1,0});g.takeoff();g.air_time=.4;g.events.clear();g.step({},1/120.);require(g.mode==RiderMode::Rail&&g.sound.grinding,"source capture");require(events(g,GameEventKind::GrindOn)==1,"one grind start");
 for(int i=0;i<40;++i)g.step({0,0,true},1/120.);
 g.step({},1/120.);require(g.mode==RiderMode::Air&&!g.sound.grinding,"rail jump release exits");require(events(g,GameEventKind::GrindOff)==1&&g.combo.sum>=115,"rail 350 points/second");
 // An active rail owns its definition even when caches are retired.
 g.position=rail.a+rail.d*2;g.velocity=rail.d*8;g.takeoff();g.step({},1/120.);for(int c=20;c<325;++c)g.world.rails(c);g.step({},1/120.);require(g.mode==RiderMode::Rail,"owned rail survives definition cache trim");
 // Force original 500-cell terrain FIFO eviction, including the feature
 // vector used by a subsequent nested railsOf height query. Regeneration must
 // be pure and cumulative cliff history must remain intact.
 Terrain fresh(0x1234);double before=fresh.height(0,45000);for(int c=500;c<1100;++c){g.terrain.features(c);g.terrain.params(c*CELL);}
 require(g.terrain.feature_cache_size()<=500&&g.terrain.parameter_cache_size()<=500,"bounded original terrain caches");close(g.terrain.height(0,45000),before,"cumulative height survives definition trim");
 World regenerated(g.terrain,0x1234),pristine(fresh,0x1234);
 for(int cell:{2,1500,1501}){auto again=regenerated.rails(cell),expected=pristine.rails(cell);require(again.size()==expected.size(),"evicted rail definition regenerates");for(size_t i=0;i<again.size();++i){close(again[i].length,expected[i].length,"nested query rail clearance after trim");close(again[i].a.y,expected[i].a.y,"nested query rail height after trim");}}
 g.reset();const Tree tree=g.world.trees(0).front();g.position={tree.x,tree.y+.1,tree.z};g.velocity={0,0,6};g.takeoff();g.step({},1/120.);require(g.mode==RiderMode::Bail&&g.hud.result=="Hit a tree","source trunk collision");
 g.place(tree.z);require(g.mode==RiderMode::Ground,"safe course recovery");g.position={tree.x,tree.y+.1,tree.z};g.takeoff();g.step({},1/120.);require(g.mode!=RiderMode::Bail,"1.2s safe recovery period");
 std::cout<<"world: seeded source definitions, rail capture/pop/cache ownership, tree/safe collision passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
