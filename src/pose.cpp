#include "pose.hpp"
namespace fl {
IkResult ik2(Vec3 a,Vec3 target,double l1,double l2,Vec3 pole){
 Vec3 dv=target-a;double dl=length(dv),d=clamp(dl,std::abs(l1-l2)+1e-3,l1+l2-1e-3);Vec3 dir=dl>1e-6?dv*(1/dl):Vec3{0,-1,0};
 double ca=clamp((l1*l1+d*d-l2*l2)/(2*l1*d),-1,1),sa=std::sqrt(1-ca*ca);Vec3 pp=source_norm(pole-dir*dot(pole,dir),{0,0,1});return {a+dir*(l1*ca)+pp*(l1*sa),a+dir*d};
}
Pose base_pose(double c,double lean,double steer,int grab,double tweak,double amount,double look){
 const auto* g=grab_definition(grab);double tw0=g?g->twist:0,tw1=g?g->tilt:0;Pose p;
 p.board=qmul(qaxis({1,0,0},lean*.35+tweak*tw0),qaxis({0,0,1},tweak*tw1));p.board_position={0,.3*amount,0};
 p.pelvis={0,.8-.26*c-.1*amount+.05*std::abs(tweak*tw1),-.02+.25*std::sin(lean)-.04*c};
 p.torso=qmul(qaxis({0,1,0},.3-.15*c),qaxis({1,0,0},.18+.4*c+lean*.8+.25*amount));p.head=qmul(qaxis({0,1,0},look),qaxis({1,0,0},.1-.2*c));
 p.hands={Vec3{.56,1-.28*c+.12*steer,.22+lean*.3},Vec3{-.56,.93-.28*c-.12*steer,.02+lean*.3}};
 auto mix=[](Vec3 a,Vec3 b,double t){return a+(b-a)*t;};
 if(g){if(g->left)p.hands[0]=mix(p.hands[0],p.board_position+qrot(p.board,*g->left),amount);else p.hands[0]=mix(p.hands[0],{.6,.95,-.1},amount*.6);
 if(g->right)p.hands[1]=mix(p.hands[1],p.board_position+qrot(p.board,*g->right),amount);else p.hands[1]=mix(p.hands[1],{-.62,1.1,.2},amount*.6);}
 return p;
}
Pose bind_feet(Pose p){p.feet={p.board_position+qrot(p.board,{.25,.1,0}),p.board_position+qrot(p.board,{-.25,.1,0})};return p;}
Pose uber_pose(int id,double t){
 Pose p;p.pelvis={0,.85,0};p.head=qaxis({0,1,0},1.1);p.hands={Vec3{.6,1.2,.2},Vec3{-.6,1.2,.2}};p.feet={Vec3{.25,.1,0},Vec3{-.25,.1,0}};
 if(id==1){p.board_position={0,2.08,.05};p.board=qaxis({0,1,0},t*TAU*3.2);p.hands={Vec3{.75,1.25,.35},Vec3{0,1.99,.05}};p.feet={Vec3{.22,.12+.06*std::sin(t*22),.1},Vec3{-.22,.12-.06*std::sin(t*22),.1}};p.pelvis={0,.92,0};p.torso=qaxis({1,0,0},-.1);}
 else if(id==2){p.pelvis={-.72,.5,0};p.torso=qmul(qaxis({0,0,1},-PI/2+.15),qaxis({0,1,0},PI/2));p.head=qaxis({1,0,0},-.6);p.hands={Vec3{.2,.05,.13},Vec3{.2,.05,-.13}};p.feet={Vec3{-1.55,.62+.1*std::sin(t*9),.14},Vec3{-1.55,.62-.1*std::sin(t*9),-.14}};p.knee_pole=Vec3{0,1,0};}
 else if(id==4){double k=smooth(.1,.85,t);p.board=qaxis({1,0,0},k*TAU*2);p.board_position={0,-.05-.25*std::sin(PI*t),0};p.feet={Vec3{.25,.5,.1},Vec3{-.25,.5,.1}};p.pelvis={0,.95,0};p.hands={Vec3{.7,1.4,.1},Vec3{-.7,1.4,.1}};p.torso=qaxis({1,0,0},.3);}
 else if(id==8){p.board_position={0,2,0};p.hands={Vec3{.28,1.97,0},Vec3{-.28,1.97,0}};p.pelvis={0,1,0};p.feet={Vec3{.12,.72,.85},Vec3{-.12,.72,-.85}};p.knee_pole=Vec3{};p.torso=qaxis({1,0,0},-.05);}
 else if(id==3){double ph=t*TAU*3;p.pelvis={0,1.12,0};p.torso=qaxis({0,1,0},PI/2);p.head={};p.feet={Vec3{.35*std::cos(ph),.3+.2*std::max(0.,std::sin(ph)),.1},Vec3{-.35*std::cos(ph),.3+.2*std::max(0.,-std::sin(ph)),-.1}};p.hands={Vec3{-.3*std::cos(ph),1.2,.3},Vec3{.3*std::cos(ph),1.2,-.3}};p.knee_pole=Vec3{1,0,0};}
 else if(id==12){double a=t*TAU*2;p.board_position={std::cos(a)*1.15,.95,std::sin(a)*1.15};p.board=qmul(qaxis({0,1,0},-a),qaxis({1,0,0},PI/2));p.hands={Vec3{.95,1.35,0},Vec3{-.95,1.35,0}};p.feet={Vec3{.08,.1,0},Vec3{-.08,.1,0}};p.pelvis={0,.9,0};p.torso=qaxis({1,0,0},-.15);}
 return p;
}
Pose blend_pose(const Pose& a,const Pose& b,double w){
 auto mix=[w](Vec3 a,Vec3 b){return a+(b-a)*w;};Pose p;p.pelvis=mix(a.pelvis,b.pelvis);p.torso=qslerp(a.torso,b.torso,w);p.head=qslerp(a.head,b.head,w);p.board_position=mix(a.board_position,b.board_position);p.board=qslerp(a.board,b.board,w);
 for(int i=0;i<2;++i){p.hands[i]=mix(a.hands[i],b.hands[i]);p.feet[i]=mix(a.feet[i],b.feet[i]);}p.knee_pole=w>.5?b.knee_pole:a.knee_pole;return p;
}
RiderRig compute_rig(const Game& game){
 Pose p;if(game.mode==RiderMode::Bail){p=bind_feet(base_pose(.9,0,0,0,0,0,0));p.pelvis={0,.42,-.2};p.torso=qaxis({1,0,0},1.3);p.hands={Vec3{.7,.5,.6},Vec3{-.8,.3,-.3}};}
 else {bool air=game.mode==RiderMode::Air;p=bind_feet(base_pose(game.crouch,game.mode==RiderMode::Ground?game.lean:0,game.steer_visual,air?game.air.grab:0,air?game.air.tweak:0,game.grab_amount,game.fakie?-1.15:1.15));
 if(air&&game.air.uber){const auto& u=*game.air.uber;double t=u.t/u.duration,w=smooth(0,.16,t)*(1-smooth(.84,1,t));p=blend_pose(p,uber_pose(u.id,t),w);}}
 RiderRig rig;rig.origin=game.position+(game.mode==RiderMode::Bail?Vec3{0,.45,0}:Vec3{});rig.orientation=game.visual_orientation;auto world=[&](Vec3 v){return rig.origin+qrot(rig.orientation,v);};
 Quat hipq=qslerp({},p.torso,.35),headq=qmul(p.torso,p.head);Vec3 chest=p.pelvis+qrot(p.torso,{0,.44,0}),neck=p.pelvis+qrot(p.torso,{0,.57,0}),head=neck+qrot(headq,{0,.14,.03});
 rig.pelvis=world(p.pelvis);rig.chest=world(chest);rig.neck=world(neck);rig.head=world(head);rig.head_orientation=qmul(rig.orientation,headq);rig.board_position=world(p.board_position);rig.board_orientation=qmul(rig.orientation,p.board);
 for(int i=0;i<2;++i){double sign=i==0?1:-1;Vec3 hip=p.pelvis+qrot(hipq,{sign*.1,-.05,0}),sh=p.pelvis+qrot(p.torso,{sign*.19,.5,0});auto leg=ik2(hip,p.feet[i],.44,.45,p.knee_pole.value_or(Vec3{sign*.3,.2,1})),arm=ik2(sh,p.hands[i],.3,.28,{sign*.5,-1,-.5});rig.hip[i]=world(hip);rig.knee[i]=world(leg.mid);rig.ankle[i]=world(leg.end);rig.shoulder[i]=world(sh);rig.elbow[i]=world(arm.mid);rig.hand[i]=world(arm.end);}
 return rig;
}
RiderGeometry::RiderGeometry(){
 MeshBuilder cyl;for(int k=0;k<=10;++k){double a=TAU*k/10;Vec3 n{std::cos(a),0,std::sin(a)};cyl.vertex(n,n,1);cyl.mesh.vertices.back().py=0;cyl.vertex({n.x,1,n.z},n,1);}for(int k=0;k<10;++k){uint32_t a=k*2;cyl.triangle(a,a+1,a+2);cyl.triangle(a+1,a+3,a+2);}cylinder_=std::move(cyl.mesh);
 MeshBuilder sph;for(int j=0;j<=9;++j){double v=j/9.*PI;for(int i=0;i<=14;++i){double u=i/14.*TAU;Vec3 n{std::sin(v)*std::cos(u),std::cos(v),std::sin(v)*std::sin(u)};sph.vertex(n,n,1);}}for(int j=0;j<9;++j)for(int i=0;i<14;++i){uint32_t a=j*15+i,b=a+15;sph.triangle(a,a+1,b);sph.triangle(a+1,b+1,b);}sphere_=std::move(sph.mesh);
 MeshBuilder box;box.box({0,0,-.5},{0,0,.5},1,1,1);box_=std::move(box.mesh);
 MeshBuilder board;std::array<Vec3,26> top,bot,pts;for(int k=0;k<26;++k){double a=TAU*k/26,c=std::cos(a),s=std::sin(a),x=std::copysign(std::pow(std::abs(c),.32),c)*.78,z=std::copysign(std::pow(std::abs(s),.6),s)*(.145-.012*(1-std::abs(c))),e=std::max(0.,std::abs(x)-.55)/.23;pts[k]={x,0,z};top[k]={x,.028+e*e*.075,z};bot[k]={x,e*e*.075,z};}
 auto ct=board.vertex({0,.028,0},{0,1,0},.97);uint32_t t0=uint32_t(board.mesh.vertices.size());for(Vec3 p:top)board.vertex(p,{0,1,0},.97);for(int k=0;k<26;++k)board.triangle(ct,t0+(k+1)%26,t0+k);
 auto cb=board.vertex({0,0,0},{0,-1,0},.05);uint32_t b0=uint32_t(board.mesh.vertices.size());for(Vec3 p:bot)board.vertex(p,{0,-1,0},.05);for(int k=0;k<26;++k)board.triangle(cb,b0+k,b0+(k+1)%26);
 for(int k=0;k<26;++k){int k2=(k+1)%26;Vec3 n=norm(pts[k]);auto a=board.vertex(top[k],n,.05),b=board.vertex(top[k2],n,.05),c=board.vertex(bot[k2],n,.05),d=board.vertex(bot[k],n,.05);board.quad(a,b,c,d);}
 for(auto range:std::array<std::array<double,2>,3>{{{{-.05,.05}},{{.2,.32}},{{-.32,-.2}}}}){double x0=range[0],x1=range[1],y=.031,z=.12;auto a=board.vertex({x0,y,-z},{0,1,0},.05),b=board.vertex({x1,y,-z},{0,1,0},.05),c=board.vertex({x1,y,z},{0,1,0},.05),d=board.vertex({x0,y,z},{0,1,0},.05);board.triangle(a,c,b);board.triangle(a,d,c);}board_=std::move(board.mesh);
}
void RiderGeometry::update(const RiderRig& w,RiderMesh& output)const{
 output.mesh.vertices.clear();output.mesh.indices.clear();output.groups.clear();output.mesh.vertices.reserve(6000);output.mesh.indices.reserve(26000);output.groups.reserve(32);
 auto draw=[&](const CpuMesh& m,Vec3 x,Vec3 y,Vec3 z,Vec3 o,double alb,int mat){
  uint32_t offset=uint32_t(output.mesh.vertices.size()),first=uint32_t(output.mesh.indices.size());Vec3 nx=cross(y,z),ny=cross(z,x),nz=cross(x,y);double det=dot(x,nx);if(std::abs(det)<1e-15)det=1;nx=nx*(1/det);ny=ny*(1/det);nz=nz*(1/det);
  for(auto v:m.vertices){Vec3 p=o+x*v.px+y*v.py+z*v.pz,n=norm(nx*v.nx+ny*v.ny+nz*v.nz);v.px=p.x;v.py=p.y;v.pz=p.z;v.nx=n.x;v.ny=n.y;v.nz=n.z;if(alb>=0)v.albedo=alb;output.mesh.vertices.push_back(v);}for(uint32_t i:m.indices)output.mesh.indices.push_back(offset+i);output.groups.push_back({first,uint32_t(m.indices.size()),uint32_t(mat)});
 };
 auto seg=[&](Vec3 a,Vec3 b,double r,double alb,int mat){Vec3 y=b-a;double l=length(y);if(l<1e-4)return;Vec3 yy=y*(1/l),x=norm(cross(yy,std::abs(yy.y)<.9?Vec3{0,1,0}:Vec3{1,0,0})),z=cross(x,yy);draw(cylinder_,x*r,y,z*r,a,alb,mat);};
 auto ball=[&](Vec3 c,double r,double alb,int mat){draw(sphere_,{r,0,0},{0,r,0},{0,0,r},c,alb,mat);};
 auto box=[&](Vec3 c,Quat q,double sx,double sy,double sz,double alb,int mat){draw(box_,qrot(q,{sx,0,0}),qrot(q,{0,sy,0}),qrot(q,{0,0,sz}),c,alb,mat);};
 draw(board_,qrot(w.board_orientation,{1,0,0}),qrot(w.board_orientation,{0,1,0}),qrot(w.board_orientation,{0,0,1}),w.board_position,-1,26);
 for(int i=0;i<2;++i){seg(w.hip[i],w.knee[i],.088,.07,22);seg(w.knee[i],w.ankle[i],.078,.07,22);ball(w.knee[i],.086,.07,22);box(w.ankle[i]+qrot(w.board_orientation,{0,-.04,0}),w.board_orientation,.14,.13,.31,.05,28);seg(w.shoulder[i],w.elbow[i],.066,.5,21);seg(w.elbow[i],w.hand[i],.058,.5,21);ball(w.elbow[i],.064,.5,21);ball(w.hand[i],.066,.05,27);}
 ball(w.pelvis,.16,.07,22);seg(w.pelvis,w.chest,.165,.5,20);ball(w.chest,.19,.5,20);seg(w.chest,w.head,.06,.5,20);ball(w.head,.13,.95,23);box(w.head+qrot(w.head_orientation,{0,0,.1}),w.head_orientation,.2,.065,.06,.04,25);
}
}
