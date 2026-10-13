#include "developer_overlay.hpp"
#include "gameplay_test_support.hpp"
#include <limits>
using namespace fl;using namespace gameplay_test;
int main(){try{
 DeveloperOverlaySnapshot s;auto lines=format_developer_overlay(s);
 auto contains=[&](const std::string& needle){for(const auto& l:lines)if(l.find(needle)!=std::string::npos)return true;return false;};
 require(contains("Previous completed frame n/a")&&contains("Audio n/a")&&contains("GPU elapsed n/a")&&contains("FPS n/a")&&contains("Queue n/a"),"unmeasured values explicit n/a");
 s.frame={true,true,20,1,2,3,1,4,5,6,7,8,2,.4};
 s.title=true;s.position={1,2,3};s.mode=RiderMode::Air;s.seed=42;s.generation=3;s.air_time=.5;s.speed=10;s.charge=.7;s.slowmo=.4;
 s.rolling_fps=50;s.rolling_interval_ms=20;
 s.render.terrain_cells=7;s.render.object_cells=6;s.render.job_active=true;s.render.job_cell=9;s.render.snow=234;s.render.trail=345;
 s.render.terrain_vertices=12345;s.render.terrain_indices=45678;s.render.object_vertices=123;s.render.object_indices=456;s.render.render_width=512;s.render.render_height=384;s.render.output_width=1024;s.render.output_height=768;s.render.origin={0,128,256};
 s.audio_open=true;s.audio_backend="pulseaudio";s.audio.sample_rate=48000;s.audio.buffer_frames=512;s.audio.callback_count=1000;s.audio.callback_total_ns=6667000000;s.audio.callback_max_ns=23991000;s.audio.callback_over_budget=521;
 s.audio.queue_depth=2;s.audio.queue_high_water=10;s.audio.dropped_commands=4;s.audio.stolen_voices=5;s.audio.nonfinite_samples=6;s.audio.active_voices=32;s.audio.peak_voices=128;
 lines=format_developer_overlay(s);
 require(contains("50.0 FPS / 20.00 ms")&&contains("ticks 2 / physics scale 0.40"),"rolling loop and fixed tick values");
 require(contains("player 3.00 (stream 1.00 subset)")&&contains("bottom submit 6.00 / swap 7.00"),"CPU stage timing labels");
 require(contains("TITLE / air")&&contains("seed 42 / gen 3")&&contains("cell 9 ACTIVE"),"rider and stream state");
 require(contains("render 512x384 / output 1024x768")&&contains("Snow 234 / trail points 345"),"render and effect counts");
 require(contains("mean 6.667 / max 23.991 / budget 10.667")&&contains("misses 521 / calls 1000 (NOT driver XRUNs)"),"callback wall budget not XRUN counts");
 require(contains("Queue 2 / high 10 / drops 4 / steals 5")&&contains("Nonfinite 6 / voices active 32 / peak 128"),"lock-free audio numeric diagnostics");
 for(const auto& row:lines){require(!row.empty()&&row.size()<=developer_overlay_columns,"bounded complete panel");for(unsigned char c:row)require(c>=32&&c<=127,"original-font ASCII");}
 s.frame.dual=false;s.title=false;s.paused=true;s.audio.callback_count=0;lines=format_developer_overlay(s);
 require(contains("PAUSE")&&contains("bottom submit n/a")&&contains("mean n/a"),"pause and missing bottom/callback measures");
 s.audio.callback_count=std::numeric_limits<uint64_t>::max();s.audio.callback_over_budget=std::numeric_limits<uint64_t>::max();s.audio.callback_total_ns=std::numeric_limits<uint64_t>::max();s.audio.callback_max_ns=std::numeric_limits<uint64_t>::max();
 s.audio.dropped_commands=s.audio.stolen_voices=s.audio.nonfinite_samples=std::numeric_limits<uint64_t>::max();s.audio_backend=std::string(1000,'x')+"\n";s.audio_backend[0]='\n';
 lines=format_developer_overlay(s);for(const auto& row:lines){require(row.size()<=developer_overlay_columns,"extreme counters/backend remain bounded");for(unsigned char c:row)require(c>=32&&c<=127,"backend bytes sanitized for atlas");}
 DeveloperOverlayCache cache;require(cache.due(0),"initial refresh due");cache.refresh(s,0);auto before=cache.lines();s.seed=99;cache.refresh(s,.249);require(cache.lines()==before&&!cache.due(.249),"4Hz cache no early reformat");require(cache.due(.25),"250ms refresh");cache.refresh(s,.25);require(cache.lines()!=before,"refresh replaces cached text");cache.invalidate();require(cache.due(.26),"toggle invalidates cache");
 DeveloperFrameWindow window;close(window.fps(),0,"empty window");for(int i=0;i<120;++i)window.add(20);close(window.fps(),50,"rolling FPS");window.add(40);close(window.interval_ms(),2420./120,"rolling overwrite bounded");window.add(0);window.add(-1);window.add(std::nan(""));close(window.interval_ms(),2420./120,"bad intervals ignored");
 std::cout<<"developer overlay: bounded labels, timing scope, callback budget/n-a, cache cadence and rolling window passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
