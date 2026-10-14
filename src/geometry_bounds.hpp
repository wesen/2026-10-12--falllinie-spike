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
// Consecutive index ranges of one mesh with the bounds of the vertices they reference.
// Drawing the surviving ranges in their original order rasterizes the same triangles
// in the same order as one draw of the whole index buffer.
struct IndexChunk {uint32_t first=0,count=0;Aabb bounds{};};
template<class VertexT>
inline std::vector<IndexChunk> index_chunks(const std::vector<VertexT>& vertices,const std::vector<uint32_t>& indices,uint32_t chunk_indices){
 std::vector<IndexChunk> chunks;if(chunk_indices<3)chunk_indices=3;chunk_indices-=chunk_indices%3;
 for(uint32_t first=0;first<indices.size();first+=chunk_indices){
  IndexChunk c;c.first=first;c.count=uint32_t(std::min<size_t>(chunk_indices,indices.size()-first));
  for(uint32_t k=first;k<first+c.count;++k){const auto& v=vertices.at(indices[k]);c.bounds.add(v.px,v.py,v.pz);}
  chunks.push_back(c);
 }
 return chunks;
}
// PERF-010 row-decimated terrain level of detail (pixel-changing; owner decision).
// A terrain cell is rows x columns vertices; strip k joins rows k and k+1 with quads
// {A,C,B,B,C,D}, A=k*columns+i. Level L of a chunk keeps every column and every
// 2^L-th row between the chunk's boundary rows, which stay shared with neighbours.
inline constexpr int terrain_lod_levels=5;
struct TerrainLod {std::vector<uint32_t> extra;std::vector<std::array<std::array<uint32_t,2>,terrain_lod_levels>> ranges;};
inline bool build_terrain_lod(const std::vector<uint32_t>& base,size_t vertices,uint32_t columns,uint32_t strips_per_chunk,TerrainLod& out){
 out={};if(columns<2||strips_per_chunk==0)return false;
 const size_t per_strip=size_t(columns-1)*6;if(base.empty()||base.size()%per_strip)return false;
 const uint32_t strips=uint32_t(base.size()/per_strip);if(vertices!=size_t(strips+1)*columns)return false;
 for(uint32_t k=0;k<strips;++k)for(uint32_t i=0;i+1<columns;++i){
  const uint32_t a=k*columns+i,b=a+1,c=a+columns,d=c+1;const size_t at=k*per_strip+size_t(i)*6;
  if(base[at]!=a||base[at+1]!=c||base[at+2]!=b||base[at+3]!=b||base[at+4]!=c||base[at+5]!=d)return false;
 }
 for(uint32_t s0=0;s0<strips;s0+=strips_per_chunk){
  const uint32_t n=std::min(strips_per_chunk,strips-s0);std::array<std::array<uint32_t,2>,terrain_lod_levels> r{};
  r[0]={uint32_t(s0*per_strip),uint32_t(n*per_strip)};
  out.ranges.push_back(r);
 }
 const uint32_t base_size=uint32_t(base.size());
 for(int level=1;level<terrain_lod_levels;++level){
  const uint32_t stride=1u<<level;size_t chunk=0;
  for(uint32_t s0=0;s0<strips;s0+=strips_per_chunk,++chunk){
   const uint32_t n=std::min(strips_per_chunk,strips-s0);if(n%stride)continue;
   const uint32_t first=base_size+uint32_t(out.extra.size());
   for(uint32_t row=s0;row<s0+n;row+=stride)for(uint32_t i=0;i+1<columns;++i){
    const uint32_t a=row*columns+i,b=a+1,c=a+stride*columns,d=c+1;out.extra.insert(out.extra.end(),{a,c,b,b,c,d});
   }
   out.ranges[chunk][size_t(level)]={first,base_size+uint32_t(out.extra.size())-first};
  }
 }
 return true;
}
// Highest supported level for a chunk at this distance; level 0 below near_m.
template<class Ranges>
inline int terrain_lod_level(double distance,double near_m,const Ranges& ranges){
 if(!(near_m>0)||!(distance>=near_m))return 0;
 int level=std::min(terrain_lod_levels-1,int(std::floor(std::log2(distance/near_m)))+1);
 while(level>0&&ranges[size_t(level)][1]==0)--level;
 return level;
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
