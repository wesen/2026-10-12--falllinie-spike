#pragma once
#include "audio.hpp"
#include "game.hpp"
#include <array>
#include <string>
namespace fl {
// Main-thread-owned CPU wall times; streaming is a subset of player_ms.
// Bottom times are unavailable without a second window, not measured zeros.
struct DeveloperFrameTimings {
 bool valid=false,dual=false;
 double interval_ms=0,input_ms=0,physics_ms=0,player_ms=0,stream_ms=0;
 double top_submit_ms=0,top_swap_ms=0,bottom_submit_ms=0,bottom_swap_ms=0,pace_ms=0;
 int ticks=0;double physics_scale=1;
};
struct DeveloperRenderDiagnostics {
 size_t terrain_cells=0,object_cells=0,terrain_vertices=0,terrain_indices=0;
 size_t object_vertices=0,object_indices=0,snow=0,trail=0;
 bool job_active=false,relative_origin=true;int job_cell=0;
 int render_width=0,render_height=0,output_width=0,output_height=0;
 Vec3 origin{};
};
struct DeveloperOverlaySnapshot {
 DeveloperFrameTimings frame;
 DeveloperRenderDiagnostics render;
 AudioDiagnostics audio;
 std::string audio_backend="n/a";
 bool audio_open=false,title=false,paused=false;
 RiderMode mode=RiderMode::Ground;Vec3 position{};
 double speed=0,air_time=0,charge=0,slowmo=0,rolling_fps=0,rolling_interval_ms=0;
 uint32_t seed=0;uint64_t generation=0;
};
// Bounded ASCII rows using the original 96-character HUD atlas.
constexpr size_t developer_overlay_rows=18,developer_overlay_columns=82;
using DeveloperOverlayLines=std::array<std::string,developer_overlay_rows>;
DeveloperOverlayLines format_developer_overlay(const DeveloperOverlaySnapshot&);
class DeveloperOverlayCache {
 double refresh_at_=0;
 bool dirty_=true;
 DeveloperOverlayLines lines_{};
public:
 void invalidate(){dirty_=true;}
 bool due(double now)const{return dirty_||now>=refresh_at_;}
 void refresh(const DeveloperOverlaySnapshot&,double now);
 const DeveloperOverlayLines& lines()const{return lines_;}
};
// Bounded rolling frame interval window, updated without formatting/allocation.
class DeveloperFrameWindow {
 std::array<double,120> intervals_{};
 size_t next_=0,count_=0;double sum_=0;
public:
 void add(double interval_ms);
 double interval_ms()const{return count_?sum_/count_:0;}
 double fps()const{return sum_>0?1000*count_/sum_:0;}
};
}
