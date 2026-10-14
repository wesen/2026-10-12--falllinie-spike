#pragma once
#include "math.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace fl {
// World-space box over the exact float positions uploaded to GL.
struct Aabb {
 std::array<float,3> min{{std::numeric_limits<float>::infinity(),std::numeric_limits<float>::infinity(),std::numeric_limits<float>::infinity()}};
 std::array<float,3> max{{-std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity()}};
 bool valid()const{for(int i=0;i<3;++i)if(!std::isfinite(min[i])||!std::isfinite(max[i])||min[i]>max[i])return false;return true;}
 void add(float x,float y,float z){const float p[3]={x,y,z};for(int i=0;i<3;++i){min[i]=std::min(min[i],p[i]);max[i]=std::max(max[i],p[i]);}}
};

// True only when every corner lies outside one clip half-space by a margin.
// Clip coordinates are linear in position, so the whole box is then outside
// and its triangles cannot produce fragments. Invalid boxes are never culled.
inline bool clip_rejects(const Aabb& box,const Mat4& vp,const Vec3& origin){
 if(!box.valid())return false;
 int outside[6]={0,0,0,0,0,0};
 for(int corner=0;corner<8;++corner){
  const double p[3]={double((corner&1)?box.max[0]:box.min[0])-double(float(origin.x)),double((corner&2)?box.max[1]:box.min[1])-double(float(origin.y)),double((corner&4)?box.max[2]:box.min[2])-double(float(origin.z))};
  double c[4];for(int r=0;r<4;++r)c[r]=double(vp[r])*p[0]+double(vp[4+r])*p[1]+double(vp[8+r])*p[2]+double(vp[12+r]);
  // Generous relative slack covers float evaluation of the same transform on the GPU.
  const double slack=1e-4*(std::abs(c[3])+std::abs(c[0])+std::abs(c[1])+std::abs(c[2]))+1e-4;
  outside[0]+=c[0]< -c[3]-slack;outside[1]+=c[0]>c[3]+slack;
  outside[2]+=c[1]< -c[3]-slack;outside[3]+=c[1]>c[3]+slack;
  outside[4]+=c[2]< -c[3]-slack;outside[5]+=c[2]>c[3]+slack;
 }
 for(int plane=0;plane<6;++plane)if(outside[plane]==8)return true;
 return false;
}

// Distance from a point to the nearest point of the box (zero inside).
inline double box_distance(const Aabb& box,const std::array<float,3>& point){
 double sum=0;for(int i=0;i<3;++i){const double v=double(point[i]);const double nearest=std::clamp(v,double(box.min[i]),double(box.max[i]));sum+=(v-nearest)*(v-nearest);}
 return std::sqrt(sum);
}

// Every fragment of a box farther than fog_far (+margin) from the shading eye is
// in full fog, where the scene shader's output no longer depends on its body.
inline constexpr double fog_constant_margin=2.0;
inline bool fully_fogged(const Aabb& box,const std::array<float,3>& eye,double fog_far){
 if(!box.valid())return false;
 for(float v:eye)if(!std::isfinite(v))return false;
 return box_distance(box,eye)>fog_far+fog_constant_margin;
}

// 16-bit storage is legal only when every index value fits and the mesh is non-empty.
inline bool fits_short_indices(size_t vertices,const std::vector<uint32_t>& indices){
 if(vertices==0||vertices>65536)return false;
 for(uint32_t i:indices)if(i>65535||i>=vertices)return false;
 return true;
}
// PERF-009: explicit adaptation for meshes proven to lie entirely beyond full fog.
// For dist>=uFog.y the original tail yields (1,0,uMat/255,0) independent of the body.
inline std::string adapt_fog_constant_shader(std::string source){
 const std::string tail="float f=smoothstep(uFog.x,uFog.y,dist);\n  lum=mix(lum,1.,f); blk*=1.-f; gry*=1.-f;\n  o=vec4(clamp(lum,0.,1.),blk,uMat/255.,gry);";
 if(source.find(tail)==std::string::npos)throw std::runtime_error("original scene fog tail missing; fog-constant adaptation refused");
 const std::string main="void main(){";auto at=source.find(main);
 if(at==std::string::npos||source.find(main,at+1)!=std::string::npos)throw std::runtime_error("scene main missing or ambiguous");
 source.insert(at+main.size(),"\n  // PERF-009 uniform early-out: CPU-proven full-fog mesh; equals the original fog tail.\n  if(uFogConstant!=0){ o=vec4(1.,0.,uMat/255.,0.); return; }");
 source.insert(at,"uniform int uFogConstant;\n");
 return source;
}
}
