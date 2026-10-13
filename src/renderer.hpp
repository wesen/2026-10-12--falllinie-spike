#pragma once
#include "terrain.hpp"
#include "game.hpp"
#include <GLES3/gl3.h>
#include <string>
namespace fl {
struct Camera {Vec3 eye,look;Mat4 vp,inverse_vp;};
class Renderer {
public:
 Renderer()=default;
 ~Renderer();
 Renderer(const Renderer&)=delete;Renderer& operator=(const Renderer&)=delete;
 void initialize(uint32_t seed,int first_cell,int cell_count,int width,int height,const std::string& assets,bool precise_depth=true,bool stable_sky=true);
 void resize(int width,int height);
 void update_player(Game& game,double dt);
 void render_first_person(int width,int height,int palette=0);
 void render(int drawable_width,int drawable_height,int debug_view=0,int palette=0);
 void capture(const std::string& path);
 size_t vertex_count()const{return vertices_;}
 size_t index_count()const{return indices_;}
 double build_ms()const{return build_ms_;}
 double stream_ms()const{return stream_ms_;}
 void set_fps(double fps){fps_=fps;}
 int width()const{return width_;}
 int height()const{return height_;}
 const Camera& camera()const{return camera_;}
private:
 GLuint hud_program_=0,hud_font_=0;
 std::string hud_line_,hud_status_;
 GLuint scene_=0,post_=0,copy_=0,vao_=0,vbo_=0,ibo_=0,fullscreen_vao_=0;
 GLuint rider_vao_=0,rider_vbo_=0,rider_ibo_=0;
 GLsizei rider_indices_=0;
 std::array<std::vector<Vertex>,3> rider_poses_;
 std::vector<uint32_t> rider_triangles_;
 std::vector<std::array<uint32_t,3>> rider_groups_;
 Vec3 camera_direction_{0,0,1},camera_velocity_{},look_velocity_{};
 bool following_=false;
 Vec3 first_person_eye_{},first_person_forward_{0,0,1},first_person_up_{0,1,0};
 double fov_=62,game_time_=0,stream_ms_=0,fps_=0;
 int patch_first_=0,patch_count_=3;
 GLuint scene_fbo_=0,print_fbo_=0,color_=0,depth_=0,printed_=0,glyph_=0;
 int width_=0,height_=0;size_t vertices_=0,indices_=0;double build_ms_=0;
 Camera camera_{};
 void delete_targets();
 void make_targets();
};
}
