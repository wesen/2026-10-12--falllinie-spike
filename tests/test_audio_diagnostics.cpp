#include "developer_overlay.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
namespace { void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);} }
int main(){try{
 fl::DeveloperOverlaySnapshot s;s.audio_open=true;s.audio_backend="dummy";
 auto& a=s.audio;a.sample_rate=48000;a.buffer_frames=1024;a.pcm_target=2048;a.pcm_depth=1536;a.pcm_low=512;a.pcm_high=2048;
 a.frames=8192;a.delivered_frames=6656;a.starved_callbacks=2;a.missing_frames=17;a.source_vinyl=false;
 a.callback_count=100;a.callback_total_ns=100000;a.callback_max_ns=2000;a.callback_cpu_unavailable=100;
 a.producer.count=8;a.producer.total_ns=8000000;a.producer.max_ns=1500000;a.producer.cpu_unavailable=8;
 auto lines=fl::format_developer_overlay(s);
 auto contains=[&](const std::string& text){for(const auto& line:lines)if(line.find(text)!=std::string::npos)return true;return false;};
 require(contains("worker / vinyl off")&&contains("CPU mean n/a")&&contains("CPU n/a"),"mode/vinyl/unavailable CPU clock labels");
 require(contains("starved 2 / missing 17 frames (NOT XRUNs)")&&contains("produced/delivered 8192/6656")&&contains("1536/2048 target low/high 512/2048"),"distinct counters/frontiers/lead");
 a.callback_cpu_count=100;a.callback_cpu_total_ns=100000;a.producer.cpu_count=8;a.producer.cpu_total_ns=4000000;
 lines=fl::format_developer_overlay(s);require(contains("CPU mean 0.001")&&contains("CPU 0.500"),"measured thread CPU, not wall estimate");
 a.frames=a.delivered_frames=a.starved_callbacks=a.missing_frames=a.callback_over_budget=a.producer.over_budget=std::numeric_limits<uint64_t>::max();
 lines=fl::format_developer_overlay(s);
 for(const auto& row:lines){require(!row.empty()&&row.size()<=82,"18x82 bounded rows");for(unsigned char c:row)require(c>=32&&c<=127,"ASCII source font");}
 std::cout<<"buffered main-owned diagnostic panel: bounded mode/vinyl/lead/frontiers/starvation/wall/measured-or-unavailable CPU labels passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
