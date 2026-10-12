#pragma once
#include <array>
#include <cmath>
#include <algorithm>
#include <stdexcept>
namespace fl {
struct Vec3 { double x=0,y=0,z=0; };
inline Vec3 operator+(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec3 operator-(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec3 operator*(Vec3 a,double s){return {a.x*s,a.y*s,a.z*s};}
inline double dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline Vec3 cross(Vec3 a,Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline double length(Vec3 a){return std::sqrt(dot(a,a));}
inline Vec3 norm(Vec3 a){const double l=length(a);return l>1e-9?a*(1/l):Vec3{0,1,0};}
inline double clamp(double v,double a,double b){return std::max(a,std::min(b,v));}
inline double lerp(double a,double b,double t){return a+(b-a)*t;}
inline double smooth(double a,double b,double x){double t=clamp((x-a)/(b-a),0,1);return t*t*(3-2*t);}
using Mat4=std::array<float,16>;
inline Mat4 identity(){return {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};}
inline Mat4 perspective(double fovy,double aspect,double near,double far){
 Mat4 m{};double t=1/std::tan(fovy/2);m[0]=t/aspect;m[5]=t;m[10]=(far+near)/(near-far);m[11]=-1;m[14]=2*far*near/(near-far);return m;
}
inline Mat4 look_at(Vec3 eye,Vec3 center,Vec3 up){
 Vec3 z=norm(eye-center),x=norm(cross(up,z)),y=cross(z,x);
 return {float(x.x),float(y.x),float(z.x),0,float(x.y),float(y.y),float(z.y),0,float(x.z),float(y.z),float(z.z),0,float(-dot(x,eye)),float(-dot(y,eye)),float(-dot(z,eye)),1};
}
inline Mat4 multiply(Mat4 a,Mat4 b){
 Mat4 out{};for(int i=0;i<4;++i)for(int j=0;j<4;++j)for(int k=0;k<4;++k)out[i*4+j]+=a[k*4+j]*b[i*4+k];return out;
}
inline Mat4 inverse(Mat4 m){
 // Gauss-Jordan in double precision; input/output storage remains column-major.
 double a[4][8]{};
 for(int r=0;r<4;++r){for(int c=0;c<4;++c)a[r][c]=m[c*4+r];a[r][r+4]=1;}
 for(int c=0;c<4;++c){
  int pivot=c;for(int r=c+1;r<4;++r)if(std::abs(a[r][c])>std::abs(a[pivot][c]))pivot=r;
  if(std::abs(a[pivot][c])<1e-15)throw std::runtime_error("singular matrix");
  for(int k=0;k<8;++k)std::swap(a[c][k],a[pivot][k]);
  double d=a[c][c];for(int k=0;k<8;++k)a[c][k]/=d;
  for(int r=0;r<4;++r)if(r!=c){double f=a[r][c];for(int k=0;k<8;++k)a[r][k]-=f*a[c][k];}
 }
 Mat4 out{};for(int r=0;r<4;++r)for(int c=0;c<4;++c)out[c*4+r]=a[r][c+4];return out;
}
}
