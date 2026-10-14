// PERF-009 conservativeness checks for per-view culling, fog-constant shading
// and 16-bit index eligibility. Float evaluation mirrors the GPU expressions.
#include "geometry_bounds.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>
using fl::Aabb;

static bool outside_plane_float(const fl::Mat4& vp,const float p[3],int plane){
 float c[4];for(int r=0;r<4;++r)c[r]=vp[r]*p[0]+vp[4+r]*p[1]+vp[8+r]*p[2]+vp[12+r];
 switch(plane){case 0:return c[0]< -c[3];case 1:return c[0]>c[3];case 2:return c[1]< -c[3];case 3:return c[1]>c[3];case 4:return c[2]< -c[3];default:return c[2]>c[3];}
}
static float glsl_smoothstep(float e0,float e1,float x){float t=std::clamp((x-e0)/(e1-e0),0.f,1.f);return t*t*(3.f-2.f*t);}
static int unorm8(float v){return int(std::lround(std::clamp(v,0.f,1.f)*255.f));}

int main(int argc,char** argv){
 std::mt19937 rng(9009);std::uniform_real_distribution<double> unit(-1,1);
 // 1. Clip rejection: every point of a rejected box must be outside one plane in float.
 size_t boxes=0,rejected=0,points=0;
 for(int view=0;view<400;++view){
  fl::Vec3 eye{unit(rng)*3000,unit(rng)*500,unit(rng)*3000},look=eye+fl::Vec3{unit(rng)*50,unit(rng)*20,40+unit(rng)*30};
  const auto frame=fl::render_frame(eye,look,{0,1,0},(50+unit(rng)*30)*3.14159265/180,4.0/3.0,true);
  for(int b=0;b<60;++b){
   Aabb box;fl::Vec3 centre=eye+fl::Vec3{unit(rng)*700,unit(rng)*200,unit(rng)*700};
   double half[3]={1+std::abs(unit(rng))*120,1+std::abs(unit(rng))*40,1+std::abs(unit(rng))*120};
   box.add(float(centre.x-half[0]),float(centre.y-half[1]),float(centre.z-half[2]));box.add(float(centre.x+half[0]),float(centre.y+half[1]),float(centre.z+half[2]));
   ++boxes;if(!fl::clip_rejects(box,frame.vp,frame.origin))continue;
   ++rejected;
   std::uniform_real_distribution<float> t(0,1);
   const float origin[3]={float(frame.origin.x),float(frame.origin.y),float(frame.origin.z)};
   for(int s=0;s<400;++s){
    float w[3];for(int k=0;k<3;++k){float u=s<8?float((s>>k)&1):t(rng);w[k]=box.min[k]+(box.max[k]-box.min[k])*u;}
    const float rel[3]={w[0]-origin[0],w[1]-origin[1],w[2]-origin[2]};
    bool out=false;for(int plane=0;plane<6&&!out;++plane)out=outside_plane_float(frame.vp,rel,plane);
    assert(out);++points;
   }
  }
 }
 assert(rejected>boxes/10&&rejected<boxes);
 // 2. Fog constant: flagged boxes have every sampled point at float distance >= fog far,
 //    and the original fog tail then quantizes to (255,0,uMat,0) for arbitrary bodies.
 size_t fogged=0,fog_points=0;
 for(int i=0;i<20000;++i){
  std::array<float,3> eye{float(unit(rng)*4000),float(unit(rng)*800),float(unit(rng)*4000)};
  Aabb box;double cx=eye[0]+unit(rng)*700,cy=eye[1]+unit(rng)*300,cz=eye[2]+unit(rng)*700,h=1+std::abs(unit(rng))*150;
  box.add(float(cx-h),float(cy-h/3),float(cz-h));box.add(float(cx+h),float(cy+h/3),float(cz+h));
  if(!fl::fully_fogged(box,eye,400))continue;
  ++fogged;
  std::uniform_real_distribution<float> t(0,1);
  for(int s=0;s<64;++s){
   float d2=0;for(int k=0;k<3;++k){float v=box.min[k]+(box.max[k]-box.min[k])*(s<8?float((s>>k)&1):t(rng))-eye[k];d2+=v*v;}
   const float dist=std::sqrt(d2);assert(dist>=400.f);
   const float f=glsl_smoothstep(150.f,400.f,dist);assert(f==1.f);
   const float lum=t(rng)*1.3f-.1f,blk=t(rng),gry=t(rng);
   const float mix_a=lum*(1.f-f)+1.f*f,mix_b=lum+(1.f-lum)*f; // two legal mix implementations
   assert(unorm8(mix_a)==255&&unorm8(mix_b)==255&&unorm8(blk*(1.f-f))==0&&unorm8(gry*(1.f-f))==0);
   ++fog_points;
  }
 }
 assert(fogged>100);
 // 3. Short indices: values preserved, limits enforced.
 std::vector<uint32_t> idx{0,1,2,65535,2,1};
 assert(fl::fits_short_indices(65536,idx));
 assert(!fl::fits_short_indices(65537,idx));
 assert(!fl::fits_short_indices(0,{}));
 assert(!fl::fits_short_indices(100,{0,1,100}));
 std::vector<uint16_t> narrow(idx.begin(),idx.end());for(size_t i=0;i<idx.size();++i)assert(uint32_t(narrow[i])==idx[i]);
 // 4. The adaptation is anchored on the original fog tail and leaves the body intact.
 assert(argc==2);std::ifstream in(argv[1]);assert(in.good());std::string source{std::istreambuf_iterator<char>(in),{}};
 const auto adapted=fl::adapt_fog_constant_shader(source);
 assert(adapted.find("if(uFogConstant!=0){ o=vec4(1.,0.,uMat/255.,0.); return; }")!=std::string::npos);
 std::string restored=adapted;
 for(const std::string& piece:{std::string("uniform int uFogConstant;\n"),std::string("\n  // PERF-009 uniform early-out: CPU-proven full-fog mesh; equals the original fog tail.\n  if(uFogConstant!=0){ o=vec4(1.,0.,uMat/255.,0.); return; }")}){auto at=restored.find(piece);assert(at!=std::string::npos);restored.erase(at,piece.size());}
 assert(restored==source);
 bool threw=false;try{fl::adapt_fog_constant_shader("void main(){}");}catch(const std::runtime_error&){threw=true;}assert(threw);
 std::cout<<"geometry bounds: boxes="<<boxes<<" rejected="<<rejected<<" rejected_points="<<points<<" fogged_boxes="<<fogged<<" fog_points="<<fog_points<<" OK\n";
}
