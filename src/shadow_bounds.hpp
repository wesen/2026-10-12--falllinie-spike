#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

namespace fl {
enum class ShadowMode {Original, Off, Bounds};
inline const char* shadow_mode_name(ShadowMode m){return m==ShadowMode::Original?"original":m==ShadowMode::Off?"off":"bounds";}
// Engineering-qualified float domain, not a universal IEEE/GPU error proof.
inline constexpr float shadow_coordinate_limit=8192.f;
struct ShadowBounds {
 bool enabled=false;
 std::array<float,2> slope{};
 std::array<float,4> rectangle{}; // min X,Z; max X,Z in light-invariant coordinates
};
inline ShadowBounds shadow_bounds(const float* segments,const std::array<float,3>& light){
 ShadowBounds out;
 double norm=0;for(float x:light){if(!std::isfinite(x))return out;norm+=double(x)*x;}
 if(std::abs(norm-1)>1e-4||std::abs(light[1])<.01f)return out;
 out.slope={light[0]/light[1],light[2]/light[1]};
 for(float x:out.slope)if(std::abs(x)>2)return out;
 double lo[2]={INFINITY,INFINITY},hi[2]={-INFINITY,-INFINITY};
 for(int i=0;i<3;++i){
  const float* a=segments+8*i;const float* b=a+4;
  double length2=0;
  for(int j=0;j<3;++j){
   if(!std::isfinite(a[j])||!std::isfinite(b[j])||std::abs(a[j])>shadow_coordinate_limit||std::abs(b[j])>shadow_coordinate_limit)return out;
   length2+=(double(b[j])-a[j])*(double(b[j])-a[j]);
  }
  if(length2<1e-10||!std::isfinite(a[3])||a[3]<=0||a[3]>16)return out;
  for(int k=0;k<2;++k){
   const int j=k==0?0:2;const double slope=out.slope[k];
   const double x=double(a[j])-slope*a[1],y=double(b[j])-slope*b[1];
   // Radius covers smoothstep's full support; generous float safety slack.
   const double radius=double(a[3])*1.2*std::sqrt(1+slope*slope)+.25;
   lo[k]=std::min(lo[k],std::min(x,y)-radius);hi[k]=std::max(hi[k],std::max(x,y)+radius);
  }
 }
 for(int k=0;k<2;++k){out.rectangle[k]=std::nextafter(float(lo[k]),-INFINITY);out.rectangle[k+2]=std::nextafter(float(hi[k]),INFINITY);}
 out.enabled=true;return out;
}
inline bool shadow_rejected(const ShadowBounds& b,const std::array<float,3>& p){
 if(!b.enabled)return false;
 for(float x:p)if(!std::isfinite(x)||std::abs(x)>shadow_coordinate_limit)return false;
 const float x=p[0]-b.slope[0]*p[1],z=p[2]-b.slope[1]*p[1];
 return x<b.rectangle[0]||z<b.rectangle[1]||x>b.rectangle[2]||z>b.rectangle[3];
}
inline std::string adapt_shadow_shader(std::string source,ShadowMode mode){
 if(mode==ShadowMode::Original)return source;
 const std::string loop="for(int i=0;i<3;i++){ vec4 a=uSeg[i*2], b=uSeg[i*2+1]; sh=min(sh, smoothstep(a.w*0.7, a.w*1.2, rayseg(vW,uL,a.xyz,b.xyz))); }";
 const auto pos=source.find(loop);
 if(pos==std::string::npos||source.find(loop,pos+1)!=std::string::npos)throw std::runtime_error("original shadow loop missing or ambiguous");
 if(mode==ShadowMode::Off){source.replace(pos,loop.size(),"// Non-parity shadow-only diagnostic: sh remains one.");return source;}
 const std::string guard=R"(vec2 shadowP=vW.xz-uShadowSlope*vW.y;
    bool shadowInside=shadowP.x>=uShadowBounds.x && shadowP.y>=uShadowBounds.y && shadowP.x<=uShadowBounds.z && shadowP.y<=uShadowBounds.w;
    // Unsupported receivers execute the original loop, including nonfinite values.
    bool shadowQualified=all(lessThanEqual(abs(vW),vec3(8192.)));
    if(uShadowCullEnabled==0 || !shadowQualified || shadowInside){ )";
 source.replace(pos,loop.size(),guard+loop+" }");
 auto main=source.find("void main(){");if(main==std::string::npos)throw std::runtime_error("scene main missing");
 source.insert(main,"uniform highp vec2 uShadowSlope; uniform highp vec4 uShadowBounds; uniform int uShadowCullEnabled;\n");
 return source;
}
}
