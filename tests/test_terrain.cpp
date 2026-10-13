#include "terrain.hpp"
#include "golden.hpp"
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <cstddef>
static void require(bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);}
int main(){try{
 static_assert(offsetof(fl::Vertex,lip)==28 && offsetof(fl::Vertex,lateral)==36,"attribute layout");
 double max_error=0,min_dot=1;
 for(const auto& g:golden){fl::Terrain terrain(g.seed);fl::Paint p;double h=terrain.height(g.x,g.z,&p);max_error=std::max(max_error,std::abs(h-g.h));
  if(std::abs(h-g.h)>1e-5*std::max(1.0,std::abs(g.h))) std::cerr<<"case seed="<<g.seed<<" x="<<g.x<<" z="<<g.z<<" expected="<<g.h<<" actual="<<h<<'\n';
  require(std::abs(h-g.h)<=1e-5*std::max(1.0,std::abs(g.h)),"height mismatch");
  double agreement=fl::dot(terrain.normal(g.x,g.z),{g.nx,g.ny,g.nz});min_dot=std::min(min_dot,agreement);require(agreement>=0.99999,"normal mismatch");
  require(p.lip==g.lip && p.stripe==g.stripe,"paint mask mismatch");require(std::abs(p.rel-g.rel)<1e-5 && std::abs(p.u-g.u)<1e-5,"metadata mismatch");
 }
 for(const auto& h:hashes)require(fl::hash_int(h[0],42)==h[1],"hash mismatch");
 fl::Rng rng{12345};for(double r:random_values)require(rng.next()==r,"RNG mismatch");
 fl::Terrain terrain(42);auto m=fl::build_terrain(terrain,0);
 require(m.vertices.size()==14873 && m.indices.size()==87768,"mesh count mismatch");
 for(uint32_t i:m.indices)require(i<m.vertices.size(),"index out of range");
 for(const auto& v:m.vertices){require(std::isfinite(v.py) && std::isfinite(v.ny),"nonfinite vertex");require(v.ny>=0,"normal points down");}
 for(const auto& g:mesh_golden){const auto& v=m.vertices[g.index];float a[]={v.px,v.py,v.pz,v.nx,v.ny,v.nz,v.albedo,v.lip,v.stripe,v.lateral,v.relative_height};for(int k=0;k<11;++k)require(std::abs(a[k]-g.v[k])<1e-4,"mesh vertex/normal mismatch");}
 require(std::abs(m.vertices.back().pz-110.4)<1e-4,"original row overlap lost");
 fl::Mat4 matrix=fl::multiply(fl::perspective(1.0,4.0/3,0.1,900),fl::look_at({2,3,-4},{0,0,4},{0,1,0}));
 auto product=fl::multiply(matrix,fl::inverse(matrix)),id=fl::identity();for(int k=0;k<16;++k)require(std::abs(product[k]-id[k])<1e-4,"matrix inverse mismatch");
 std::cout<<"PASS "<<sizeof(golden)/sizeof(golden[0])<<" terrain cases; 5 hash cases; 12 RNG values; 6 mesh samples; topology/layout/matrix checks. max_height_error="<<max_error<<" min_normal_dot="<<min_dot<<"\n";
 return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}}
