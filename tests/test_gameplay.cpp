#include "gameplay_test_support.hpp"
#include "gameplay_oracle.hpp"
#include <filesystem>
#include <chrono>
#include <fstream>
using namespace fl;using namespace gameplay_test;
int main(){try{
 for(const auto& c:gameplay_oracle::scores){
  Game g(0x1234);g.takeoff();g.spin=c.spin;g.flip=c.flip;g.air_time=c.t;g.air.switch_takeoff=c.sw;if(c.grab)g.air.grabs.push_back({c.grab,c.held,c.tweak});for(int id:c.ubers)g.air.ubers.push_back(uber_action(id));
  auto res=g.score_air(c.perfect);require(bool(res)==c.valid,"source score presence");if(res){require(res->points==c.points&&res->name==c.name,"source score/label "+c.name);auto repeated=g.score_air(c.perfect);require(repeated->points==c.repeat_points,"source repetition "+c.name);}
 }
 for(const auto& c:gameplay_oracle::ticks){
  Game g(0x1234);if(c.air){g.takeoff(true);g.position.y+=12;g.air_time=c.start_air;if(c.uber){g.air.uber=uber_action(c.uber);g.air.uber->t=.2;}}g.step({c.steer,c.drive,false,c.boost,0},c.dt);
  std::vector<double> actual;vector(actual,g.position);vector(actual,g.velocity);quat(actual,g.orientation);actual.insert(actual.end(),{g.yaw,g.crouch,g.lean,g.meter,g.airborne?g.air_time:0,g.airborne?g.spin:0,g.airborne?g.flip:0});compare(actual,c.state,"source tick");
 }
 for(const auto& c:gameplay_oracle::lands){
  Game g(0x1234);g.letters=c.letters;g.takeoff(true);g.velocity={6,0,0};g.orientation=qmul(qaxis({0,0,1},c.tilt),qaxis({0,1,0},c.yaw));g.air_time=c.t;g.tricked=c.tricked;g.spin=PI;if(c.uber>=0){g.air.uber=uber_action(4);g.air.uber->t=c.uber;g.air.uber->duration=1;}g.land({0,1,0},4);
  std::string mode=g.mode==RiderMode::Bail?"bail":g.mode==RiderMode::Ground?"ground":"other";require(mode==c.mode,"source landing mode");std::vector<double> actual;vector(actual,g.velocity);actual.insert(actual.end(),{g.yaw,double(g.fakie),double(g.combo.count),double(g.combo.sum),g.meter,double(g.letters),g.grotesk,g.uber_boost,g.slowmo,g.flash,g.shake});compare(actual,c.state,"source landing");
  require(events(g,GameEventKind::Land)==int(c.sounded),"one original land call");if(c.sounded){auto event=std::find_if(g.events.begin(),g.events.end(),[](const GameEvent& e){return e.kind==GameEventKind::Land;});close(event->amount,c.big,"Land big");require(event->flag==c.scored,"Land scored flag");}
 }
 for(const auto& c:gameplay_oracle::frames){
  Game g(0x1234);g.slowmo=c.old_slowmo;g.takeoff(true);g.velocity={6,0,0};g.orientation={};g.air_time=1;g.tricked=true;g.spin=PI;g.air.uber=uber_action(4);g.air.uber->t=.75;g.air.uber->duration=1;
  close(g.begin_frame(.05),c.scale,"source frame scale before landing");g.land({0,1,0},4);g.real_time_feedback({},.05);close(g.slowmo,c.new_slowmo,"new uber slowmo survives frame");close(g.particles.snow.front().life,c.particle_life,"particles use pre-tick scale");
 }
 for(int mask=0;mask<16;++mask){require(grab_for(mask)==gameplay_oracle::grab_masks[mask],"source grab precedence");require(uber_for(mask)==gameplay_oracle::uber_masks[mask],"source uber precedence");}
 // Source timing/transition constraints: completion edge, 0.35s air threshold,
 // combo ground clock, real-time slowmo, and safe recovery.
 Game g(0x1234);g.takeoff();g.position.y+=30;g.meter=1;g.air_time=.1;
 g.step({0,0,false,true,4},1/120.);require(g.air.uber&&g.air.uber->id==4,"uber grab edge");require(events(g,GameEventKind::Uber)==1,"single uber");close(g.meter,.35,"uber meter");
 for(int i=0;i<150;++i)g.step({0,0,false,true,4},1/120.);
 require(!g.air.uber&&g.air.ubers.size()==1,"held button cannot retrigger uber");
 g.reset();g.events.clear();g.takeoff();g.position.y+=30;g.air_time=.35;g.step({},0);require(!g.sound.sustained_air,"air exactly .35 is not sustained");g.step({},.001);require(g.sound.sustained_air,"air beyond .35 sustained");
 g.do_bail("test");require(!g.sound.sustained_air&&!g.sound.grinding&&g.sound.speed==0,"bail sound zero");for(int i=0;i<218;++i)g.step({},1/120.);require(g.mode==RiderMode::Ground,"source 1.8s recovery");
 g.add_to_combo("FS 180",155);g.add_to_combo("Indy",200);require(g.score==0,"unbanked not scored");g.combo_tick(2.21);require(g.score==710&&g.best==710&&g.best_combo==710,"source bank multiplier");g.add_to_combo("Indy",200);g.do_bail("loss");require(g.combo.count==0&&g.score==710,"bail loses unbanked only");
 g.slowmo=.45;close(g.physics_scale(),.4,"slowmo physics scale");double time=g.time;g.set_paused(true);g.begin_frame(.05);g.step({},1);g.real_time_feedback({},.05);close(g.time,time,"pause simulation");close(g.slowmo,.45,"pause slowmo clock");g.set_paused(false);g.begin_frame(.05);g.real_time_feedback({},.05);close(g.slowmo,.4,"real slowmo clock");
 auto generation=g.generation();g.restart(456);require(g.generation()==generation+1&&g.seed()==456&&g.best==710,"new seed identity and best persistence");
 // File path is integration supplied; use an app-test-owned directory only.
 const auto directory=std::filesystem::temp_directory_path()/std::filesystem::path("falllinie-gameplay-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directory(directory);auto path=(directory/"best").string();
 g.set_best_path(path);g.add_to_combo("new best",1000);g.bank_combo();require(g.persistence_error.empty(),"atomic best save");Game restored(456);restored.set_best_path(path);require(restored.best==1000,"best load");require(!std::filesystem::exists(path+".tmp"),"temp renamed");
 {std::ofstream file(path);file<<"-1";}Game malformed(456);malformed.set_best_path(path);require(malformed.best==0,"reject invalid best");restored.set_best_path((directory/"missing"/"best").string());restored.add_to_combo("write fail",1200);restored.bank_combo();require(!restored.persistence_error.empty()&&restored.score==1200,"save failure remains playable");std::filesystem::remove_all(directory);
 std::cout<<"gameplay: source score/repetition/tick/landing oracles and transition checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
