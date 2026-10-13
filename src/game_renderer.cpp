#include "renderer.hpp"
#include "gl_api.hpp"
#include <iostream>
namespace fl {
void Renderer::update_player(Game& game,double dt){
 hud_line_="SPEED "+std::to_string(int(length(game.velocity)*3.6))+" KM/H   DIST "+std::to_string(int(game.distance))+" M   SCORE "+std::to_string(game.score);
 hud_status_=game.bail_time>0?"BAIL - RECOVERING":game.airborne?"AIR: LEFT/RIGHT SPIN, UP/DOWN FLIP; RELEASE TO COMPLETE":"RIDE: UP TUCK, DOWN BRAKE; LAST LANDING +"+std::to_string(game.last_points);
 int cell=int(std::floor(game.position.z/CELL));
 if(cell!=patch_first_+1){
  patch_first_=cell-1;patch_count_=4;CpuMesh mesh;
  for(int c=patch_first_;c<patch_first_+patch_count_;++c){auto part=build_terrain(game.terrain,c);uint32_t offset=uint32_t(mesh.vertices.size());mesh.vertices.insert(mesh.vertices.end(),part.vertices.begin(),part.vertices.end());for(auto i:part.indices)mesh.indices.push_back(offset+i);}
  vertices_=mesh.vertices.size();indices_=mesh.indices.size();glBindVertexArray(vao_);glBindBuffer(GL_ARRAY_BUFFER,vbo_);glBufferData(GL_ARRAY_BUFFER,mesh.vertices.size()*sizeof(Vertex),mesh.vertices.data(),GL_STATIC_DRAW);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ibo_);glBufferData(GL_ELEMENT_ARRAY_BUFFER,mesh.indices.size()*4,mesh.indices.data(),GL_STATIC_DRAW);
  std::cout<<"terrain patch "<<patch_first_<<".."<<patch_first_+3<<'\n';
 }
 Vec3 f{std::sin(game.yaw),0,std::cos(game.yaw)};
 Vec3 x=qrot(game.orientation,{1,0,0}),y=qrot(game.orientation,{0,1,0}),z=qrot(game.orientation,{0,0,1});
 double pose=clamp(game.crouch,0,1)*2;int a=std::min(1,int(pose));double mix=pose-a;std::vector<Vertex> vertices=rider_poses_[a];
 for(size_t i=0;i<vertices.size();++i){const auto& b=rider_poses_[a+1][i];auto& v=vertices[i];Vec3 p{lerp(v.px,b.px,mix),lerp(v.py,b.py,mix),lerp(v.pz,b.pz,mix)},n{lerp(v.nx,b.nx,mix),lerp(v.ny,b.ny,mix),lerp(v.nz,b.nz,mix)};p=game.position+x*p.x+y*p.y+z*p.z;n=norm(x*n.x+y*n.y+z*n.z);v.px=p.x;v.py=p.y;v.pz=p.z;v.nx=n.x;v.ny=n.y;v.nz=n.z;}
 glBindVertexArray(rider_vao_);glBindBuffer(GL_ARRAY_BUFFER,rider_vbo_);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STREAM_DRAW);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,rider_ibo_);glBufferData(GL_ELEMENT_ARRAY_BUFFER,rider_triangles_.size()*4,rider_triangles_.data(),GL_STREAM_DRAW);rider_indices_=GLsizei(rider_triangles_.size());
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
}
