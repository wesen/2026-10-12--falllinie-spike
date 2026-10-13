#include "renderer.hpp"
#include "gl_api.hpp"
#include <iostream>
#include <chrono>
namespace fl {
void Renderer::update_player(Game& game,double dt){
 // The ASCII source-font atlas deliberately transliterates only known UTF-8
 // labels for display. Score/repetition labels retain original Unicode.
 auto ascii=[](std::string s){for(auto replacement:std::array<std::array<std::string,2>,4>{{{{" · "," / "}},{{"×","x"}},{{"Über","Uber"}},{{"→","->"}}}}){size_t at;while((at=s.find(replacement[0]))!=std::string::npos)s.replace(at,replacement[0].size(),replacement[1]);}return s;};
 hud_line_="SCORE "+std::to_string(game.score)+"   BEST "+std::to_string(game.best)+"   "+std::to_string(int(length(game.velocity)*3.6+.5))+" KM/H   "+std::to_string(int(game.distance))+" M";
 hud_status_="Air "+std::to_string(game.airborne?game.air_time:0).substr(0,3)+" s   Grade "+std::to_string(int((.30+.07*std::sin(game.position.z/190)+.04*std::sin(game.position.z/77+1.3))*100+.5))+" %";
 hud_grade_=game.hud.live_time>0?"":game.hud.result_time>0?game.hud.grade:"";
 hud_points_=game.hud.result_time>0&&game.hud.live_time<=0&&game.last_points?"+"+std::to_string(game.last_points)+" pts":"";
 hud_trick_lines_.clear();std::string label=game.hud.live_time>0?game.hud.live:game.hud.result_time>0?game.hud.result:"";
 while(!label.empty()&&hud_trick_lines_.size()<4){auto at=label.find(" · ");hud_trick_lines_.push_back(ascii(label.substr(0,at)));if(at==std::string::npos)break;label.erase(0,at+4);}
 hud_combo_=game.combo.count>1?"x"+std::to_string(std::min(10,game.combo.count))+"  "+std::to_string(game.combo.sum)+" -> "+std::to_string(game.combo.sum*std::min(10,game.combo.count)):"";
 hud_combo_fraction_=clamp(game.combo.timer/2.2,0,1);
 bool infinite=game.grotesk>0||game.uber_boost>0,ready=game.meter>=.999||infinite;
 hud_meter_=game.grotesk>0?"GROTESK / "+std::to_string(int(std::ceil(game.grotesk)))+" s":game.uber_boost>0?"Uber boost / "+std::to_string(int(std::ceil(game.uber_boost)))+" s":ready?"Uber ready / BOOST + GRAB":"Adrenaline "+std::to_string(int(game.meter*100+.5))+" %";
 hud_meter_+="   "+std::string("GROTESK").substr(0,game.letters);
 hud_meter_fraction_=infinite?1:game.meter;
 hud_banner_=game.hud.banner_time>0?ascii(game.hud.banner):"";
 hud_controls_=controller_?"A JUMP / B-X BOOST / L1-R1-L2-R2 GRABS / START PAUSE":"SPACE JUMP / SHIFT-B BOOST / J-K-U-I GRABS / P PAUSE";
 if(game.title()){
  hud_line_="FALLLINIE";hud_status_="SOURCE DEMO / BEST "+std::to_string(game.best)+" / SEED "+std::to_string(game.seed());
  hud_grade_="";hud_points_="";hud_combo_="";hud_meter_="";hud_meter_fraction_=hud_combo_fraction_=0;
  hud_trick_lines_={controller_?"A / START: DROP IN":"ENTER / SPACE: DROP IN","C: SIX COLOURS / M: SOUND","F10 / SELECT+START: QUIT"};hud_banner_="";
 }else if(game.paused()){
  hud_banner_="PAUSED";hud_status_=std::string(muted_?"MUSIC OFF":"MUSIC ON")+" / M TO TOGGLE";
  hud_controls_=controller_?"START RESUME / SELECT NEW RUN / SELECT+START QUIT":"P-ESC RESUME / R NEW RUN / F10 QUIT";
 }
 if(!game.persistence_error.empty())hud_status_="BEST SAVE FAILED / SEE LOG";
 if(game.playing()&&!game.paused()&&muted_)hud_status_+=" / MUTED";
 stream_ms_=0;auto stream_start=std::chrono::steady_clock::now();
 int cell=int(std::floor(game.position.z/CELL));bool streamed=false;
 if(world_generation_!=game.generation()){
  // Destroy the old CPU job before touching its terrain; no old-seed upload.
  terrain_job_.reset();for(auto& entry:cell_meshes_)delete_cell(entry.second);cell_meshes_.clear();for(auto& entry:object_meshes_)delete_cell(entry.second);object_meshes_.clear();world_generation_=game.generation();following_=false;
  // Source ensureCell primes the two essential cells on a new course. Only
  // this explicit restart boundary is synchronous; lookahead stays cooperative.
  for(int essential:{cell,cell+1}){cell_meshes_.emplace(essential,upload_cell(build_terrain(game.terrain,essential)));object_meshes_.emplace(essential,upload_cell(game.world.objects(essential)));}
  streamed=true;
 }
 patch_first_=cell-1;patch_count_=7;
 if(terrain_job_&&(terrain_job_->cell()<cell-1||terrain_job_->cell()>cell+5))terrain_job_.reset();
 if(!terrain_job_)for(int wanted:{cell,cell+1,cell-1,cell+2,cell+3,cell+4,cell+5})if(!cell_meshes_.count(wanted)){terrain_job_=std::make_unique<TerrainJob>(game.terrain,wanted);break;}
 if(terrain_job_){streamed=true;if(terrain_job_->step(3.5)){int ready=terrain_job_->cell();auto mesh=terrain_job_->take();cell_meshes_.emplace(ready,upload_cell(mesh));terrain_job_.reset();}}
 // One bounded original buildObjects cell per frame, separate from row job.
 for(int wanted=cell-1;wanted<=cell+5;++wanted)if(cell_meshes_.count(wanted)&&!object_meshes_.count(wanted)){object_meshes_.emplace(wanted,upload_cell(game.world.objects(wanted)));streamed=true;break;}
 auto retire=[&](auto& cells){for(auto it=cells.begin();it!=cells.end();){if(it->first<cell-1||it->first>cell+7){delete_cell(it->second);it=cells.erase(it);streamed=true;}else ++it;}};retire(cell_meshes_);retire(object_meshes_);
 vertices_=indices_=0;for(int visible=patch_first_;visible<patch_first_+patch_count_;++visible){auto found=cell_meshes_.find(visible);if(found!=cell_meshes_.end()){vertices_+=found->second.vertices;indices_+=size_t(found->second.indices);}}
 if(streamed)stream_ms_=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-stream_start).count();
 rig_=compute_rig(game);rig_valid_=true;rider_geometry_.update(rig_,rider_mesh_);rider_groups_=rider_mesh_.groups;
 first_person_eye_=rig_.head+qrot(game.visual_orientation,{0,.025,0});
 // Extension policy retained: actual head anchor, board-nose direction/up;
 // head yaw is the source rider facing sideways, not its travel/gaze camera.
 first_person_forward_=qrot(game.visual_orientation,{1,0,0});first_person_up_=qrot(game.visual_orientation,{0,1,0});
 glBindVertexArray(rider_vao_);glBindBuffer(GL_ARRAY_BUFFER,rider_vbo_);glBufferData(GL_ARRAY_BUFFER,rider_mesh_.mesh.vertices.size()*sizeof(Vertex),rider_mesh_.mesh.vertices.data(),GL_STREAM_DRAW);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,rider_ibo_);glBufferData(GL_ELEMENT_ARRAY_BUFFER,rider_mesh_.mesh.indices.size()*sizeof(uint32_t),rider_mesh_.mesh.indices.data(),GL_STREAM_DRAW);rider_indices_=GLsizei(rider_mesh_.mesh.indices.size());
 particles_=game.particles;inversion_=game.inversion;flash_=game.flash>0?1:0;speed_effect_=game.boosting||(infinite&&game.mode!=RiderMode::Bail&&length(game.velocity)>15)?1:0;
 Vec3 f=forward_of(game.yaw);
 // Source camera direction blending, offsets, damped spring and speed-dependent FOV.
 double speed=length(game.velocity);Vec3 want=game.mode==RiderMode::Bail?camera_direction_:speed>2.5?norm({game.velocity.x,0,game.velocity.z}):f;
 if(game.mode==RiderMode::Ground&&game.fakie&&speed<2.5)want=want*-1;
 want=norm(want*.7+Vec3{0,0,1}*.3);camera_direction_=norm(camera_direction_+(want-camera_direction_)*(1-std::exp(-dt*(game.airborne?1.2:2.4))));
 double port=width_<height_?1.25:1;Vec3 target=game.position-camera_direction_*((5.2+speed*.055)*port)+Vec3{0,(1.95+speed*.02)*port,0};double ground=game.terrain.height(target.x,target.z)+1.4;target.y=std::max(target.y,ground);Vec3 look=game.position+camera_direction_*4.5+Vec3{0,.8,0};
 if(!following_){camera_.eye=target;camera_.look=look;following_=true;}
 auto spring=[dt](double& position,double& velocity,double goal,double half){double y=(4*.693)/(half+1e-5)/2,e=1/(1+y*dt+.48*y*y*dt*dt+.235*y*y*y*dt*dt*dt),j0=position-goal,j1=velocity+j0*y;position=e*(j0+j1*dt)+goal;velocity=e*(velocity-j1*y*dt);};
 spring(camera_.eye.x,camera_velocity_.x,target.x,.09);spring(camera_.eye.z,camera_velocity_.z,target.z,.09);spring(camera_.eye.y,camera_velocity_.y,target.y,game.airborne?.16:.08);
 spring(camera_.look.x,look_velocity_.x,look.x,.06);spring(camera_.look.y,look_velocity_.y,look.y,.06);spring(camera_.look.z,look_velocity_.z,look.z,.06);camera_.eye.y=std::max(camera_.eye.y,ground-.4);
 fov_=lerp(fov_,60+std::max(0.,1-double(width_)/height_)*42+clamp(speed-8,0,30)*.5+(game.boosting?7:0)+(game.slowmo>0?-6:0),1-std::exp(-dt*3));double amplitude=game.shake*game.shake*.25;camera_shake_={(camera_rng_.next()-.5)*amplitude,(camera_rng_.next()-.5)*amplitude,(camera_rng_.next()-.5)*amplitude};camera_.vp=multiply(perspective(fov_*3.141592653589793/180,double(width_)/height_,.1,900),look_at(camera_.eye+camera_shake_,camera_.look,{0,1,0}));camera_.inverse_vp=inverse(camera_.vp);game_time_=game.time;
}
void Renderer::render_first_person(int width,int height,int palette){
 Camera saved=camera_;Vec3 saved_shake=camera_shake_;camera_shake_={};double saved_fov=fov_;bool saved_follow=following_;GLsizei saved_rider=rider_indices_;
 camera_.eye=first_person_eye_;camera_.look=first_person_eye_+first_person_forward_*20;fov_=78;
 camera_.vp=multiply(perspective(fov_*3.141592653589793/180,double(width_)/height_,.1,900),look_at(camera_.eye,camera_.look,first_person_up_));camera_.inverse_vp=inverse(camera_.vp);
 following_=false;rider_indices_=0; // Do not render the rider's own face into its view.
 try{render(width,height,0,palette);}catch(...){camera_=saved;camera_shake_=saved_shake;fov_=saved_fov;following_=saved_follow;rider_indices_=saved_rider;throw;}
 camera_=saved;camera_shake_=saved_shake;fov_=saved_fov;following_=saved_follow;rider_indices_=saved_rider;
}
}
