#pragma once
#include "world.hpp"
#include "quaternion.hpp"
#include "feedback.hpp"
#include "particles.hpp"
#include <optional>
#include <string>
namespace fl {
constexpr double PI=3.14159265358979323846, TAU=2*PI;
struct Controls {double steer=0,drive=0;bool jump=false,boost=false;uint8_t grabs=0;};
enum class RiderMode {Ground,Air,Rail,Bail};
struct GrabDefinition {int id;const char* name;std::optional<Vec3> left,right;double twist=0,tilt=0;};
struct UberAction {int id=0;double t=0,duration=0;std::string name;};
const GrabDefinition* grab_definition(int id);
int grab_for(int mask);int uber_for(int mask);UberAction uber_action(int id);
struct GrabTime {int id;double t=0,tweak=0;};
struct AirState {
 double peak=0,grab_time=0,tweak=0;int grab=0;bool switch_takeoff=false,ollie=false;
 std::vector<GrabTime> grabs;std::optional<UberAction> uber;std::vector<UberAction> ubers;
};
struct Combo {int count=0,sum=0;double timer=0;std::vector<std::string> names;};
struct TrickResult {std::string name;int points=0,ubers=0;};
struct HudFeedback {std::string live,result,grade,banner;double live_time=0,result_time=0,banner_time=0;};
class Game {
public:
 Terrain terrain;World world;
 Vec3 position{},velocity{},board_normal{0,1,0};
 Quat orientation{},visual_orientation{},takeoff_orientation{};
 double yaw=0,charge=0,crouch=0,distance=0,time=0;
 double air_time=0,spin=0,flip=0,spin_velocity=0,flip_velocity=0,bail_time=0;
 int score=0,last_points=0,best=0,best_combo=0,letters=0;
 bool airborne=false,tricked=false,fakie=false,boosting=false;
 double lean=0,steer_visual=0,grab_amount=0,gap=0;
 double meter=.3,grotesk=0,uber_boost=0,slowmo=0,flash=0,inversion=0,shake=0;
 RiderMode mode=RiderMode::Ground;AirState air;Combo combo;HudFeedback hud;
 std::vector<GameEvent> events;RideSound sound;Particles particles;
 std::string persistence_error;
 explicit Game(uint32_t seed);
 Game(const Game&)=delete;Game& operator=(const Game&)=delete;
 void reset();void restart(uint32_t seed);void start();void set_paused(bool on);
 bool paused()const{return paused_;}bool playing()const{return playing_;}
 uint64_t generation()const{return generation_;}uint32_t seed()const{return seed_;}
 // Caller scales its fixed-step accumulator once; step uses an unscaled 1/120 s.
 double physics_scale()const{return slowmo>0?.4:1;}
 // Once before fixed ticks: snapshot scale and decrement pre-existing slowmo.
 double begin_frame(double raw_dt);
 // Once after fixed ticks, real clock: UI, qVis, particles, flash/inversion.
 void real_time_feedback(Controls input,double raw_dt);
 void step(Controls input,double dt);
 void place(double z);void takeoff(bool ollie=false);
 void set_best_path(const std::string& path);
 // Source-shaped score helpers are also used by oracle tests.
 std::string trick_name(bool live)const;std::optional<TrickResult> score_air(bool perfect);
 void add_to_combo(const std::string& name,int points);void bank_combo();void combo_tick(double dt);
 void land(Vec3 normal,double impact);void do_bail(const std::string& reason);
private:
 uint32_t seed_;uint64_t generation_=1;bool playing_=true,paused_=false;
 double frame_scale_=1;
 bool previous_jump_=false;uint8_t previous_grabs_=0;
 Vec3 flip_axis_{1,0,0},air_normal_{0,1,0},bail_angular_{};bool air_normal_initialized_=false;
 double spin_goal_=0,flip_goal_=0,out_of_bounds_=0,stuck_=0,safe_=0,z0_=4,bail_elapsed_=0;
 struct RailState {Rail rail;double s=0,speed=0,t=0;bool parallel=true;};std::optional<RailState> rail_;
 std::vector<std::string> recent_;std::string best_path_;Rng effects_rng_;
 void reset_rider(double z);void ground_step(Controls,double,bool);void air_step(Controls,double,uint8_t);
 void resolve_ground();void try_rail();void enter_rail(const Rail&,double,double);
 void rail_step(Controls,double,bool);void hit_trees();void bail_step(double);
 void banner(const std::string&,double);void landed(const std::string&,int,const std::string&);
 void emit(GameEventKind,double=0,bool=false,int=0,const std::string& ={});
 void silence_ride();void save_best();void sync_sound();
};
// Source norm's caller-selected fallback, used only at source call sites needing it.
inline Vec3 source_norm(Vec3 a,Vec3 fallback){double l=length(a);return l>1e-9?a*(1/l):fallback;}
inline Vec3 horizontal(Vec3 a){return {a.x,0,a.z};}
inline Vec3 forward_of(double yaw){return {std::sin(yaw),0,std::cos(yaw)};}
inline Quat source_frame(Vec3 f,Vec3 n){Vec3 x=source_norm(f-n*dot(f,n),{0,0,1});return qbasis(x,n,cross(x,n));}
}
