#include "terrain.hpp"
#include <chrono>
#include <limits>
#include <stdexcept>
namespace fl {
uint32_t hash_int(uint32_t n,uint32_t seed){uint32_t h=n^seed;h=(h^(h>>16))*0x7feb352dU;h=(h^(h>>15))*0x846ca68bU;return h^(h>>16);}
double Rng::next(){a+=0x6D2B79F5U;uint32_t t=(a^(a>>15))*(1U|a);t=(t+(t^(t>>7))*(61U|t))^t;return double(t^(t>>14))/4294967296.0;}
double Terrain::h2(int x,int z)const{uint32_t h=uint32_t(x)*0x27d4eb2dU ^ uint32_t(z)*0x165667b1U ^ seed_;h=(h^(h>>15))*0x2c1b3c6dU;h=(h^(h>>12))*0x297a2d39U;h^=h>>15;return double(h)/4294967296.0;}
double Terrain::noise(double x,double z)const{
 int ix=int(std::floor(x)),iz=int(std::floor(z));double fx=x-ix,fz=z-iz,ux=fx*fx*(3-2*fx),uz=fz*fz*(3-2*fz);
 double a=h2(ix,iz),b=h2(ix+1,iz),c=h2(ix,iz+1),d=h2(ix+1,iz+1);
 return (a+(b-a)*ux+(c-a)*uz+(a-b-c+d)*ux*uz)*2-1;
}
double Terrain::fbm(double x,double z,int octaves)const{
 double sum=0,a=0.5,f=1,n=0;for(int i=0;i<octaves;++i){sum+=a*noise(x*f+i*17.3,z*f-i*9.1);n+=a;a*=0.5;f*=2.03;}return sum/n;
}
double Terrain::base(double z){return -(0.30*z-0.07*190*std::cos(z/190)-0.04*77*std::cos(z/77+1.3));}
double Terrain::center(double z){return 55*std::sin(z/310)+22*std::sin(z/127+2.1)+8*std::sin(z/53+0.4);}
Params Terrain::cell_params(int c){
 auto found=params_.find(c);if(found!=params_.end())return found->second;
 Rng r{hash_int(uint32_t(c)*7919U+13U,seed_)};Params p;
 if(c<=0)p={46,15,5,false};
 else if(c>2 && r.next()<0.17){p={15+r.next()*4,5.2+r.next()*0.8,6.6,true};}
 else p={34+r.next()*24,11+r.next()*8,3.6+r.next()*4,false};
 params_[c]=p;return p;
}
Params Terrain::params(double z){
 int c=int(std::floor(z/CELL));double t=z/CELL-c;Params a=cell_params(c);double k=smooth(0.72,1,t);if(k<=0)return a;
 Params b=cell_params(c+1);return {lerp(a.W,b.W,k),lerp(a.R,b.R,k),lerp(a.cap,b.cap,k),k<0.5?a.pipe:b.pipe};
}
const std::vector<Feature>& Terrain::features(int c){
 auto found=features_.find(c);if(found!=features_.end())return found->second;
 auto& F=features_[c];if(c<1)return F;
 Rng r{hash_int(uint32_t(c)*104729U+7U,seed_)};Params P=cell_params(c);double z0=c*CELL,half=P.W/2;
 bool cliff=!P.pipe && c>1 && r.next()<0.24;
 for(int s=0;s<3;++s){
  double z=z0+14+s*32+r.next()*8,q=r.next();
  if(cliff && s==2){Feature f;f.k=2;f.z=z+6;f.D=4+r.next()*5;f.face=1.6;F.push_back(f);continue;}
  if(cliff && s==1){if(q<0.45){Feature f;f.k=3;f.z=z-6;f.u=(r.next()-0.5)*(P.W-10);f.len=14+r.next()*8;F.push_back(f);}continue;}
  auto kick=[&](bool mega){Feature f;f.k=1;f.mega=mega;
   if(mega){f.z=z;f.u=(r.next()-0.5)*0.3*P.W;f.H=4.4+r.next()*1.4;f.L=15+r.next()*4;f.w=11+r.next()*3;}
   else {f.w=5+r.next()*4;double span=std::max(0.0,half-f.w/2-3);f.z=z+(r.next()-0.5)*6;f.u=(r.next()*2-1)*span;f.H=1.5+r.next()*1.7;f.L=7+r.next()*5;}
   return f;
  };
  auto rail=[&](double width,double minlen,double delta){Feature f;f.k=3;f.z=z;f.u=(r.next()-0.5)*width;f.len=minlen+r.next()*delta;F.push_back(f);};
  if(P.pipe){if(q<0.45)rail(P.W-6,16,12);else if(q<0.72)F.push_back(kick(false));}
  else {if(q<0.46){F.push_back(kick(false));if(r.next()<0.4){Feature f2=kick(false);if(std::abs(f2.u-F.back().u)>9)F.push_back(f2);}}
   else if(q<0.61 && P.W>32)F.push_back(kick(true));else if(q<0.86)rail(P.W-10,14,14);
  }
 }
 return F;
}
double Terrain::drop(double z){
 int c=int(std::floor(z/CELL));
 while(c>0 && int(cumulative_.size())<=c){int k=int(cumulative_.size())-1;double d=0;for(const auto& f:features(k))if(f.k==2)d+=f.D;cumulative_.push_back(cumulative_[k]-d);}
 double off=c>0?cumulative_[c]:0;
 for(const auto& f:features(c))if(f.k==2 && z>f.z)off-=f.D*smooth(0,f.face,z-f.z);
 return off;
}
double Terrain::height(double x,double z,Paint* out){
 double u=x-center(z);Params P=params(z);double d=std::abs(u)-P.W/2,rel=0;
 if(d>0){double d1=0.92*P.R;rel=d<d1?P.R-std::sqrt(P.R*P.R-d*d):P.R*0.6081+(d-d1)*2.35;
  double k=1.2,hh=clamp(0.5+0.5*(P.cap-rel)/k,0,1);rel=lerp(P.cap,rel,hh)-k*hh*(1-hh);double o=std::max(0.0,d-P.R-4);rel+=0.0022*o*o+0.09*o;
 }
 rel+=0.9*fbm(x/40,z/55,2);if(d>8)rel+=fbm(x/110,z/110,3)*30*smooth(10,90,d);
 int c=int(std::floor(z/CELL));
 for(int cell=c-1;cell<=c+1;++cell)for(const auto& f:features(cell)){
  double a=z-f.z;
  if(f.k==1){if(a< -f.L || a>1.3)continue;double b=std::abs(u-f.u),lat=1-smooth(f.w*0.5-1.2,f.w*0.5+1.2,b);if(lat<=0)continue;
   double y=a<=0?f.H*std::pow((a+f.L)/f.L,2):f.H*(1-smooth(0,1.3,a));
   if(out && a> -1 && a<0.4 && lat>0.55) out->lip=1;
   rel+=y*lat;
  }else if(f.k==2 && out && a> -2.8 && a<=0.05)out->stripe=1;
 }
 if(out){out->rel=rel;out->u=u;}return base(z)+drop(z)+rel;
}
Vec3 Terrain::normal(double x,double z){double e=0.08,hx=height(x+e,z)-height(x-e,z),hz=height(x,z+e)-height(x,z-e);return norm({-hx/(2*e),1,-hz/(2*e)});}
TerrainJob::TerrainJob(Terrain& terrain,int cell):terrain_(terrain),cell_(cell){
 for(double u:{300,240,185,140,105,80,64,54,48})columns_.push_back(-u);
 for(int u=-44;u<=44;++u)columns_.push_back(u);
 for(double u:{48,54,64,80,105,140,185,240,300})columns_.push_back(u);
 samples_.reserve(ROWS+3);mesh_.vertices.reserve((ROWS+1)*columns_.size());mesh_.indices.reserve(ROWS*(columns_.size()-1)*6);
}
bool TerrainJob::step(double budget_ms){
 if(complete_)return true;
 const auto started=std::chrono::steady_clock::now();
 do{
  if(sampling_){double z=cell_*CELL+row_*ROW,cx=Terrain::center(z);std::vector<Sample> row;row.reserve(columns_.size());
   for(double u:columns_){Sample s;s.p={cx+u,0,z};s.p.y=terrain_.height(s.p.x,z,&s.o);row.push_back(s);}samples_.push_back(std::move(row));
   if(++row_>ROWS+1){sampling_=false;row_=1;}
  }else{
   int j=row_;for(int i=0;i<int(columns_.size());++i){const auto& s=samples_[j][i];Vec3 n=norm(cross(samples_[j+1][i].p-samples_[j-1][i].p,samples_[j][std::min(int(columns_.size())-1,i+1)].p-samples_[j][std::max(0,i-1)].p));if(n.y<0)n=n*-1;
    mesh_.vertices.push_back({float(s.p.x),float(s.p.y),float(s.p.z),float(n.x),float(n.y),float(n.z),.97f,float(s.o.lip),float(s.o.stripe),float(s.o.u),float(s.o.rel)});
   }
   if(j<=ROWS){uint32_t nc=uint32_t(columns_.size());for(uint32_t i=0;i<nc-1;++i){uint32_t a=(j-1)*nc+i,b=a+1,c=a+nc,d=c+1;mesh_.indices.insert(mesh_.indices.end(),{a,c,b,b,c,d});}}
   if(++row_>ROWS+1)complete_=true;
  }
 }while(!complete_ && std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count()<budget_ms);
 return complete_;
}
CpuMesh TerrainJob::take(){if(!complete_)throw std::runtime_error("terrain job is incomplete");return std::move(mesh_);}
CpuMesh build_terrain(Terrain& terrain,int cell){TerrainJob job(terrain,cell);job.step(std::numeric_limits<double>::infinity());return job.take();}
}
