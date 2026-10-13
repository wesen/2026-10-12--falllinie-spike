#include "runtime.hpp"
#include "gameplay_test_support.hpp"
using namespace fl;using namespace gameplay_test;
namespace {
struct Fixture {
 NativeInputMapper mapper;NativeInputSnapshot s;bool visible=false,saved=false;
 NativeActions read(bool title=false){auto a=mapper.read(s,title);if(a.dev_overlay_restore)visible=saved;if(a.dev_overlay_toggle){if(a.dev_overlay_select)saved=visible;visible=!visible;}return a;}
};
}
int main(){try{
 for(bool initial:{false,true})for(bool title:{false,true}){
  Fixture f;f.visible=initial;f.s.buttons[8]=true;auto a=f.read(title);
  require(a.dev_overlay_toggle&&!a.new_run&&f.visible!=initial,"SELECT press toggles in title/run/pause");
  for(int i=0;i<10;++i)require(!f.read(title).dev_overlay_toggle,"held SELECT no repeat");
  f.s={};f.read(title);f.s.buttons[8]=true;f.read(title);require(f.visible==initial,"next SELECT press toggles");
 }
 for(bool initial:{false,true})for(int other:{3,9})for(int order:{0,1,2}){
  Fixture f;f.visible=initial;
  if(order==1){f.s.buttons[8]=true;f.read();require(f.visible!=initial,"SELECT-first gives immediate transient feedback");}
  if(order==2){f.s.buttons[other]=true;auto a=f.read();require(!a.new_run&&!a.dev_overlay_toggle,"Y/START first not a SELECT action");}
  f.s.buttons[8]=f.s.buttons[other]=true;auto a=f.read();
  require(f.visible==initial&&!a.dev_overlay_toggle,"chord suppresses or restores SELECT effect in either order");
  require(other==9?(a.quit&&!a.new_run&&!a.pause&&!a.start):(!a.quit&&a.new_run),"quit/new-run chord precedence");
  require(!f.read().new_run,"held new-run chord not repeated");
 }
 Fixture f;f.s.buttons[8]=f.s.buttons[3]=f.s.buttons[9]=true;auto a=f.read();
 require(a.quit&&!a.new_run&&!a.dev_overlay_toggle&&!a.pause,"quit wins three-button chord");
 f={};f.s.buttons[8]=f.s.buttons[3]=true;a=f.read(true);require(!a.start&&!a.new_run&&!a.dev_overlay_toggle,"title chord does not start/reset demo");
 f={};f.s.keys[SDL_SCANCODE_F2]=1;a=f.read(true);require(a.dev_overlay_toggle&&f.visible,"F2 toggles in title");require(!f.read(true).dev_overlay_toggle,"held F2 no repeat");
 // Independent keyboard changes cancel ownership, so joining Y/START cannot
 // accidentally restore visibility from before an unrelated F2 toggle.
 for(int other:{3,9}){
  f={};f.s.buttons[8]=true;f.read();f.s.keys[SDL_SCANCODE_F2]=1;f.read();require(!f.visible,"F2 independent toggle");
  f.s.buttons[other]=true;a=f.read();require(!a.dev_overlay_restore&&!f.visible,"chord does not undo independent F2");
 }
 f={};f.visible=true;f.s.buttons[8]=true;f.read();f.s.buttons[3]=true;f.s.keys[SDL_SCANCODE_F2]=1;a=f.read();
 require(a.new_run&&a.dev_overlay_restore&&a.dev_overlay_toggle&&!f.visible,"joined chord restores before simultaneous independent F2");
 f={};f.s.buttons[8]=true;f.s.keys[SDL_SCANCODE_F2]=1;a=f.read();require(f.visible&&!a.dev_overlay_select,"simultaneous F2/SELECT is one independent toggle");f.s.buttons[3]=true;a=f.read();require(a.new_run&&!a.dev_overlay_restore&&f.visible,"simultaneous keyboard toggle not owned by SELECT");
 f={};f.s.buttons[8]=true;f.read();f.s={};f.read();f.s.buttons[3]=true;a=f.read();require(f.visible&&!a.new_run&&!a.dev_overlay_restore,"release clears press ownership");
 f={};f.s.keys[SDL_SCANCODE_R]=1;a=f.read();require(a.new_run&&!a.dev_overlay_toggle,"R new run only");require(!f.read().new_run,"R edge only");
 f={};f.s.keys[SDL_SCANCODE_F3]=1;a=f.read();require(a.debug&&!a.dev_overlay_toggle,"F3 diagnostics preserved");
 f={};f.s.buttons[9]=true;a=f.read();require(a.pause&&!a.dev_overlay_toggle&&!a.new_run,"START pause preserved");
 f={};f.s.buttons[9]=true;require(f.read(true).start,"START title start preserved");
 f={};f.s.buttons[0]=f.s.buttons[1]=f.s.buttons[2]=true;for(int i=4;i<8;++i)f.s.buttons[i]=true;
 a=f.read();require(a.ride.jump&&a.ride.boost&&a.ride.grabs==15&&!a.dev_overlay_toggle,"jump boost grabs preserved");
 Game g(42);g.enter_title();f={};f.s.buttons[8]=true;f.read(g.title());require(g.title(),"overlay does not start simulation");
 g.start();g.set_paused(true);double time=g.time;auto generation=g.generation();f={};f.s.keys[SDL_SCANCODE_F2]=1;a=f.read(g.title());require(a.dev_overlay_toggle&&!a.pause&&g.paused()&&g.time==time&&g.generation()==generation,"pause overlay has no game side effect");
 std::cout<<"developer controls: press edges, chord order/rollback/precedence, title/pause and preserved controls passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
