#include "game.hpp"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <limits>
namespace fl {
namespace {
const GrabDefinition grabs[]={
 {1,"Indy",{},Vec3{-.05,.03,.16}}, {2,"Mute",Vec3{.08,.03,.16},{}},
 {4,"Method",Vec3{.04,.03,-.16},{},-.7,.2}, {8,"Stalefish",{},Vec3{-.08,.03,-.16},-.4,-.2},
 {3,"Tail",{},Vec3{-.72,.05,0},0,-.5}, {12,"Nose",Vec3{.72,.05,0},{},0,.5},
 {5,"Japan",Vec3{.2,.03,.16},{},.6,.3}, {10,"Crail",{},Vec3{.55,.05,.12},.2,.45},
 {9,"Melon",Vec3{-.02,.03,-.16},{},-.5,-.3}, {6,"Rocket",Vec3{.7,.05,.05},Vec3{.66,.05,-.05},0,.9}
};
const double NaN=std::numeric_limits<double>::quiet_NaN();
int rounded(double x){return int(std::floor(x+.5));}
std::string fixed1(double x){std::ostringstream s;s<<std::fixed<<std::setprecision(1)<<x;return s.str();}
void append(std::string& s,const std::string& part,const char* delimiter=" · "){if(part.empty())return;if(!s.empty())s+=delimiter;s+=part;}
}
const GrabDefinition* grab_definition(int id){for(const auto& g:grabs)if(g.id==id)return &g;return nullptr;}
int grab_for(int mask){if(grab_definition(mask))return mask;for(int k:{3,12,5,10,9,6,1,2,4,8})if((mask&k)==k)return k;return 0;}
UberAction uber_action(int id){switch(id){case 1:return {1,0,1.5,"Helikopter"};case 2:return {2,0,1.35,"Superman Deck"};case 4:return {4,0,1.1,"Kickflip ×2"};case 8:return {8,0,1.3,"Air Split"};case 3:return {3,0,1.5,"Moonwalk"};case 12:return {12,0,1.5,"Orbit"};default:return {};}}
int uber_for(int mask){if(uber_action(mask).id)return mask;for(int k:{3,12,1,2,4,8})if((mask&k)==k)return k;return 0;}
Game::Game(uint32_t seed):terrain(seed),world(terrain,seed),seed_(seed),effects_rng_{seed^0xaba17835u}{events.reserve(32);reset();events.clear();}
void Game::emit(GameEventKind kind,double amount,bool flag,int points,const std::string& text){events.push_back({kind,amount,flag,points,text});}
void Game::silence_ride(){sound={};}
void Game::reset_rider(double z){
 position={Terrain::center(z),terrain.height(Terrain::center(z),z),z};
 yaw=std::atan2(55./310*std::cos(z/310)+22./127*std::cos(z/127+2.1)+8./53*std::cos(z/53+.4),1);
 board_normal=terrain.normal(position.x,z);Vec3 f=forward_of(yaw);velocity=norm(f-board_normal*dot(f,board_normal))*6;
 orientation=source_frame(f,board_normal);visual_orientation=orientation;mode=RiderMode::Ground;airborne=false;air={};rail_.reset();fakie=false;crouch=charge=bail_time=0;
}
void Game::reset(){
 if(sound.grinding)emit(GameEventKind::GrindOff);
 if(grotesk>0)emit(GameEventKind::GroteskOff);
 reset_rider(4);score=0;distance=0;meter=.3;letters=0;grotesk=uber_boost=0;combo={};recent_.clear();z0_=position.z;particles.clear();silence_ride();
 // Source newRun keeps input edges, slowmo/flash/shake, HUD and recovery timers.
 // In particular a held grab must not manufacture a new uber edge on restart.
 emit(GameEventKind::Restart);
}
void Game::restart(uint32_t seed){++generation_;seed_=seed;terrain=Terrain(seed);world.reset(seed);reset();paused_=false;playing_=true;}
void Game::start(){restart(seed_);}
void Game::enter_title(){playing_=false;paused_=false;}
void Game::set_paused(bool on){paused_=on;if(on)silence_ride();}
void Game::place(double z){
 if(sound.grinding)emit(GameEventKind::GrindOff);
 auto p=terrain.params(z);double u=clamp(position.x-Terrain::center(z),-(p.W/2-4),p.W/2-4);int c=int(std::floor(z/CELL));
 for(int k=c-1;k<=c+1;++k)for(const auto& t:world.trees(k))if(std::abs(t.z-z)<6&&std::abs(t.x-Terrain::center(z)-u)<3)u+=u>0?-5:5;
 // Source placeOnCourse is not resetRider: retain charge/crouch/grab amount,
 // input edges and boost transient through automatic recovery.
 position.x=Terrain::center(z)+u;position.z=z;position.y=terrain.height(position.x,z);
 yaw=std::atan2(55./310*std::cos(z/310)+22./127*std::cos(z/127+2.1)+8./53*std::cos(z/53+.4),1);
 board_normal=terrain.normal(position.x,z);Vec3 f=forward_of(yaw);orientation=source_frame(f,board_normal);visual_orientation=orientation;velocity=norm(f-board_normal*dot(f,board_normal))*6;
 mode=RiderMode::Ground;airborne=false;air={};rail_.reset();fakie=false;bail_time=0;
 out_of_bounds_=stuck_=0;safe_=1.2;particles.break_trail();silence_ride();emit(GameEventKind::Recovery);
}
void Game::takeoff(bool ollie){
 mode=RiderMode::Air;airborne=true;particles.break_trail();sound.carve=0;air_time=spin=flip=spin_velocity=flip_velocity=0;tricked=false;spin_goal_=flip_goal_=NaN;takeoff_orientation=orientation;
 Vec3 vh=source_norm(horizontal(velocity),forward_of(yaw));flip_axis_=source_norm(cross({0,1,0},vh),{1,0,0});air_normal_={0,1,0};air_normal_initialized_=false;
 air={};air.switch_takeoff=fakie;air.ollie=ollie;air.peak=position.y;
}
void Game::step(Controls input,double dt){
 if(paused_)return;
 time+=dt;if(grotesk>0){grotesk-=dt;if(grotesk<=0){letters=0;banner("Grotesk off",1.2);emit(GameEventKind::GroteskOff);}}
 if(uber_boost>0)uber_boost-=dt;
 bool release=!input.jump&&previous_jump_;uint8_t grab_edge=input.grabs&~previous_grabs_;previous_jump_=input.jump;previous_grabs_=input.grabs;
 if(mode==RiderMode::Bail){bail_step(dt);combo_tick(dt);sync_sound();return;}
 if(mode==RiderMode::Rail){rail_step(input,dt,release);combo_tick(dt);sync_sound();return;}
 if(mode==RiderMode::Ground)ground_step(input,dt,release);else air_step(input,dt,grab_edge);
 if(mode==RiderMode::Rail||mode==RiderMode::Bail){sync_sound();return;}
 position=position+velocity*dt;resolve_ground();
 if(mode!=RiderMode::Bail){try_rail();if(safe_<=0)hit_trees();}
 distance=std::max(distance,position.z-z0_);safe_-=dt;auto p=terrain.params(position.z);
 out_of_bounds_=std::abs(position.x-Terrain::center(position.z))>p.W/2+p.R+16?out_of_bounds_+dt:0;
 stuck_=mode==RiderMode::Ground&&length(velocity)<1.5?stuck_+dt:0;
 if(out_of_bounds_>1.2||stuck_>3){std::string why=out_of_bounds_>1.2?"Out of bounds":"Back on course";place(position.z+4);banner(why,1.2);}
 combo_tick(dt);sync_sound();
}
void Game::ground_step(Controls input,double dt,bool release){
 Vec3 n=terrain.normal(position.x,position.z);board_normal=n;
 if(position.y+velocity.y*dt-.5*1.6*13*dt*dt>terrain.height(position.x+velocity.x*dt,position.z+velocity.z*dt)+.002){takeoff(false);return;}
 Vec3 f=source_norm(forward_of(yaw)-n*dot(forward_of(yaw),n),{0,0,1}),s=cross(n,f),g{0,-13,0};
 velocity=velocity+(g-n*dot(g,n))*dt;velocity=velocity-n*dot(velocity,n);
 double vf=dot(velocity,f),vs=dot(velocity,s),sp0=std::hypot(vf,vs),brake=input.drive<-.2?-input.drive:0,tuck=input.drive>.2?input.drive:0;
 double nvs=vs*std::exp(-(7-3*brake)*dt),sp1=std::hypot(vf,nvs),target=lerp(sp1,sp0,.9-.55*brake);
 if(sp1>1e-5){double k=target/sp1;vf*=k;vs=nvs*k;}
 double sp=std::hypot(vf,vs),dec=.55+(.0042-.0016*tuck)*sp*sp+brake*9;
 bool infinite=grotesk>0||uber_boost>0;boosting=input.boost&&(meter>.01||infinite)&&sp>1;double acc=0;
 if(boosting){acc=11*std::max(0.,1-sp/44);if(!infinite)meter=std::max(0.,meter-.2*dt);}
 double next=std::max(0.,sp-dec*dt)+acc*dt;if(sp>1e-4){vf*=next/sp;vs*=next/sp;}else if(acc>0)vf=acc*dt;
 velocity=f*vf+s*vs;fakie=vf<-.5?true:vf>.5?false:fakie;
 yaw-=input.steer*lerp(2.5,1.55,clamp(sp/30,0,1))*dt;
 if(input.jump)charge=std::min(1.,charge+dt/.45);
 crouch=lerp(crouch,input.jump?.35+.65*charge:tuck*.55+brake*.2,1-std::exp(-dt*12));
 Vec3 t=source_norm(horizontal(velocity),forward_of(yaw)),right=cross(t,{0,1,0}),zw=qrot(orientation,{0,0,1});
 lean=lerp(lean,dot(right*input.steer,zw)*.5*smooth(2,14,sp),1-std::exp(-dt*8));orientation=source_frame(forward_of(yaw),n);
 if(release){velocity=velocity+n*(4.2+3.6*charge);charge=0;takeoff(true);emit(GameEventKind::Pop);}
 sound.carve=clamp(sp/28,0,1)*(.4+std::abs(input.steer)*.6+brake);
 if(sp>4&&effects_rng_.next()<dt*(sp*2+std::abs(input.steer)*60+brake*80))particles.spray(1,false,position,velocity,visual_orientation,effects_rng_);
 particles.add_trail(position,board_normal,velocity,time);
}
void Game::resolve_ground(){
 double h=terrain.height(position.x,position.z);gap=position.y-h;
 if(mode==RiderMode::Air&&gap<=0){Vec3 n=terrain.normal(position.x,position.z);position.y=h;double vn=dot(velocity,n),impact=-vn;if(vn<0)velocity=velocity-n*vn;land(n,impact);return;}
 if(mode==RiderMode::Ground){Vec3 n=terrain.normal(position.x,position.z);if(gap<.5){position.y=h;velocity=velocity-n*dot(velocity,n);board_normal=n;}else takeoff(false);}
}
void Game::air_step(Controls input,double dt,uint8_t edge){
 air_time+=dt;velocity.y-=13*dt;double sp=length(velocity);velocity=velocity*(1-.0006*sp*dt);air.peak=std::max(air.peak,position.y);
 bool locked=air.uber&&air.uber->id==2;
 if(std::abs(input.steer)>.3){spin_goal_=NaN;spin_velocity+=(-std::copysign(7.2,input.steer)-spin_velocity)*std::min(1.,dt*9);tricked=true;}
 else {if(std::isnan(spin_goal_)){double k=spin/PI,fr=k-std::floor(k);spin_goal_=(spin_velocity>.5&&fr>.25?std::ceil(k):spin_velocity<-.5&&fr<.75?std::floor(k):std::floor(k+.5))*PI;}spin_velocity+=(clamp((spin_goal_-spin)*7,-7,7)-spin_velocity)*std::min(1.,dt*10);}
 if(!locked&&std::abs(input.drive)>.3&&air_time>.08){flip_goal_=NaN;flip_velocity+=(std::copysign(6.,input.drive)-flip_velocity)*std::min(1.,dt*8);tricked=true;}
 else {if(std::isnan(flip_goal_)){double k=flip/TAU,fr=k-std::floor(k);flip_goal_=(flip_velocity>.5&&fr>.22?std::ceil(k):flip_velocity<-.5&&fr<.78?std::floor(k):std::floor(k+.5))*TAU;}flip_velocity+=(clamp((flip_goal_-flip)*6,-6,6)-flip_velocity)*std::min(1.,dt*9);}
 spin+=spin_velocity*dt;flip+=flip_velocity*dt;
 bool infinite=grotesk>0||uber_boost>0,ready=meter>=.999||infinite;
 if(air.uber){air.uber->t+=dt;if(air.uber->t>=air.uber->duration){air.ubers.push_back(*air.uber);air.uber.reset();}}
 else if(input.boost&&edge&&ready&&air_time>.05){int id=uber_for(input.grabs);if(id){air.uber=uber_action(id);air.grab=0;tricked=true;emit(GameEventKind::Uber);if(!infinite)meter=.35;}}
 if(!air.uber){int id=grab_for(input.grabs);air.grab=id;if(id){auto e=std::find_if(air.grabs.begin(),air.grabs.end(),[id](const GrabTime& g){return g.id==id;});if(e==air.grabs.end()){air.grabs.push_back({id,0,0});e=air.grabs.end()-1;}e->t+=dt;if(input.boost)e->tweak+=dt;tricked=true;}air.tweak=lerp(air.tweak,id&&input.boost?1:0,1-std::exp(-dt*10));}
 grab_amount=lerp(grab_amount,air.grab&&!air.uber?1:0,1-std::exp(-dt*14));
 double agl=position.y-terrain.height(position.x,position.z);Vec3 nb=agl<6?terrain.normal(position.x+velocity.x*.15,position.z+velocity.z*.15):Vec3{0,1,0};
 if(air_normal_initialized_)air_normal_=norm(air_normal_+(nb-air_normal_)*(1-std::exp(-dt*6)));else {air_normal_={0,1,0};air_normal_initialized_=true;}
 Quat up=source_frame(source_norm(horizontal(qrot(takeoff_orientation,{1,0,0})),{0,0,1}),air_normal_),base=qslerp(takeoff_orientation,up,1-std::exp(-air_time*2.5));
 orientation=qnorm(qmul(qaxis({0,1,0},spin),qmul(qaxis(flip_axis_,flip),base)));crouch=lerp(crouch,.3+.6*grab_amount,1-std::exp(-dt*10));
 sound.wind=clamp(sp/30,0,1);if(playing_){hud.live=trick_name(true);hud.live_time=.12;}
}
std::string Game::trick_name(bool live)const{
 int sn=rounded(std::abs(spin)/PI),fn=rounded(std::abs(flip)/TAU),fl=live?int(std::floor(std::abs(flip)/TAU+.15)):fn;
 int deg=live?int(std::floor(std::abs(spin)/PI+.12))*180:sn*180;std::string dir=spin>0?"FS":"BS",main;
 if(air.switch_takeoff&&(deg||fl))append(main,deg&&!fl?"Cab":"Switch"," ");
 const char* multiple[]={"","","Double","Triple","Quad"};
 if(fl&&deg>=180){std::string p=dir+" "+(flip<0?"Rodeo":"Misty");if(fl>1)p+=" "+std::string(multiple[std::min(fl,4)]);p+=" "+std::to_string(deg);append(main,p," ");}
 else if(fl)append(main,(fl>1?std::string(multiple[std::min(fl,4)])+" ":"")+(flip<0?"Backflip":"Frontflip")," ");
 else if(deg)append(main,dir+" "+std::to_string(deg)," ");
 for(const auto& g:air.grabs)if(g.t>.12)append(main,grab_definition(g.id)->name);
 for(const auto& u:air.ubers)append(main,u.name);
 if(air.uber)append(main,air.uber->name);
 return main;
}
std::optional<TrickResult> Game::score_air(bool perfect){
 int sn=rounded(std::abs(spin)/PI),fn=rounded(std::abs(flip)/TAU);double points=sn*125*(1+.25*sn)+(fn?fn*700*(1+.5*(fn-1)):0);
 for(const auto& g:air.grabs)if(g.t>.12)points+=(200+450*g.t)*(g.tweak>.2?1.5:1);
 for(const auto& u:air.ubers)points+=3000+1200*u.duration;
 if(air_time>.9)points+=160*(air_time-.9);
 if(sn&&fn)points*=1.3;
 if(perfect)points*=1.2;
 std::string label=trick_name(false);if(label.empty()||points<80)return {};
 int repeat=int(std::count(recent_.begin(),recent_.end(),label));points*=std::pow(.6,repeat);recent_.push_back(label);if(recent_.size()>10)recent_.erase(recent_.begin());
 return TrickResult{label,rounded(points/5)*5,int(air.ubers.size())};
}
void Game::land(Vec3 n,double impact){
 mode=RiderMode::Ground;airborne=false;board_normal=n;Vec3 x=qrot(orientation,{1,0,0}),y=qrot(orientation,{0,1,0}),vh=source_norm(velocity-n*dot(velocity,n),x),xp=source_norm(x-n*dot(x,n),vh);
 double up=dot(y,n),alignment=dot(xp,vh),sp=length(velocity);sound.sustained_air=false;sound.air_time=sound.wind=0;double big=clamp((air_time-.4)/2,0,1);
 if(air_time<.3&&!tricked){yaw=std::atan2(xp.x,xp.z);orientation=source_frame(xp,n);air={};return;}
 bool unfinished=air.uber&&air.uber->t<air.uber->duration*.75;
 if(air.uber&&!unfinished){air.ubers.push_back(*air.uber);air.uber.reset();}
 if(up<.28||(sp>4&&std::abs(alignment)<.36)||unfinished){do_bail(unfinished?"Über not finished":up<.28?"Landed upside down":"Landed sideways");return;}
 bool sketchy=up<.65||std::abs(alignment)<.75;if(sketchy&&sp>4){Vec3 d=xp*(alignment<0?-1:1);velocity=d*(std::max(0.,dot(velocity,d))*.8);}
 yaw=std::atan2(xp.x,xp.z);fakie=alignment<0;orientation=source_frame(xp,n);bool perfect=up>.94&&std::abs(alignment)>.95;auto res=score_air(perfect);
 shake=std::min(1.,.25+impact*.05);particles.spray(rounded(10+impact*3+big*20),false,position,velocity,visual_orientation,effects_rng_);emit(GameEventKind::Land,big,bool(res));last_points=res?res->points:0;
 if(res){add_to_combo(res->name,res->points);landed(res->name,res->points,perfect?"Perfect":sketchy?"Sketchy":"Clean");if(res->ubers){slowmo=.45;flash=.12;uber_boost=5;letters=std::min(7,letters+res->ubers);if(letters>=7&&grotesk<=0){grotesk=30;banner("Grotesk",1.8);emit(GameEventKind::GroteskOn);}}}
 air={};
}
void Game::add_to_combo(const std::string& name,int points){++combo.count;combo.sum+=points;combo.timer=2.2;combo.names.push_back(name);meter=std::min(1.,meter+points/7000.);emit(GameEventKind::ComboAdded,0,false,points,name);}
void Game::bank_combo(){if(!combo.count)return;int total=rounded(combo.sum*std::min(10,combo.count));score+=total;best_combo=std::max(best_combo,total);if(score>best){best=score;save_best();}if(combo.count>1||total>1500)banner("+"+std::to_string(total),1);emit(GameEventKind::ComboBanked,combo.count,false,total);combo={};}
void Game::combo_tick(double dt){if(combo.count&&mode==RiderMode::Ground){combo.timer-=dt;if(combo.timer<=0)bank_combo();}}
void Game::try_rail(){
 int c=int(std::floor(position.z/CELL));double sp=length(velocity);if(sp<3)return;
 for(int k=c-1;k<=c+1;++k)for(const auto& r:world.rails(k)){Vec3 w=position-r.a;double s=dot(w,r.d);if(s<.3||s>r.length-.3)continue;Vec3 q=r.a+r.d*s,lat=horizontal(position-q);double dy=position.y-q.y,lh=std::hypot(lat.x,lat.z);if(lh>.85||dy<-.5||dy>.45)continue;double along=dot(norm(velocity),r.d);if(std::abs(along)<.45)continue;if(mode==RiderMode::Air&&velocity.y>2.5)continue;enter_rail(r,s,along);return;}
}
void Game::enter_rail(const Rail& rail,double s,double /*along*/){
 if(mode==RiderMode::Air){auto res=score_air(false);if(res)add_to_combo(res->name,res->points);}
 Vec3 x=qrot(orientation,{1,0,0});bool par=std::abs(dot(norm(horizontal(x)),norm(horizontal(rail.d))))>.6;
 mode=RiderMode::Rail;airborne=false;air={};particles.break_trail();rail_=RailState{rail,s,dot(velocity,rail.d),0,par};sound.sustained_air=false;sound.air_time=0;sound.grinding=true;emit(GameEventKind::GrindOn);particles.spray(8,false,position,velocity,visual_orientation,effects_rng_);
}
void Game::rail_step(Controls input,double dt,bool release){
 auto& g=*rail_;Vec3 d=g.rail.d;auto sign=[](double x){return x>0?1.:x<0?-1.:0.;};g.speed+=-13*d.y*dt;g.speed-=sign(g.speed)*.8*dt;bool infinite=grotesk>0||uber_boost>0;
 if(input.boost&&(meter>.01||infinite)){g.speed+=sign(g.speed)*6*dt;if(!infinite)meter=std::max(0.,meter-.2*dt);}
 g.s+=g.speed*dt;g.t+=dt;position=g.rail.a+d*g.s;velocity=d*g.speed;Vec3 travel=d*(sign(g.speed)==0?1:sign(g.speed)),side=norm(cross(travel,{0,1,0}));orientation=source_frame(g.parallel?travel:side,{0,1,0});crouch=lerp(crouch,.4,1-std::exp(-dt*8));grab_amount=0;
 std::string style=g.parallel?"50-50":"Boardslide";if(playing_){hud.live=style+" "+fixed1(g.t)+" s";hud.live_time=.12;}combo.timer=std::max(combo.timer,1.2);
 if(effects_rng_.next()<dt*30)particles.spray(1,true,position,velocity,visual_orientation,effects_rng_);
 if(g.s<0||g.s>g.rail.length||release){int pts=rounded(350*g.t/5)*5;if(pts>50){add_to_combo(style,pts);landed(style+" "+fixed1(g.t)+" s",pts,"Rail");last_points=pts;}mode=RiderMode::Ground;rail_.reset();sound.grinding=false;emit(GameEventKind::GrindOff);if(release)velocity.y+=6.5;position.y+=.05;velocity.y+=.8;takeoff(true);}
}
void Game::hit_trees(){int c=int(std::floor(position.z/CELL));for(int k=c-1;k<=c+1;++k)for(const auto& t:world.trees(k)){double dx=position.x-t.x,dz=position.z-t.z,r=t.radius*.35+.25;if(dx*dx+dz*dz<r*r&&position.y<t.y+t.height*.85){velocity=velocity*.15;do_bail("Hit a tree");return;}}}
void Game::do_bail(const std::string& reason){
 if(sound.grinding)emit(GameEventKind::GrindOff);
 mode=RiderMode::Bail;airborne=false;air={};rail_.reset();particles.break_trail();bail_elapsed_=0;bail_time=1.8;
 bail_angular_={(effects_rng_.next()-.5)*12,(effects_rng_.next()-.5)*6,6+effects_rng_.next()*6};combo={};if(playing_){last_points=0;hud.result=reason;hud.grade="Bail";hud.result_time=1.6;hud.live_time=0;}banner("Bail",1.1);emit(GameEventKind::Bail,0,false,0,reason);particles.spray(40,false,position,velocity,visual_orientation,effects_rng_);shake=1;grab_amount=0;silence_ride();
}
void Game::bail_step(double dt){
 bail_elapsed_+=dt;bail_time=std::max(0.,1.8-bail_elapsed_);velocity.y-=13*dt;position=position+velocity*dt;double h=terrain.height(position.x,position.z);
 if(position.y<=h){Vec3 n=terrain.normal(position.x,position.z);position.y=h;double vn=dot(velocity,n);if(vn<0)velocity=velocity-n*(vn*1.3);velocity=velocity*std::exp(-1.8*dt);if(effects_rng_.next()<dt*20)particles.spray(1,false,position,velocity,visual_orientation,effects_rng_);}
 bail_angular_=bail_angular_*std::exp(-1.2*dt);double wl=length(bail_angular_);if(wl>1e-3)orientation=qnorm(qmul(qaxis(bail_angular_,wl*dt),orientation));if(bail_elapsed_>1.8)place(position.z);
}
void Game::banner(const std::string& text,double duration){if(!playing_)return;hud.banner=text;hud.banner_time=duration;}
void Game::landed(const std::string& name,int points,const std::string& grade){if(!playing_)return;hud.result=name;hud.grade=grade;hud.result_time=1.7;hud.live_time=0;last_points=points;}
void Game::sync_sound(){
 if(paused_||mode==RiderMode::Bail){silence_ride();sound.grotesk=grotesk>0;return;}
 sound.speed=length(velocity);sound.air_time=mode==RiderMode::Air?air_time:0;sound.sustained_air=mode==RiderMode::Air&&air_time>.35;sound.grinding=mode==RiderMode::Rail;sound.grotesk=grotesk>0;
 if(mode!=RiderMode::Ground)sound.carve=0;
 if(mode!=RiderMode::Air)sound.wind=0;
}
double Game::begin_frame(double raw){
 frame_scale_=physics_scale();if(!paused_)slowmo-=clamp(raw,0,.05);
 return frame_scale_;
}
void Game::real_time_feedback(Controls input,double raw){
 raw=clamp(raw,0,.05);if(!paused_)particles.step(raw*frame_scale_);
 steer_visual=lerp(steer_visual,input.steer,1-std::exp(-raw*8));visual_orientation=qslerp(visual_orientation,orientation,1-std::exp(-raw*(mode==RiderMode::Ground?14:40)));
 flash=std::max(0.,flash-raw);inversion=clamp(inversion+(grotesk>0?raw:-raw)/.6,0,1);shake=std::max(0.,shake-raw*2.2);
 if(playing_){hud.live_time-=raw;hud.result_time-=raw;hud.banner_time-=raw;}sync_sound();
}
void Game::set_best_path(const std::string& path){best_path_=path;persistence_error.clear();if(path.empty())return;std::ifstream f(path);long long value=0;if(f>>value&&value>=0&&value<=std::numeric_limits<int>::max())best=std::max(best,int(value));}
void Game::save_best(){
 if(best_path_.empty())return;
 std::string tmp=best_path_+".tmp";std::ofstream file(tmp,std::ios::trunc);file<<best<<'\n';file.close();if(!file){persistence_error="Cannot write best score: "+tmp;return;}
 std::error_code error;std::filesystem::rename(tmp,best_path_,error);if(error)persistence_error="Cannot replace best score: "+error.message();else persistence_error.clear();
}
}
