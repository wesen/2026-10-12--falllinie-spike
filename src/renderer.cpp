#include "renderer.hpp"
#include "gl_api.hpp"
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace fl {
static std::string read_file(const std::string& p){std::ifstream f(p);if(!f)throw std::runtime_error("cannot read shader: "+p);return {std::istreambuf_iterator<char>(f),{}};}
static GLuint compile(GLenum kind,const std::string& source,const std::string& label){
 // Optional developer dump of the exact adapted bytes passed to glShaderSource.
 // No shader rewriting or per-frame work; disabled unless explicitly requested.
 if(const char* dir=std::getenv("FALLLINIE_SHADER_DUMP_DIR")){if(*dir){
  static unsigned serial=0;std::filesystem::create_directories(dir);
  auto path=std::filesystem::path(dir)/(std::to_string(serial++)+(kind==GL_VERTEX_SHADER?".vert":".frag"));
  std::ofstream dump(path,std::ios::binary);dump<<source;if(!dump)throw std::runtime_error("cannot dump shader "+path.string());
 }}
 GLuint shader=glCreateShader(kind);const char* text=source.c_str();glShaderSource(shader,1,&text,nullptr);glCompileShader(shader);
 GLint ok=0;glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
 if(!ok){GLint size=0;glGetShaderiv(shader,GL_INFO_LOG_LENGTH,&size);std::vector<char> log(std::max(1,size));glGetShaderInfoLog(shader,size,nullptr,log.data());glDeleteShader(shader);throw std::runtime_error(label+": "+log.data());}
 return shader;
}
static GLuint program(const std::string& vs,const std::string& fs,const std::string& label){
 GLuint v=0,f=0,p=0;
 try{v=compile(GL_VERTEX_SHADER,vs,label+" vertex");f=compile(GL_FRAGMENT_SHADER,fs,label+" fragment");p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);
  GLint ok=0;glGetProgramiv(p,GL_LINK_STATUS,&ok);if(!ok){GLint size=0;glGetProgramiv(p,GL_INFO_LOG_LENGTH,&size);std::vector<char> log(std::max(1,size));glGetProgramInfoLog(p,size,nullptr,log.data());throw std::runtime_error(label+" link: "+log.data());}
  glDeleteShader(v);glDeleteShader(f);std::cout<<"program "<<label<<" linked\n";return p;
 }catch(...){if(v)glDeleteShader(v);if(f)glDeleteShader(f);if(p)glDeleteProgram(p);throw;}
}
static GLint loc(GLuint p,const char* name){return glGetUniformLocation(p,name);}
static void scalar(GLuint p,const char* name,float value){glUniform1f(loc(p,name),value);}
static void integer(GLuint p,const char* name,int value){glUniform1i(loc(p,name),value);}
static void vec3(GLuint p,const char* name,Vec3 v){glUniform3f(loc(p,name),float(v.x),float(v.y),float(v.z));}
static void matrix(GLuint p,const char* name,Mat4 v){glUniformMatrix4fv(loc(p,name),1,GL_FALSE,v.data());}
static void gl_check(const char* stage){GLenum e=glGetError();if(e!=GL_NO_ERROR)throw std::runtime_error(std::string(stage)+" GL error "+std::to_string(e));}
static void framebuffer_check(const char* name){GLenum status=glCheckFramebufferStatus(GL_FRAMEBUFFER);if(status!=GL_FRAMEBUFFER_COMPLETE)throw std::runtime_error(std::string(name)+" FBO status "+std::to_string(status));}
static GLuint texture(int w,int h,GLint internal,GLenum format,GLenum type){
 GLuint t=0;glGenTextures(1,&t);glBindTexture(GL_TEXTURE_2D,t);glTexImage2D(GL_TEXTURE_2D,0,internal,w,h,0,format,type,nullptr);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);return t;
}
void Renderer::initialize(uint32_t seed,int first_cell,int count,int w,int h,const std::string& assets,bool precise_depth,bool stable_sky,bool play_mode){
 std::string scene_source=read_file(assets+"/scene_vs.glsl");
 const std::string projection="gl_Position=uVP*w;";
 auto projection_at=scene_source.find(projection);
 if(projection_at==std::string::npos)throw std::runtime_error("original scene projection expression missing");
 scene_source.replace(projection_at,projection.size(),"gl_Position=uVP*vec4(w.xyz-uRenderOrigin,1.);");
 scene_source.insert(scene_source.find("void main(){"),"uniform highp vec3 uRenderOrigin;\n");
 scene_=program(scene_source,read_file(assets+"/scene_fs.glsl"),"original scene with relative projection");
 std::cout<<"projection coordinates="<<(relative_coordinates_?"camera-relative":"source absolute (diagnostic)")<<'\n';
 std::string print_source=read_file(assets+"/post_fs.glsl");
 if(precise_depth){
  const std::string original="uniform sampler2D uC, uD;";
  auto at=print_source.find(original);
  if(at==std::string::npos) throw std::runtime_error("original depth sampler declaration not found");
  print_source.replace(at,original.size(),"uniform sampler2D uC; uniform highp sampler2D uD;");
 }
 if(stable_sky){
  // Source layer amplitudes/shapes are retained; only its unstable random basis changes.
  // Wrap the angular lattice so the first-person rear-facing view has no atan seam.
  const std::string helper=R"(
uniform highp float uSkySamples[170];
float skyHash(float index){return uSkySamples[int(index)];}
float skyNoise(float az,float frequency,float offset){float count=floor(6.28318530718*frequency+.5);float x=(az+3.14159265359)/6.28318530718*count+offset;float i=floor(x),f=fract(x);f=f*f*(3.-2.*f);return mix(skyHash(mod(i,count)),skyHash(mod(i+1.,count)),f);}
)";
  auto at=print_source.find("void main(){");print_source.insert(at,helper);
  for(const auto& entry:std::array<std::array<std::string,2>,4>{{{{"n1(az*3.+3.)","skyNoise(az,3.,3.)"}},{{"n1(az*11.)","skyNoise(az,11.,0.)"}},{{"n1(az*6.+11.)","skyNoise(az,6.,11.)"}},{{"n1(az*27.)","skyNoise(az,27.,0.)"}}}}){auto pos=print_source.find(entry[0]);if(pos==std::string::npos)throw std::runtime_error("sky source expression missing");print_source.replace(pos,entry[0].size(),entry[1]);}
 }
 std::cout<<"sky hash="<<(stable_sky?"stable wrapped integer lattice":"source sine hash")<<'\n';
 std::cout<<"depth sampler="<<(precise_depth?"explicit highp portability override":"original implicit precision")<<'\n';
 post_=program(read_file(assets+"/post_vs.glsl"),print_source,"original print");
 if(stable_sky){std::array<float,170> values{};for(uint32_t i=0;i<values.size();++i){uint32_t h=i;h=(h^(h>>16))*0x7feb352du;h=(h^(h>>15))*0x846ca68bu;h^=h>>16;values[i]=float(h>>8)*(1.f/16777216.f);}glUseProgram(post_);glUniform1fv(loc(post_,"uSkySamples[0]"),GLsizei(values.size()),values.data());}
 const std::string copy_fs=R"(#version 300 es
