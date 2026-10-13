#include "world.hpp"
namespace fl {
const std::vector<Rail>& World::rails(int c){
 auto found=rails_.find(c);if(found!=rails_.end())return found->second;
 std::vector<Rail> result;
 // height below can populate/evict terrain definitions. Own this tiny cell
 // vector during nested queries so FIFO trimming cannot invalidate iteration.
 const auto features=terrain_.features(c);
 for(const auto& f:features)if(f.k==3){
  double z1=f.z+f.len;Vec3 a{Terrain::center(f.z)+f.u,0,f.z},b{Terrain::center(z1)+f.u,0,z1};
  a.y=terrain_.height(a.x,a.z)+.8;b.y=terrain_.height(b.x,b.z)+.8;double need=0;
  for(int i=1;i<14;++i){double t=i/14.;Vec3 p=a+(b-a)*t;need=std::max(need,terrain_.height(p.x,p.z)+.55-p.y);}
  a.y+=need;b.y+=need;Vec3 dv=b-a;double l=length(dv);result.push_back({a,b,dv*(1/l),l});
 }
 if(rails_.size()>=300){rails_.erase(rails_order_.front());rails_order_.pop_front();}
 rails_order_.push_back(c);return rails_.emplace(c,std::move(result)).first->second;
}
const std::vector<Tree>& World::trees(int c){
 auto found=trees_.find(c);if(found!=trees_.end())return found->second;
 std::vector<Tree> result;Rng rng{hash_int(uint32_t(c)*15485863u+3u,seed_)};
 for(int i=0;i<38;++i){
  double z=c*CELL+rng.next()*CELL;auto p=terrain_.params(z);double side=rng.next()<.5?-1:1;
  double r1=rng.next(),r2=rng.next(),u=side*(p.W/2+p.R+3+r1*r2*70),x=Terrain::center(z)+u;
  double y=terrain_.height(x,z)-.3,h=5+rng.next()*8,r=1.3+rng.next()*1.4;bool grey=rng.next()<.3;
  result.push_back({x,z,y,h,r,grey,false});
 }
 if(c>=2){int n=rng.next()<.4?1:0;for(int i=0;i<n;++i){
  double z=c*CELL+8+rng.next()*(CELL-16);auto p=terrain_.params(z);double u=(rng.next()-.5)*p.W*.75;bool clear=true;
  for(int k=c-1;k<=c+1;++k)for(const auto& f:terrain_.features(k)){
   if(f.k==1&&z>f.z-f.L-14&&z<f.z+26&&std::abs(u-f.u)<f.w+4)clear=false;
   if(f.k==2&&z>f.z-20&&z<f.z+30)clear=false;
   if(f.k==3&&z>f.z-8&&z<f.z+f.len+8&&std::abs(u-f.u)<5)clear=false;
  }
  if(!clear)continue;
  double x=Terrain::center(z)+u,y=terrain_.height(x,z)-.3,h=5+rng.next()*5,r=1.2+rng.next()*.8;
  result.push_back({x,z,y,h,r,false,true});
 }}
 if(trees_.size()>=300){trees_.erase(trees_order_.front());trees_order_.pop_front();}
 trees_order_.push_back(c);return trees_.emplace(c,std::move(result)).first->second;
}
uint32_t MeshBuilder::vertex(Vec3 p,Vec3 n,double alb,double p0,double p1,double u,double r){
 uint32_t index=uint32_t(mesh.vertices.size());mesh.vertices.push_back({float(p.x),float(p.y),float(p.z),float(n.x),float(n.y),float(n.z),float(alb),float(p0),float(p1),float(u),float(r)});return index;
}
void MeshBuilder::triangle(uint32_t a,uint32_t b,uint32_t c){mesh.indices.insert(mesh.indices.end(),{a,b,c});}
void MeshBuilder::quad(uint32_t a,uint32_t b,uint32_t c,uint32_t d){mesh.indices.insert(mesh.indices.end(),{a,b,c,a,c,d});}
void MeshBuilder::face(const std::vector<Vec3>& p,double alb,Vec3 ref){
 Vec3 n=norm(cross(p[1]-p[0],p[2]-p[0])),center{};for(Vec3 v:p)center=center+v;
 if(dot(n,center*(1./p.size())-ref)<0)n=n*-1;
 uint32_t b=uint32_t(mesh.vertices.size());
 for(Vec3 v:p)vertex(v,n,alb);
 for(uint32_t k=1;k+1<p.size();++k)triangle(b,b+k,b+k+1);
}
void MeshBuilder::box(Vec3 a,Vec3 b,double w,double h,double alb){
 Vec3 d=norm(b-a),cr=cross(d,{0,1,0});double l=length(cr);Vec3 s=l>1e-9?cr*(1/l):Vec3{1,0,0},u=cross(s,d);
 auto corner=[=](Vec3 p,double i,double j){return p+s*(i*w/2)+u*(j*h/2);};
 Vec3 a1=corner(a,-1,-1),a2=corner(a,1,-1),a3=corner(a,1,1),a4=corner(a,-1,1),b1=corner(b,-1,-1),b2=corner(b,1,-1),b3=corner(b,1,1),b4=corner(b,-1,1),m=(a+b)*.5;
 face({a1,a2,a3,a4},alb,m);face({b1,b2,b3,b4},alb,m);face({a1,b1,b4,a4},alb,m);face({a2,a3,b3,b2},alb,m);face({a4,b4,b3,a3},alb,m);face({a1,a2,b2,b1},alb,m);
}
void MeshBuilder::cone(double x,double y,double z,double r,double y0,double y1,int sides,double alb){
 std::vector<Vec3> pts;for(int k=0;k<sides;++k){double a=6.283185307179586*k/sides+x*.13;pts.push_back({x+std::cos(a)*r,y+y0,z+std::sin(a)*r});}
 for(int k=0;k<sides;++k)face({pts[k],{x,y+y1,z},pts[(k+1)%sides]},alb,{x,y+y0,z});
}
CpuMesh World::objects(int c){
 MeshBuilder mb;
 for(const auto& t:trees(c)){double al=t.grey?.5:.04;mb.box({t.x,t.y,t.z},{t.x,t.y+t.height*.3,t.z},.28,.28,.04);mb.cone(t.x,t.y,t.z,t.radius,t.height*.18,t.height*.72,6,al);mb.cone(t.x,t.y,t.z,t.radius*.72,t.height*.48,t.height,6,al);}
 for(const auto& r:rails(c)){mb.box(r.a,r.b,.1,.1,.05);int n=std::max(2,int(std::floor(r.length/5+.5)));for(int k=0;k<=n;++k){Vec3 p=r.a+(r.b-r.a)*(double(k)/n);double g=terrain_.height(p.x,p.z)-.3;mb.box({p.x,g,p.z},{p.x,p.y-.05,p.z},.07,.07,.05);}}
 for(int m=int(std::ceil(c*CELL/250));m*250<(c+1)*CELL;++m){if(m<1)continue;double z=m*250;auto p=terrain_.params(z);for(int sign:{-1,1}){double x=Terrain::center(z)+sign*(p.W/2-1),y=terrain_.height(x,z);mb.box({x,y-.4,z},{x,y+7.5,z},.16,.16,.04);mb.box({x,y+6.2,z},{x-sign*1.5,y+6.2,z},1.3,.05,.04);}}
 return std::move(mb.mesh);
}
}
