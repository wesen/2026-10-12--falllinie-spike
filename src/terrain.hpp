#pragma once
#include "math.hpp"
#include <cstdint>
#include <map>
#include <vector>
namespace fl {
constexpr double CELL=110, ROW=0.8;
constexpr int ROWS=138;
struct Paint {double lip=0,stripe=0,rel=0,u=0;};
struct Params {double W=0,R=0,cap=0;bool pipe=false;};
struct Feature {int k=0;double z=0,u=0,H=0,L=0,w=0,D=0,face=0,len=0;bool mega=false;};
uint32_t hash_int(uint32_t n,uint32_t seed);
struct Rng {uint32_t a;double next();};
class Terrain {
public:
 explicit Terrain(uint32_t seed):seed_(seed){}
 double height(double x,double z,Paint* out=nullptr);
 Vec3 normal(double x,double z);
 Params params(double z);
 const std::vector<Feature>& features(int cell);
 static double center(double z);
 static double base(double z);
private:
 uint32_t seed_;
 std::map<int,Params> params_;
 std::map<int,std::vector<Feature>> features_;
 std::vector<double> cumulative_{0};
 Params cell_params(int cell);
 double h2(int x,int z) const;
 double noise(double x,double z) const;
 double fbm(double x,double z,int octaves) const;
 double drop(double z);
};
struct Vertex {float px,py,pz,nx,ny,nz,albedo,lip,stripe,lateral,relative_height;};
static_assert(sizeof(Vertex)==44,"vertex stride must match the original 11 floats");
struct CpuMesh {std::vector<Vertex> vertices;std::vector<uint32_t> indices;};
class TerrainJob {
public:
 TerrainJob(Terrain& terrain,int cell);
 bool step(double budget_ms);
 int cell()const{return cell_;}
 CpuMesh take();
private:
 struct Sample{Vec3 p;Paint o;};
 Terrain& terrain_;int cell_,row_=-1;bool sampling_=true,complete_=false;
 std::vector<double> columns_;std::vector<std::vector<Sample>> samples_;CpuMesh mesh_;
};
CpuMesh build_terrain(Terrain& terrain,int cell);
}