precision highp float;
uniform sampler2D uC; uniform highp sampler2D uD; uniform vec2 uOutputRes; uniform int uMode; out vec4 o;
void main(){vec2 uv=gl_FragCoord.xy/uOutputRes;vec4 c=texture(uC,uv);float x=0.;
if(uMode==0){o=vec4(c.rgb,1.);return;}
if(uMode==1)x=c.r;else if(uMode==2)x=c.g;else if(uMode==3)x=c.b*255./32.;else if(uMode==4)x=c.a;
else{float d=texture(uD,uv).r;float z=d*2.-1.;float linear=2.*.1*900./(900.+.1-z*(900.-.1));x=linear/150.;}
o=vec4(vec3(x),1.);})";
 copy_=program(read_file(assets+"/post_vs.glsl"),copy_fs,"presentation diagnostic");
 const std::string hud_fs=R"(#version 300 es
precision highp float;
uniform highp sampler2D uFont;uniform vec3 uInk,uPaper;in vec2 vUV;out vec4 o;
void main(){if(vUV.x< -1.5){o=vec4(uInk,1.);return;}if(vUV.x<0.){o=vec4(uPaper,.92);return;}float alpha=texture(uFont,vUV).a;o=vec4(uInk,alpha);})";
 const std::string hud_vs=R"(#version 300 es
