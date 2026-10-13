#pragma once
#include "terrain.hpp"
namespace fl {
struct Rail {Vec3 a,b,d;double length=0;};
struct Tree {double x=0,z=0,y=0,height=0,radius=0;bool grey=false,hazard=false;};
// Seeded definitions are shared by collision and object rendering. Rail rides own
// a copy: bounded definition-cache retirement cannot invalidate an active ride.
class World {
public:
 World(Terrain& terrain,uint32_t seed):terrain_(terrain),seed_(seed){}
 void reset(uint32_t seed){seed_=seed;rails_.clear();trees_.clear();rails_order_.clear();trees_order_.clear();}
 const std::vector<Rail>& rails(int cell);
 const std::vector<Tree>& trees(int cell);
 CpuMesh objects(int cell);
private:
 Terrain& terrain_;uint32_t seed_;
 std::map<int,std::vector<Rail>> rails_;std::map<int,std::vector<Tree>> trees_;
 std::deque<int> rails_order_,trees_order_;
};
// Original MB flat polygons/bars/cones, shared by object and primitive meshes.
class MeshBuilder {
public:
 CpuMesh mesh;
 uint32_t vertex(Vec3 p,Vec3 normal,double alb,double p0=0,double p1=0,double u=0,double r=0);
 void triangle(uint32_t a,uint32_t b,uint32_t c);void quad(uint32_t a,uint32_t b,uint32_t c,uint32_t d);
 void face(const std::vector<Vec3>& points,double alb,Vec3 ref);
 void box(Vec3 a,Vec3 b,double w,double h,double alb);
 void cone(double x,double y,double z,double r,double y0,double y1,int sides,double alb);
};
}
