#pragma once
#include "terrain.hpp"
#include <GLES3/gl3.h>
#include <string>
namespace fl {
struct Camera {Vec3 eye,look;Mat4 vp,inverse_vp;};
class Renderer {
public:
 Renderer()=default;
 ~Renderer();
 Renderer(const Renderer&)=delete;Renderer& operator=(const Renderer&)=delete;
 void initialize(uint32_t seed,int first_cell,int cell_count,int width,int height,const std::string& assets,bool precise_depth=true);
 void resize(int width,int height);
 void render(int drawable_width,int drawable_height,int debug_view=0,int palette=0);
 void capture(const std::string& path);
 size_t vertex_count()const{return vertices_;}
 size_t index_count()const{return indices_;}
 double build_ms()const{return build_ms_;}
 int width()const{return width_;}
 int height()const{return height_;}
 const Camera& camera()const{return camera_;}
private:
 GLuint scene_=0,post_=0,copy_=0,vao_=0,vbo_=0,ibo_=0,fullscreen_vao_=0;
 GLuint scene_fbo_=0,print_fbo_=0,color_=0,depth_=0,printed_=0,glyph_=0;
 int width_=0,height_=0;size_t vertices_=0,indices_=0;double build_ms_=0;
 Camera camera_{};
 void delete_targets();
 void make_targets();
};
}
