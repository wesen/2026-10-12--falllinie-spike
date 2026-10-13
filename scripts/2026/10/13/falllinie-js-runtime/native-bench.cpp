// Same row coordinates and paint outputs as JS terrainRow, different native allocation strategy.
#include "terrain.hpp"
#include <chrono>
#include <iostream>
#include <algorithm>
int main(){fl::Terrain t(42);std::vector<double> cols;for(double u:{300,240,185,140,105,80,64,54,48})cols.push_back(-u);for(int i=-44;i<=44;i++)cols.push_back(i);for(double u:{48,54,64,80,105,140,185,240,300})cols.push_back(u);
 std::vector<double> times,out(749);double sum=0;
 for(int j=0;j<330;++j){auto a=std::chrono::steady_clock::now();int k=0;double z=3*fl::CELL+4*fl::ROW,cx=fl::Terrain::center(z);for(double u:cols){fl::Paint p;double x=cx+u,h=t.height(x,z,&p);for(double d:{x,h,z,p.lip,p.stripe,p.rel,p.u})out[k++]=d;}std::vector<double> copy(out);sum=0;for(double d:copy)sum+=d;auto b=std::chrono::steady_clock::now();if(j>=30)times.push_back(std::chrono::duration<double,std::micro>(b-a).count());}
 std::sort(times.begin(),times.end());std::cout<<"native row n=300 warmup=30 p50_us="<<times[149]<<" p95_us="<<times[284]<<" p99_us="<<times[296]<<" max_us="<<times.back()<<" checksum="<<sum<<'\n';}
