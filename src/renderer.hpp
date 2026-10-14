#pragma once
#include "terrain.hpp"
#include "shadow_bounds.hpp"
#include "geometry_bounds.hpp"
#include "profiler.hpp"
#include "game.hpp"
#include "pose.hpp"
#include "developer_overlay.hpp"
#include <GLES3/gl3.h>
#include <string>
#include <memory>
namespace fl {
struct Camera {Vec3 eye,look;Mat4 vp,inverse_vp;};
class Renderer {
public:
 Renderer()=default;
 ~Renderer();
 Renderer(const Renderer&)=delete;Renderer& operator=(const Renderer&)=delete;
 void initialize(uint32_t seed,int first_cell,int cell_count,int width,int height,const std::string& assets,bool precise_depth=true,bool stable_sky=true,bool play_mode=false);
 void resize(int width,int height);
 void update_player(Game& game,double dt);
 void render_first_person(int width,int height,int palette=0);
 void render(int drawable_width,int drawable_height,int debug_view=0,int palette=0);
 void capture(const std::string& path);
 void capture_output(const std::string& path,int width,int height);
 void set_shadow_mode(ShadowMode mode){shadow_mode_=mode;}
 void set_measurement(Profiler* profiler,uint64_t frame,int view){profiler_=profiler;profile_frame_=frame;profile_view_=view;}
 void set_post_bypass(bool bypass){post_bypass_=bypass;}
 void set_draw_ranges(bool enabled){draw_ranges_=enabled;}
 void set_benchmark_replay(bool enabled){benchmark_replay_=enabled;}
 // PERF-009 geometry policies; each is opt-in until qualified.
 void set_fog_constant(bool enabled){fog_constant_=enabled;}
 void set_view_culling(bool enabled){view_culling_=enabled;}
 void set_short_indices(bool enabled){short_indices_=enabled;}
 void set_output_clear(bool enabled){output_clear_=enabled;}
 std::string draw_stats_json()const;
 size_t vertex_count()const{return vertices_;}
 size_t index_count()const{return indices_;}
 double build_ms()const{return build_ms_;}
 double stream_ms()const{return stream_ms_;}
 void set_fps(double fps){fps_=fps;}
 void set_native_feedback(bool controller,bool muted){controller_=controller;muted_=muted;}
 void set_absolute_coordinates(bool absolute){relative_coordinates_=!absolute;}
 DeveloperRenderDiagnostics developer_diagnostics(int output_width,int output_height)const;
 void set_developer_overlay(bool visible){developer_overlay_visible_=visible;}
 void set_developer_overlay_lines(const DeveloperOverlayLines& lines){developer_overlay_lines_=lines;developer_overlay_dirty_=true;}
 int width()const{return width_;}
 int height()const{return height_;}
 const Camera& camera()const{return camera_;}
private:
 ShadowMode shadow_mode_=ShadowMode::Original;
 void upload_shadow_segments(const float* segments);
 bool draw_ranges_=false,benchmark_replay_=false,fog_constant_=false,view_culling_=false,short_indices_=false,output_clear_=false;
 void draw_indexed(GLsizei count,size_t vertices,const void* offset=nullptr,GLenum type=GL_UNSIGNED_INT);
 struct DrawStats{uint64_t views=0,chunks_drawn=0,chunks_culled=0,chunks_fog_constant=0,terrain_drawn=0,terrain_culled=0,terrain_fog_constant=0,object_drawn=0,object_culled=0,object_fog_constant=0,short_index_draws=0,triangles_submitted=0,triangles_culled=0;};
 DrawStats draw_stats_;
 Profiler* profiler_=nullptr;uint64_t profile_frame_=0;int profile_view_=0;bool post_bypass_=false;
 GLuint hud_program_=0,hud_font_=0,hud_vao_=0,hud_vbo_=0;
 GLuint developer_overlay_vao_=0,developer_overlay_vbo_=0;
 DeveloperOverlayLines developer_overlay_lines_{};
 bool developer_overlay_visible_=false,developer_overlay_dirty_=true,first_person_view_=false;
 int developer_overlay_width_=0,developer_overlay_height_=0;
 GLsizei developer_overlay_vertices_=0;
 void draw_developer_overlay(int width,int height);
 std::string hud_line_,hud_status_,hud_combo_,hud_meter_,hud_banner_,hud_controls_,hud_grade_,hud_points_;
 std::vector<std::string> hud_trick_lines_;
 double hud_meter_fraction_=0,hud_combo_fraction_=0;
 struct CellMesh{GLuint vao=0,vbo=0,ibo=0;GLsizei indices=0;size_t vertices=0;GLenum index_type=GL_UNSIGNED_INT;Aabb bounds{};std::vector<IndexChunk> chunks;};
 std::map<int,CellMesh> cell_meshes_,object_meshes_;
 CellMesh particle_mesh_,trail_mesh_;
 Particles particles_;
 uint64_t world_generation_=0;
 RiderGeometry rider_geometry_;RiderMesh rider_mesh_;RiderRig rig_;
 bool rig_valid_=false,controller_=false,muted_=false;
 double inversion_=0,flash_=0,speed_effect_=0;
 Rng camera_rng_{0x821371u};
 std::unique_ptr<TerrainJob> terrain_job_;
 CellMesh upload_cell(const CpuMesh& mesh,bool allow_short=true);
 void delete_cell(CellMesh& cell);
 void update_dynamic(CellMesh& cell,const CpuMesh& mesh);
 GLuint scene_=0,post_=0,copy_=0,vao_=0,vbo_=0,ibo_=0,fullscreen_vao_=0;
 GLuint rider_vao_=0,rider_vbo_=0,rider_ibo_=0;
 GLsizei rider_indices_=0;

 std::vector<std::array<uint32_t,3>> rider_groups_;
 Vec3 camera_direction_{0,0,1},camera_velocity_{},look_velocity_{},camera_shake_{};
 bool following_=false,relative_coordinates_=true;
 Vec3 first_person_eye_{},first_person_forward_{0,0,1},first_person_up_{0,1,0};
 double fov_=62,game_time_=0,render_time_=0,stream_ms_=0,fps_=0;
 int patch_first_=0,patch_count_=3;
 GLuint scene_fbo_=0,print_fbo_=0,color_=0,depth_=0,printed_=0,glyph_=0;
 int width_=0,height_=0;size_t vertices_=0,indices_=0;double build_ms_=0;
 Camera camera_{};
 void delete_targets();
 void make_targets();
};
}
