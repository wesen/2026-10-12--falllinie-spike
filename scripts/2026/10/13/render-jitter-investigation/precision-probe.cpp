#include "math.hpp"
#include <cstdio>
#include <array>
using namespace fl;
using V4=std::array<float,4>;
static V4 gpu_mul(const Mat4&m,V4 p){V4 o{};for(int r=0;r<4;++r)for(int c=0;c<4;++c)o[r]+=m[c*4+r]*p[c];return o;}
static Vec3 ray(const Mat4&ivp,Vec3 eye){auto w=gpu_mul(ivp,{.2f,.35f,1.f,1.f});return norm({double(w[0]/w[3]-float(eye.x)),double(w[1]/w[3]-float(eye.y)),double(w[2]/w[3]-float(eye.z))});}
int main(){const Vec3 base{12,5,8},look{12,1,38};const Mat4 p=perspective(62*3.141592653589793/180.,4./3.,.1,900);const auto reference=ray(inverse(multiply(p,look_at(base,look,{0,1,0}))),base);
 for(bool local:{false,true}){double max_error=0,max_step=0;Vec3 previous=reference;for(int i=0;i<512;++i){Vec3 offset{0,-60000.+i*.001,60000.+i*.001};Vec3 eye=base+offset,target=look+offset;Vec3 origin=local?Vec3{0,std::round(eye.y/128.)*128.,std::round(eye.z/128.)*128.}:Vec3{};eye=eye-origin;target=target-origin;auto direction=ray(inverse(multiply(p,look_at(eye,target,{0,1,0}))),eye);max_error=std::max(max_error,length(direction-reference));if(i)max_step=std::max(max_step,length(direction-previous));previous=direction;}std::printf("%s: max_ray_error=%.9g max_adjacent_ray_jump=%.9g approx_vertical_pixels=%.6g\n",local?"relative":"absolute",max_error,max_step,max_step*288/(62*3.141592653589793/180.));}
}
