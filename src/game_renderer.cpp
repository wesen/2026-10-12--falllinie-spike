#include "renderer.hpp"
#include "gl_api.hpp"
#include <iostream>
#include <chrono>
namespace fl {
void Renderer::update_player(Game& game,double dt){
 hud_line_="FPS "+std::to_string(int(fps_+.5))+"   SPEED "+std::to_string(int(length(game.velocity)*3.6))+" KM/H   DIST "+std::to_string(int(game.distance))+" M   SCORE "+std::to_string(game.score);
 hud_status_=game.bail_time>0?"BAIL - RECOVERING":game.airborne?"AIR: LEFT/RIGHT SPIN, UP/DOWN FLIP; RELEASE TO COMPLETE":"RIDE: UP TUCK, DOWN BRAKE; LAST LANDING +"+std::to_string(game.last_points);
 stream_ms_=0;auto stream_start=std::chrono::steady_clock::now();
 int cell=int(std::floor(game.position.z/CELL));
 bool streamed=false;
 patch_first_=cell-1;
 if(terrain_job_ && (terrain_job_->cell()<cell-1 || terrain_job_->cell()>cell+3))terrain_job_.reset();
 if(!terrain_job_)for(int wanted:{cell,cell-1,cell+1,cell+2,cell+3})if(!cell_meshes_.count(wanted)){terrain_job_=std::make_unique<TerrainJob>(game.terrain,wanted);break;}
 if(terrain_job_){streamed=true;if(terrain_job_->step(1.0)){int ready=terrain_job_->cell();auto mesh=terrain_job_->take();cell_meshes_.emplace(ready,upload_cell(mesh));terrain_job_.reset();std::cout<<"cell ready "<<ready<<'\n';}}
 for(auto it=cell_meshes_.begin();it!=cell_meshes_.end();){bool reset_cell=it->first>=-1 && it->first<=2;if(!reset_cell && (it->first<cell-1 || it->first>cell+3)){delete_cell(it->second);it=cell_meshes_.erase(it);streamed=true;}else ++it;}
 vertices_=indices_=0;for(int visible=patch_first_;visible<patch_first_+patch_count_;++visible){auto found=cell_meshes_.find(visible);if(found!=cell_meshes_.end()){vertices_+=found->second.vertices;indices_+=size_t(found->second.indices);}}
 if(streamed)stream_ms_=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-stream_start).count();
 Vec3 f{std::sin(game.yaw),0,std::cos(game.yaw)};
 Vec3 x=qrot(game.orientation,{1,0,0}),y=qrot(game.orientation,{0,1,0}),z=qrot(game.orientation,{0,0,1});
 double pose=clamp(game.crouch,0,1)*2;int a=std::min(1,int(pose));double mix=pose-a;std::vector<Vertex> vertices=rider_poses_[a];
 for(size_t i=0;i<vertices.size();++i){const auto& b=rider_poses_[a+1][i];auto& v=vertices[i];Vec3 p{lerp(v.px,b.px,mix),lerp(v.py,b.py,mix),lerp(v.pz,b.pz,mix)},n{lerp(v.nx,b.nx,mix),lerp(v.ny,b.ny,mix),lerp(v.nz,b.nz,mix)};p=game.position+x*p.x+y*p.y+z*p.z;n=norm(x*n.x+y*n.y+z*n.z);v.px=p.x;v.py=p.y;v.pz=p.z;v.nx=n.x;v.ny=n.y;v.nz=n.z;}
 Vec3 low{1e9,1e9,1e9},high{-1e9,-1e9,-1e9};
 for(const auto& group:rider_groups_)if(group[2]==23)for(uint32_t k=0;k<group[1];++k){const auto& v=vertices[rider_triangles_[group[0]+k]];low={std::min(low.x,double(v.px)),std::min(low.y,double(v.py)),std::min(low.z,double(v.pz))};high={std::max(high.x,double(v.px)),std::max(high.y,double(v.py)),std::max(high.z,double(v.pz))};}
 first_person_eye_=(low+high)*.5+y*.025;first_person_forward_=x;first_person_up_=y;
 glBindVertexArray(rider_vao_);glBindBuffer(GL_ARRAY_BUFFER,rider_vbo_);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STREAM_DRAW);rider_indices_=GLsizei(rider_triangles_.size());
 // Source camera direction blending, offsets, damped spring and speed-dependent FOV.
 double speed=length(game.velocity);Vec3 want=speed>2.5?norm({game.velocity.x,0,game.velocity.z}):f;
 want=norm(want*.7+Vec3{0,0,1}*.3);camera_direction_=norm(camera_direction_+(want-camera_direction_)*(1-std::exp(-dt*(game.airborne?1.2:2.4))));
 Vec3 target=game.position-camera_direction_*(5.2+speed*.055)+Vec3{0,1.95+speed*.02,0};double ground=game.terrain.height(target.x,target.z)+1.4;target.y=std::max(target.y,ground);Vec3 look=game.position+camera_direction_*4.5+Vec3{0,.8,0};
 if(!following_){camera_.eye=target;camera_.look=look;following_=true;}
 auto spring=[dt](double& position,double& velocity,double goal,double half){double y=(4*.693)/(half+1e-5)/2,e=1/(1+y*dt+.48*y*y*dt*dt+.235*y*y*y*dt*dt*dt),j0=position-goal,j1=velocity+j0*y;position=e*(j0+j1*dt)+goal;velocity=e*(velocity-j1*y*dt);};
 spring(camera_.eye.x,camera_velocity_.x,target.x,.09);spring(camera_.eye.z,camera_velocity_.z,target.z,.09);spring(camera_.eye.y,camera_velocity_.y,target.y,game.airborne?.16:.08);
 spring(camera_.look.x,look_velocity_.x,look.x,.06);spring(camera_.look.y,look_velocity_.y,look.y,.06);spring(camera_.look.z,look_velocity_.z,look.z,.06);camera_.eye.y=std::max(camera_.eye.y,ground-.4);
 fov_=lerp(fov_,60+clamp(speed-8,0,30)*.5,1-std::exp(-dt*3));camera_.vp=multiply(perspective(fov_*3.141592653589793/180,double(width_)/height_,.1,900),look_at(camera_.eye,camera_.look,{0,1,0}));camera_.inverse_vp=inverse(camera_.vp);game_time_=game.time;
}
void Renderer::render_first_person(int width,int height,int palette){
 Camera saved=camera_;double saved_fov=fov_;bool saved_follow=following_;GLsizei saved_rider=rider_indices_;
 camera_.eye=first_person_eye_;camera_.look=first_person_eye_+first_person_forward_*20;fov_=78;
 camera_.vp=multiply(perspective(fov_*3.141592653589793/180,double(width_)/height_,.1,900),look_at(camera_.eye,camera_.look,first_person_up_));camera_.inverse_vp=inverse(camera_.vp);
 following_=false;rider_indices_=0; // Do not render the rider's own face into its view.
 try{render(width,height,0,palette);}catch(...){camera_=saved;fov_=saved_fov;following_=saved_follow;rider_indices_=saved_rider;throw;}
 camera_=saved;fov_=saved_fov;following_=saved_follow;rider_indices_=saved_rider;
}
}
