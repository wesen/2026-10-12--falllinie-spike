#pragma once
#include "math.hpp"
namespace fl {
// Source qAxis/qMul/qRot/qSlerp/qFromBasis conventions: x,y,z,w.
struct Quat {double x=0,y=0,z=0,w=1;};
inline Quat qnorm(Quat q){double l=std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w);return l>1e-12?Quat{q.x/l,q.y/l,q.z/l,q.w/l}:Quat{};}
inline Quat qaxis(Vec3 axis,double angle){axis=norm(axis);double s=std::sin(angle/2);return {axis.x*s,axis.y*s,axis.z*s,std::cos(angle/2)};}
inline Quat qmul(Quat a,Quat b){return {a.w*b.x+b.w*a.x+a.y*b.z-a.z*b.y,a.w*b.y+b.w*a.y+a.z*b.x-a.x*b.z,a.w*b.z+b.w*a.z+a.x*b.y-a.y*b.x,a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};}
inline Vec3 qrot(Quat q,Vec3 v){Vec3 xyz{q.x,q.y,q.z},t=cross(xyz,v)*2;return v+t*q.w+cross(xyz,t);}
inline Quat qslerp(Quat a,Quat b,double t){double d=a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w;if(d<0){b={-b.x,-b.y,-b.z,-b.w};d=-d;}double wa=1-t,wb=t;if(d<=.9995){double theta=std::acos(clamp(d,-1,1)),s=std::sin(theta);wa=std::sin((1-t)*theta)/s;wb=std::sin(t*theta)/s;}return qnorm({a.x*wa+b.x*wb,a.y*wa+b.y*wb,a.z*wa+b.z*wb,a.w*wa+b.w*wb});}
inline Quat qbasis(Vec3 x,Vec3 y,Vec3 z){double trace=x.x+y.y+z.z;Quat q;
 if(trace>0){double s=std::sqrt(trace+1)*2;q={(y.z-z.y)/s,(z.x-x.z)/s,(x.y-y.x)/s,s/4};}
 else if(x.x>y.y && x.x>z.z){double s=std::sqrt(1+x.x-y.y-z.z)*2;q={s/4,(y.x+x.y)/s,(z.x+x.z)/s,(y.z-z.y)/s};}
 else if(y.y>z.z){double s=std::sqrt(1+y.y-x.x-z.z)*2;q={(y.x+x.y)/s,s/4,(z.y+y.z)/s,(z.x-x.z)/s};}
 else {double s=std::sqrt(1+z.z-x.x-y.y)*2;q={(z.x+x.z)/s,(z.y+y.z)/s,s/4,(x.y-y.x)/s};}
 return qnorm(q);
}
inline Quat frameq(Vec3 forward,Vec3 normal){Vec3 x=norm(forward-normal*dot(forward,normal));return qbasis(x,normal,cross(x,normal));}
}