precision highp float;
layout(location=0) in vec2 aPosition;layout(location=1) in vec2 aUV;
uniform vec2 uOutputRes;out vec2 vUV;
void main(){vUV=aUV;gl_Position=vec4(aPosition/uOutputRes*2.-1.,0.,1.);})";
 hud_program_=program(hud_vs,hud_fs,"batched native HUD using source font");
 glGenVertexArrays(1,&hud_vao_);glBindVertexArray(hud_vao_);glGenBuffers(1,&hud_vbo_);glBindBuffer(GL_ARRAY_BUFFER,hud_vbo_);glEnableVertexAttribArray(0);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,16,nullptr);glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,16,reinterpret_cast<void*>(8));
 glGenVertexArrays(1,&developer_overlay_vao_);glBindVertexArray(developer_overlay_vao_);glGenBuffers(1,&developer_overlay_vbo_);glBindBuffer(GL_ARRAY_BUFFER,developer_overlay_vbo_);glEnableVertexAttribArray(0);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,16,nullptr);glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,16,reinterpret_cast<void*>(8));
 SDL_Surface* font=SDL_LoadBMP((assets+"/../hud-font.bmp").c_str());
 if(!font)throw std::runtime_error("missing source HUD font atlas");
 SDL_Surface* font_rgba=SDL_ConvertSurfaceFormat(font,SDL_PIXELFORMAT_ABGR8888,0);SDL_FreeSurface(font);
 if(!font_rgba)throw std::runtime_error(SDL_GetError());
 hud_font_=texture(font_rgba->w,font_rgba->h,GL_RGBA8,GL_RGBA,GL_UNSIGNED_BYTE);glTexSubImage2D(GL_TEXTURE_2D,0,0,0,font_rgba->w,font_rgba->h,GL_RGBA,GL_UNSIGNED_BYTE,font_rgba->pixels);SDL_FreeSurface(font_rgba);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
 for(auto name:{"uVP","uM","uKind"})if(loc(scene_,name)<0)throw std::runtime_error(std::string("missing scene uniform ")+name);
 for(auto name:{"uC","uD","uRes"})if(loc(post_,name)<0)throw std::runtime_error(std::string("missing print uniform ")+name);
 patch_first_=first_cell;patch_count_=count;world_generation_=1;
 Terrain terrain(seed);World world(terrain,seed);CpuMesh mesh;const auto started=std::chrono::steady_clock::now();
 for(int cell=first_cell;cell<first_cell+count;++cell){auto part=build_terrain(terrain,cell);vertices_+=part.vertices.size();indices_+=part.indices.size();if(play_mode){cell_meshes_.emplace(cell,upload_cell(part));object_meshes_.emplace(cell,upload_cell(world.objects(cell)));}else{uint32_t offset=uint32_t(mesh.vertices.size());mesh.vertices.insert(mesh.vertices.end(),part.vertices.begin(),part.vertices.end());for(uint32_t i:part.indices)mesh.indices.push_back(offset+i);}}
 build_ms_=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
 const int sizes[]={3,3,1,2,2},offsets[]={0,12,24,28,36};
 if(!play_mode){auto static_mesh=upload_cell(mesh);vao_=static_mesh.vao;vbo_=static_mesh.vbo;ibo_=static_mesh.ibo;}
 glGenVertexArrays(1,&fullscreen_vao_);
 glGenVertexArrays(1,&rider_vao_);glBindVertexArray(rider_vao_);glGenBuffers(1,&rider_vbo_);glBindBuffer(GL_ARRAY_BUFFER,rider_vbo_);glGenBuffers(1,&rider_ibo_);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,rider_ibo_);
 for(int i=0;i<5;++i){glEnableVertexAttribArray(i);glVertexAttribPointer(i,sizes[i],GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(intptr_t(offsets[i])));}
 // Complete digit sampler before the original atlas upload.
 glyph_=texture(1,1,GL_RGBA8,GL_RGBA,GL_UNSIGNED_BYTE);const unsigned char transparent[4]={0,0,0,0};glTexSubImage2D(GL_TEXTURE_2D,0,0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,transparent);
 const std::string atlas=assets+"/../glyphs.bmp";
 if(SDL_Surface* loaded=SDL_LoadBMP(atlas.c_str())){
  SDL_Surface* rgba=SDL_ConvertSurfaceFormat(loaded,SDL_PIXELFORMAT_ABGR8888,0);SDL_FreeSurface(loaded);
  if(!rgba)throw std::runtime_error(SDL_GetError());
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,rgba->w,rgba->h,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba->pixels);
  glGenerateMipmap(GL_TEXTURE_2D);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);SDL_FreeSurface(rgba);
  std::cout<<"glyph atlas loaded\n";
 }else throw std::runtime_error("required original glyph atlas missing: "+atlas);
 double z=first_cell*CELL+8;camera_.eye={Terrain::center(z-8),terrain.height(Terrain::center(z),z)+5,z-8};camera_.look={Terrain::center(z+30),terrain.height(Terrain::center(z+30),z+30)+1,z+30};
 width_=w;height_=h;make_targets();gl_check("initialization");
 std::cout<<"terrain vertices="<<vertices_<<" indices="<<indices_<<" build_ms="<<build_ms_<<" targets="<<w<<"x"<<h<<" FBOs complete\n";
}
void Renderer::delete_targets(){
 if(scene_fbo_) glDeleteFramebuffers(1,&scene_fbo_);
 if(print_fbo_) glDeleteFramebuffers(1,&print_fbo_);
 GLuint textures[]={color_,depth_,printed_};glDeleteTextures(3,textures);scene_fbo_=print_fbo_=color_=depth_=printed_=0;
}
void Renderer::make_targets(){
 color_=texture(width_,height_,GL_RGBA8,GL_RGBA,GL_UNSIGNED_BYTE);depth_=texture(width_,height_,GL_DEPTH_COMPONENT24,GL_DEPTH_COMPONENT,GL_UNSIGNED_INT);printed_=texture(width_,height_,GL_RGBA8,GL_RGBA,GL_UNSIGNED_BYTE);
 glGenFramebuffers(1,&scene_fbo_);glBindFramebuffer(GL_FRAMEBUFFER,scene_fbo_);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color_,0);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,depth_,0);
 GLenum draw=GL_COLOR_ATTACHMENT0;glDrawBuffers(1,&draw);framebuffer_check("scene");
 glGenFramebuffers(1,&print_fbo_);glBindFramebuffer(GL_FRAMEBUFFER,print_fbo_);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,printed_,0);glDrawBuffers(1,&draw);framebuffer_check("print");glBindFramebuffer(GL_FRAMEBUFFER,0);
 camera_.vp=multiply(perspective(62*3.141592653589793/180,double(width_)/height_,0.1,900),look_at(camera_.eye,camera_.look,{0,1,0}));camera_.inverse_vp=inverse(camera_.vp);
}
void Renderer::resize(int w,int h){if(w==width_ && h==height_)return;delete_targets();width_=w;height_=h;make_targets();gl_check("resize");}
void Renderer::render(int drawable_w,int drawable_h,int debug,int palette){
 Profiler::Scope whole(profiler_,profile_frame_,profile_view_,RenderPass::View);
 Vec3 eye=camera_.eye+camera_shake_;
 // Keep the active view's roll; rebase projection only, never world shading.
 Vec3 up=norm({camera_.vp[1],camera_.vp[5],camera_.vp[9]});
 const auto frame=render_frame(eye,camera_.look,up,fov_*3.141592653589793/180,double(width_)/height_,relative_coordinates_);
 { Profiler::Scope scene(profiler_,profile_frame_,profile_view_,RenderPass::Scene);
 glBindFramebuffer(GL_FRAMEBUFFER,scene_fbo_);glViewport(0,0,width_,height_);glEnable(GL_DEPTH_TEST);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glDepthMask(GL_TRUE);glClearColor(1,0,0,0);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glUseProgram(scene_);
 matrix(scene_,"uVP",frame.vp);vec3(scene_,"uRenderOrigin",frame.origin);matrix(scene_,"uM",identity());const float identity3[]={1,0,0,0,1,0,0,0,1};glUniformMatrix3fv(loc(scene_,"uN"),1,GL_FALSE,identity3);
 vec3(scene_,"uCam",eye);vec3(scene_,"uL",norm({0.45,0.78,-0.5}));glUniform2f(loc(scene_,"uFog"),150,400);
 // Static spike has no rider; play uses the original live rig shadow segments.
 const float shadows[]={1000000,1000000,1000000,.26f,1000000,1000001,1000000,0,1000002,1000000,1000000,.17f,1000002,1000001,1000000,0,1000004,1000000,1000000,.2f,1000004,1000001,1000000,0};
 if(rig_valid_){Vec3 bx=qrot(rig_.board_orientation,{1,0,0}),nose=rig_.board_position+bx*.75,tail=rig_.board_position-bx*.75,ankle=(rig_.ankle[0]+rig_.ankle[1])*.5;const float seg[]={float(rig_.pelvis.x),float(rig_.pelvis.y),float(rig_.pelvis.z),.26f,float(rig_.head.x),float(rig_.head.y),float(rig_.head.z),0,float(nose.x),float(nose.y),float(nose.z),.17f,float(tail.x),float(tail.y),float(tail.z),0,float(rig_.pelvis.x),float(rig_.pelvis.y),float(rig_.pelvis.z),.2f,float(ankle.x),float(ankle.y),float(ankle.z),0};glUniform4fv(loc(scene_,"uSeg"),6,seg);}else glUniform4fv(loc(scene_,"uSeg"),6,shadows);integer(scene_,"uKind",0);scalar(scene_,"uMat",1);scalar(scene_,"uAlbO",-1);scalar(scene_,"uXray",(palette%6+6)%6==5?1:0);
 glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,glyph_);integer(scene_,"uGlyph",0);if(cell_meshes_.empty()){glBindVertexArray(vao_);glDrawElements(GL_TRIANGLES,GLsizei(indices_),GL_UNSIGNED_INT,nullptr);}else for(int cell=patch_first_;cell<patch_first_+patch_count_;++cell){auto found=cell_meshes_.find(cell);if(found!=cell_meshes_.end()){glBindVertexArray(found->second.vao);glDrawElements(GL_TRIANGLES,found->second.indices,GL_UNSIGNED_INT,nullptr);}}
 integer(scene_,"uKind",1);scalar(scene_,"uMat",2);
 for(int cell=patch_first_;cell<patch_first_+patch_count_;++cell){auto found=object_meshes_.find(cell);if(found!=object_meshes_.end()){glBindVertexArray(found->second.vao);glDrawElements(GL_TRIANGLES,found->second.indices,GL_UNSIGNED_INT,nullptr);}}
 if(rig_valid_){
  Vec3 view=norm(camera_.eye-camera_.look),right=source_norm(cross(first_person_up_,view),{1,0,0}),up=cross(view,right);
  if(following_){right=source_norm(cross({0,1,0},view),{1,0,0});up=cross(view,right);}
  if(profiler_&&profiler_->sampling(profile_frame_)){
   CpuMesh snow,trail;
   {Profiler::Scope prep(profiler_,profile_frame_,profile_view_,RenderPass::DynamicPrep);snow=particles_.snow_mesh(eye,right,up);trail=particles_.trail_mesh(game_time_);}
   {Profiler::Scope upload(profiler_,profile_frame_,profile_view_,RenderPass::DynamicUpload);update_dynamic(particle_mesh_,snow);update_dynamic(trail_mesh_,trail);}
  }else{update_dynamic(particle_mesh_,particles_.snow_mesh(eye,right,up));update_dynamic(trail_mesh_,particles_.trail_mesh(game_time_));}
  if(trail_mesh_.indices){glEnable(GL_POLYGON_OFFSET_FILL);glPolygonOffset(-2,-4);integer(scene_,"uKind",3);scalar(scene_,"uMat",5);glBindVertexArray(trail_mesh_.vao);glDrawElements(GL_TRIANGLES,trail_mesh_.indices,GL_UNSIGNED_INT,nullptr);glDisable(GL_POLYGON_OFFSET_FILL);}
 }
 integer(scene_,"uKind",1);
 if(rider_indices_){integer(scene_,"uKind",1);glBindVertexArray(rider_vao_);for(const auto& group:rider_groups_){scalar(scene_,"uMat",float(group[2]));glDrawElements(GL_TRIANGLES,GLsizei(group[1]),GL_UNSIGNED_INT,reinterpret_cast<void*>(uintptr_t(group[0])*4));}}
 if(particle_mesh_.indices){integer(scene_,"uKind",2);scalar(scene_,"uMat",30);scalar(scene_,"uAlbO",-1);glBindVertexArray(particle_mesh_.vao);glDrawElements(GL_TRIANGLES,particle_mesh_.indices,GL_UNSIGNED_INT,nullptr);}
 } // scene (includes dynamic CPU preparation and uploads)
 const Vec3 papers[]={{243/255.0,244/255.0,247/255.0},{244/255.0,244/255.0,245/255.0},{244/255.0,243/255.0,248/255.0},{238/255.0,244/255.0,246/255.0},{240/255.0,240/255.0,241/255.0},{10/255.0,16/255.0,32/255.0}};
 const Vec3 inks[]={{28/255.0,47/255.0,158/255.0},{18/255.0,18/255.0,18/255.0},{43/255.0,26/255.0,92/255.0},{11/255.0,42/255.0,58/255.0},{17/255.0,17/255.0,18/255.0},{143/255.0,234/255.0,1}};
 const Vec3 spots[]={{1,77/255.0,141/255.0},{227/255.0,18/255.0,11/255.0},{1,106/255.0,0},{0,166/255.0,166/255.0},{141/255.0,141/255.0,145/255.0},{74/255.0,99/255.0,1}};
 palette=(palette%6+6)%6;
 if(!post_bypass_){Profiler::Scope post(profiler_,profile_frame_,profile_view_,RenderPass::Post);
 glBindFramebuffer(GL_FRAMEBUFFER,print_fbo_);glViewport(0,0,width_,height_);glDisable(GL_DEPTH_TEST);glUseProgram(post_);
 glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,color_);integer(post_,"uC",0);glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,depth_);integer(post_,"uD",1);
 glUniform2f(loc(post_,"uRes"),width_,height_);matrix(post_,"uIVP",frame.inverse_vp);vec3(post_,"uCamP",frame.eye);vec3(post_,"uSunG",norm({.42,.14,1}));
 vec3(post_,"uPaper",papers[palette]);vec3(post_,"uInk",inks[palette]);vec3(post_,"uGrey",spots[palette]);
 scalar(post_,"uNear",.1);scalar(post_,"uFar",900);scalar(post_,"uPitch",4.4f*width_/1024);scalar(post_,"uInv",float(inversion_));scalar(post_,"uTime",float(render_time_));scalar(post_,"uSpeed",float(speed_effect_));scalar(post_,"uFlash",float(flash_));scalar(post_,"uEW",1);scalar(post_,"uApx",float((fov_*3.141592653589793/180)/height_*1.2));glUniform2f(loc(post_,"uFogP"),150,380);
 Vec3 sun=frame.eye+norm({.42,.14,1})*800;double clip[4]{};for(int r=0;r<4;++r)clip[r]=frame.vp[r]*sun.x+frame.vp[4+r]*sun.y+frame.vp[8+r]*sun.z+frame.vp[12+r];
 if(clip[3]>0)glUniform2f(loc(post_,"uSunPx"),(clip[0]/clip[3]*.5+.5)*width_,(clip[1]/clip[3]*.5+.5)*height_);else glUniform2f(loc(post_,"uSunPx"),-9999,-9999);
 scalar(post_,"uSunR",height_*.085);glBindVertexArray(fullscreen_vao_);glDrawArrays(GL_TRIANGLES,0,3);
 } // bypass omits the print draw and all post uniforms, not just its presentation
 {Profiler::Scope output(profiler_,profile_frame_,profile_view_,RenderPass::Output);
 glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,drawable_w,drawable_h);if(post_bypass_){glDisable(GL_DEPTH_TEST);glBindVertexArray(fullscreen_vao_);}glUseProgram(copy_);
 glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,(debug||post_bypass_)?color_:printed_);integer(copy_,"uC",0);glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,depth_);integer(copy_,"uD",1);integer(copy_,"uMode",debug);glUniform2f(loc(copy_,"uOutputRes"),drawable_w,drawable_h);glDrawArrays(GL_TRIANGLES,0,3);
 if(following_){
  glUseProgram(hud_program_);glUniform2f(loc(hud_program_,"uOutputRes"),drawable_w,drawable_h);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,hud_font_);integer(hud_program_,"uFont",0);vec3(hud_program_,"uInk",inks[palette]);vec3(hud_program_,"uPaper",papers[palette]);
  struct HUDVertex{float x,y,u,v;};static_assert(sizeof(HUDVertex)==16);std::vector<HUDVertex> glyphs;glyphs.reserve(1200);
  const float corners[6][2]={{0,0},{1,0},{0,1},{0,1},{1,0},{1,1}};
  auto quad=[&](float left,float bottom,float width,float height,int character){for(const auto& p:corners)glyphs.push_back({left+p[0]*width,bottom+p[1]*height,character<0?float(character):(character+p[0])/96.f,character<0?0.f:1.f-p[1]});};
  auto text=[&](const std::string& value,float left,float bottom,float width,float height){for(size_t i=0;i<std::min(value.size(),size_t(std::max(0.f,(drawable_w-left-8)/width)));++i)quad(left+i*width,bottom,width,height,int(static_cast<unsigned char>(value[i]))-32);};
  quad(8,drawable_h-148,drawable_w-16,140,-1);text(hud_line_,16,drawable_h-36,14,26);GLsizei primary=GLsizei(glyphs.size());
  text(hud_status_,16,drawable_h-64,12,24);text(hud_combo_,16,drawable_h-92,14,24);text(hud_meter_,16,drawable_h-120,12,24);
  text(hud_grade_,16,drawable_h-190,14,24);for(size_t line=0;line<hud_trick_lines_.size();++line)text(hud_trick_lines_[line],16,drawable_h-220-float(line)*28,16,26);text(hud_points_,16,drawable_h-340,16,26);GLsizei status_end=GLsizei(glyphs.size());
  if(!hud_banner_.empty())text(hud_banner_,16,drawable_h*.55f,24,42);
  quad(16,drawable_h-142,200*float(hud_meter_fraction_),6,-2);if(!hud_combo_.empty())quad(250,drawable_h-142,200*float(hud_combo_fraction_),6,-2);
  quad(8,8,drawable_w-16,34,-1);text(hud_controls_,16,10,12,24);
  glBindVertexArray(hud_vao_);glBindBuffer(GL_ARRAY_BUFFER,hud_vbo_);glBufferData(GL_ARRAY_BUFFER,glyphs.size()*sizeof(HUDVertex),glyphs.data(),GL_DYNAMIC_DRAW);
  glDrawArrays(GL_TRIANGLES,0,primary);vec3(hud_program_,"uInk",spots[palette]);glDrawArrays(GL_TRIANGLES,primary,status_end-primary);vec3(hud_program_,"uInk",inks[palette]);glDrawArrays(GL_TRIANGLES,status_end,GLsizei(glyphs.size())-status_end);glDisable(GL_BLEND);
 }
 if(developer_overlay_visible_&&!first_person_view_)draw_developer_overlay(drawable_w,drawable_h);
 } // output copy + HUD + developer overlay
 gl_check("render");
}
void Renderer::draw_developer_overlay(int width,int height){
 if(width<=0||height<=0)return;
 glBindVertexArray(developer_overlay_vao_);
 if(developer_overlay_dirty_||width!=developer_overlay_width_||height!=developer_overlay_height_){
  struct GlyphVertex{float x,y,u,v;};static_assert(sizeof(GlyphVertex)==16);
  std::vector<GlyphVertex> vertices;vertices.reserve((developer_overlay_rows*developer_overlay_columns+1)*6);
  const float scale=std::min(width/1024.f,height/768.f);
  const float left=8*scale,bottom=62*scale,panel_width=1008*scale,panel_height=422*scale;
  const float corners[6][2]={{0,0},{1,0},{0,1},{0,1},{1,0},{1,1}};
  auto quad=[&](float x,float y,float w,float h,int character){for(const auto& p:corners)vertices.push_back({x+p[0]*w,y+p[1]*h,character<0?-1.f:(character+p[0])/96.f,character<0?0.f:1.f-p[1]});};
  quad(left,bottom,panel_width,panel_height,-1);
  for(size_t row=0;row<developer_overlay_lines_.size();++row){const auto& text=developer_overlay_lines_[row];for(size_t col=0;col<std::min(text.size(),developer_overlay_columns);++col){unsigned char c=text[col];if(c<32||c>127)c='?';quad(left+(8+col*12)*scale,bottom+panel_height-(30+row*22)*scale,12*scale,20*scale,int(c)-32);}}
  glBindBuffer(GL_ARRAY_BUFFER,developer_overlay_vbo_);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(GlyphVertex),vertices.data(),GL_DYNAMIC_DRAW);
  developer_overlay_vertices_=GLsizei(vertices.size());developer_overlay_width_=width;developer_overlay_height_=height;developer_overlay_dirty_=false;
 }
 glUseProgram(hud_program_);glUniform2f(loc(hud_program_,"uOutputRes"),width,height);
 glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
 glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,hud_font_);integer(hud_program_,"uFont",0);
 vec3(hud_program_,"uInk",{.05,.08,.12});vec3(hud_program_,"uPaper",{.97,.98,1});
 glDrawArrays(GL_TRIANGLES,0,developer_overlay_vertices_);glDisable(GL_BLEND);
}
void Renderer::capture(const std::string& path){
 auto save=[&](GLuint fbo,const std::string& filename){
 glBindFramebuffer(GL_FRAMEBUFFER,fbo);glPixelStorei(GL_PACK_ALIGNMENT,1);std::vector<unsigned char> pixels(size_t(width_)*height_*4);glReadPixels(0,0,width_,height_,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());gl_check("readback");
 SDL_Surface* s=SDL_CreateRGBSurfaceWithFormat(0,width_,height_,32,SDL_PIXELFORMAT_ABGR8888);if(!s)throw std::runtime_error(SDL_GetError());
 for(int y=0;y<height_;++y)std::memcpy(static_cast<unsigned char*>(s->pixels)+y*s->pitch,pixels.data()+size_t(height_-1-y)*width_*4,width_*4);
 int status=SDL_SaveBMP(s,filename.c_str());SDL_FreeSurface(s);glBindFramebuffer(GL_FRAMEBUFFER,0);if(status!=0)throw std::runtime_error(SDL_GetError());
 };
 save(post_bypass_?scene_fbo_:print_fbo_,path);
 const auto parent=std::filesystem::path(path).parent_path().string();
 save(scene_fbo_,parent+"/scene.bmp");
 glBindFramebuffer(GL_FRAMEBUFFER,print_fbo_);glViewport(0,0,width_,height_);glUseProgram(copy_);glDisable(GL_DEPTH_TEST);
 glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,color_);integer(copy_,"uC",0);
 glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,depth_);integer(copy_,"uD",1);
 integer(copy_,"uMode",5);glUniform2f(loc(copy_,"uOutputRes"),width_,height_);glBindVertexArray(fullscreen_vao_);glDrawArrays(GL_TRIANGLES,0,3);gl_check("depth diagnostic");
 save(print_fbo_,parent+"/depth.bmp");
}
Renderer::CellMesh Renderer::upload_cell(const CpuMesh& mesh){
 CellMesh cell;cell.indices=GLsizei(mesh.indices.size());cell.vertices=mesh.vertices.size();glGenVertexArrays(1,&cell.vao);glBindVertexArray(cell.vao);glGenBuffers(1,&cell.vbo);glBindBuffer(GL_ARRAY_BUFFER,cell.vbo);glBufferData(GL_ARRAY_BUFFER,mesh.vertices.size()*sizeof(Vertex),mesh.vertices.data(),GL_STATIC_DRAW);glGenBuffers(1,&cell.ibo);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,cell.ibo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,mesh.indices.size()*sizeof(uint32_t),mesh.indices.data(),GL_STATIC_DRAW);
 const int sizes[]={3,3,1,2,2},offsets[]={0,12,24,28,36};for(int i=0;i<5;++i){glEnableVertexAttribArray(i);glVertexAttribPointer(i,sizes[i],GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(intptr_t(offsets[i])));}return cell;
}
void Renderer::update_dynamic(CellMesh& cell,const CpuMesh& mesh){
 if(!cell.vao){cell=upload_cell(mesh);return;}
 cell.indices=GLsizei(mesh.indices.size());cell.vertices=mesh.vertices.size();glBindVertexArray(cell.vao);glBindBuffer(GL_ARRAY_BUFFER,cell.vbo);glBufferData(GL_ARRAY_BUFFER,mesh.vertices.size()*sizeof(Vertex),mesh.vertices.data(),GL_STREAM_DRAW);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,cell.ibo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,mesh.indices.size()*sizeof(uint32_t),mesh.indices.data(),GL_STREAM_DRAW);
}
void Renderer::delete_cell(CellMesh& cell){glDeleteVertexArrays(1,&cell.vao);glDeleteBuffers(1,&cell.vbo);glDeleteBuffers(1,&cell.ibo);cell={};}
Renderer::~Renderer(){
 for(auto& entry:cell_meshes_)delete_cell(entry.second);
 for(auto& entry:object_meshes_)delete_cell(entry.second);
 delete_cell(particle_mesh_);delete_cell(trail_mesh_);
 if(developer_overlay_vao_)glDeleteVertexArrays(1,&developer_overlay_vao_);
 if(developer_overlay_vbo_)glDeleteBuffers(1,&developer_overlay_vbo_);
 if(hud_vao_)glDeleteVertexArrays(1,&hud_vao_);
 if(hud_vbo_)glDeleteBuffers(1,&hud_vbo_);
 if(hud_program_)glDeleteProgram(hud_program_);
 if(hud_font_)glDeleteTextures(1,&hud_font_);
 if(rider_vao_)glDeleteVertexArrays(1,&rider_vao_);
 if(rider_vbo_)glDeleteBuffers(1,&rider_vbo_);
 if(rider_ibo_)glDeleteBuffers(1,&rider_ibo_);
 delete_targets();if(glyph_)glDeleteTextures(1,&glyph_);if(vbo_)glDeleteBuffers(1,&vbo_);if(ibo_)glDeleteBuffers(1,&ibo_);if(vao_)glDeleteVertexArrays(1,&vao_);if(fullscreen_vao_)glDeleteVertexArrays(1,&fullscreen_vao_);
 if(scene_) glDeleteProgram(scene_);
 if(post_) glDeleteProgram(post_);
 if(copy_) glDeleteProgram(copy_);
}
}
