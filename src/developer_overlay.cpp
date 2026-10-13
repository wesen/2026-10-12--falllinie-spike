#include "developer_overlay.hpp"
#include <cmath>
#include <cstdio>
namespace fl {
DeveloperOverlayLines format_developer_overlay(const DeveloperOverlaySnapshot& s){
 DeveloperOverlayLines lines;
 size_t row=0;
 auto line=[&](const char* fmt,auto... args){
  char text[developer_overlay_columns+1]{};
  if constexpr(sizeof...(args)==0)std::snprintf(text,sizeof text,"%s",fmt);
  else std::snprintf(text,sizeof text,fmt,args...);
  lines[row++]=text;
 };
 const auto& f=s.frame;const auto& r=s.render;const auto& a=s.audio;
 const char* mode=s.mode==RiderMode::Ground?"ground":s.mode==RiderMode::Air?"air":s.mode==RiderMode::Rail?"rail":"bail";
 line("DEV / SELECT-F2 / 4 Hz cache / CPU wall times, NOT presentation rate");
 line("Rolling loop %.1f FPS / %.2f ms interval (last <=120 frames)",s.rolling_fps,s.rolling_interval_ms);
 if(f.valid){
  line("Previous completed frame %.2f ms / ticks %d / physics scale %.2f",f.interval_ms,f.ticks,f.physics_scale);
  line("CPU ms input %.2f / physics %.2f / player %.2f (stream %.2f subset)",f.input_ms,f.physics_ms,f.player_ms,f.stream_ms);
  line("CPU ms top submit %.2f / swap %.2f / pace %.2f",f.top_submit_ms,f.top_swap_ms,f.pace_ms);
  if(f.dual)line("CPU ms bottom submit %.2f / swap %.2f",f.bottom_submit_ms,f.bottom_swap_ms);
  else line("CPU ms bottom submit n/a / swap n/a (single window)");
 }else{
  line("Previous completed frame n/a / ticks n/a / physics scale n/a");
  line("CPU ms input / physics / player / stream: n/a");
  line("CPU ms top submit / swap / pace: n/a");
  line("CPU ms bottom submit / swap: n/a");
 }
 line("GPU elapsed n/a / GPU utilization n/a / CPU utilization n/a");
 line("%s / %s / pos %.1f %.1f %.1f / speed %.2f m/s",s.title?"TITLE":s.paused?"PAUSE":"RUN",mode,s.position.x,s.position.y,s.position.z,s.speed);
 line("Air %.2f s / charge %.2f / slowmo %.2f s / seed %u / gen %llu",s.air_time,s.charge,s.slowmo,s.seed,(unsigned long long)s.generation);
 if(r.job_active)line("Resident terrain %zu / objects %zu / terrain row job cell %d ACTIVE",r.terrain_cells,r.object_cells,r.job_cell);
 else line("Resident terrain %zu / objects %zu / terrain row job idle",r.terrain_cells,r.object_cells);
 line("Visible terrain V/I %zu/%zu / objects V/I %zu/%zu",r.terrain_vertices,r.terrain_indices,r.object_vertices,r.object_indices);
 line("Snow %zu / trail points %zu / render %dx%d / output %dx%d",r.snow,r.trail,r.render_width,r.render_height,r.output_width,r.output_height);
 line("%s origin %.1f %.1f %.1f",r.relative_origin?"Camera-relative":"Absolute",r.origin.x,r.origin.y,r.origin.z);
 if(s.audio_open&&a.sample_rate>0){
  const double budget=1000.*a.buffer_frames/a.sample_rate;
  line("Audio %.24s / %d Hz / actual buffer %d frames / %s",s.audio_backend.c_str(),a.sample_rate,a.buffer_frames,a.muted?"muted":"on");
  if(a.callback_count)line("Callback mean %.3f / max %.3f / budget %.3f ms (wall, cumulative)",double(a.callback_total_ns)/a.callback_count/1e6,double(a.callback_max_ns)/1e6,budget);
  else line("Callback mean n/a / max n/a / budget %.3f ms (no callbacks yet)",budget);
  line("Callback budget misses %llu / calls %llu (NOT driver XRUNs)",(unsigned long long)a.callback_over_budget,(unsigned long long)a.callback_count);
 }else{
  line("Audio n/a (device not open) / rate n/a / actual buffer n/a");
  line("Callback mean n/a / max n/a / budget n/a");
  line("Callback budget misses n/a / calls n/a (NOT driver XRUNs)");
 }
 line("Queue %u / high %u / drops %llu / steals %llu",a.queue_depth,a.queue_high_water,(unsigned long long)a.dropped_commands,(unsigned long long)a.stolen_voices);
 line("Nonfinite %llu / voices active %u / peak %u (cumulative counters)",(unsigned long long)a.nonfinite_samples,a.active_voices,a.peak_voices);
 return lines;
}
void DeveloperOverlayCache::refresh(const DeveloperOverlaySnapshot& s,double now){
 if(!due(now))return;
 lines_=format_developer_overlay(s);refresh_at_=now+.25;dirty_=false;
}
void DeveloperFrameWindow::add(double interval){
 if(!std::isfinite(interval)||interval<=0)return;
 sum_-=intervals_[next_];intervals_[next_]=interval;sum_+=interval;
 next_=(next_+1)%intervals_.size();count_=std::min(count_+1,intervals_.size());
}
}
