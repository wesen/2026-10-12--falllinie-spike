#include "shadow_bounds.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>
using V=std::array<float,3>;
V sub(V a,V b){for(int i=0;i<3;++i)a[i]-=b[i];return a;}
float dot(V a,V b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
float rayseg(V p,V l,const float* a,const float* b){
 V av{a[0],a[1],a[2]},bv{b[0],b[1],b[2]},v=sub(bv,av),w=sub(p,av);
 float bb=dot(l,v),c=dot(v,v),d=dot(l,w),e=dot(v,w),D=c-bb*bb;
 float tc=std::clamp(D<1e-5f?e/c:(e-bb*d)/D,0.f,1.f);
 V q;for(int i=0;i<3;++i)q[i]=av[i]+v[i]*tc;
 float sc=std::max(dot(sub(q,p),l),0.f);V delta;for(int i=0;i<3;++i)delta[i]=p[i]+l[i]*sc-q[i];
 return std::sqrt(dot(delta,delta));
}
int main(int argc,char** argv){
 assert(argc==2);std::ifstream in(argv[1]);assert(in.good());std::string source{std::istreambuf_iterator<char>(in),{}};
 assert(fl::adapt_shadow_shader(source,fl::ShadowMode::Original)==source);
 assert(fl::adapt_shadow_shader(source,fl::ShadowMode::Off).find("Non-parity")!=std::string::npos);
 assert(fl::adapt_shadow_shader(source,fl::ShadowMode::Bounds).find("if(uShadowCullEnabled")!=std::string::npos);
 bool threw=false;try{fl::adapt_shadow_shader("bad",fl::ShadowMode::Bounds);}catch(const std::runtime_error&){threw=true;}assert(threw);
 V light{.45f,.78f,-.5f};float n=std::sqrt(dot(light,light));for(float& x:light)x/=n;
 std::mt19937 rng(42008);std::uniform_real_distribution<float> unit(-1,1);
 size_t rejected=0,retained_shadow=0,checked=0;
 for(int rig=0;rig<1000;++rig){
  float seg[24]{};V origin{unit(rng)*8000,unit(rng)*8000,unit(rng)*8000};
  for(int i=0;i<3;++i){for(int k=0;k<3;++k){seg[8*i+k]=origin[k]+unit(rng)*2;seg[8*i+4+k]=seg[8*i+k]+unit(rng)*2;}seg[8*i+3]=.17f+i*.045f;}
  auto bounds=fl::shadow_bounds(seg,light);assert(bounds.enabled);
  for(int j=0;j<600;++j){
   V p;
   if(j<300){const float* a=seg+8*(j%3);const float* b=a+4;float t=(unit(rng)+1)*.5f,along=(unit(rng)+1)*250;
    for(int k=0;k<3;++k)p[k]=a[k]+(b[k]-a[k])*t-light[k]*along+unit(rng)*.4f;
   }else{
    p={origin[0]+unit(rng)*100,origin[1]+unit(rng)*100,origin[2]+unit(rng)*100};
   }
   bool shadow=false;for(int i=0;i<3;++i)shadow=shadow||rayseg(p,light,seg+8*i,seg+8*i+4)<seg[8*i+3]*1.2f;
   bool reject=fl::shadow_rejected(bounds,p);assert(!(reject&&shadow));rejected+=reject;retained_shadow+=shadow;++checked;
  }
  auto invalid=light;invalid[1]=0;assert(!fl::shadow_bounds(seg,invalid).enabled);
  assert(!fl::shadow_rejected(bounds,{9000,0,0}));assert(!fl::shadow_rejected(bounds,{NAN,0,0}));
  seg[0]=9000;assert(!fl::shadow_bounds(seg,light).enabled);seg[0]=NAN;assert(!fl::shadow_bounds(seg,light).enabled);
 }
 assert(rejected>100000&&retained_shadow>10000);
 std::cout<<"shadow bounds: "<<checked<<" samples, "<<rejected<<" rejected, "<<retained_shadow<<" shadow-contributing retained; zero false rejection\n";
}
