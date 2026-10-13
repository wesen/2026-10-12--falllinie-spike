// Selected existing native outputs, independently compared to frozen JS.
#include "game.hpp"
#include <iostream>
#include <iomanip>
int main(){
 fl::Game g(42);std::cout<<std::setprecision(17)<<"{\"hashes\":[";bool first=true;
 for(uint32_t n:{0u,1u,0xffffffffu,0x80000000u,0x12345678u}){if(!first)std::cout<<',';first=false;std::cout<<fl::hash_int(n,42);}
 std::cout<<"],\"random\":[";fl::Rng r{12345};for(int i=0;i<12;++i){if(i)std::cout<<',';std::cout<<r.next();}
 std::cout<<"],\"terrain\":[";first=true;
 for(auto p:{std::pair<double,double>{0,4},{12,150},{-25,365},{60,900},{0,-10}}){if(!first)std::cout<<',';first=false;fl::Paint paint;double h=g.terrain.height(p.first,p.second,&paint);auto n=g.terrain.normal(p.first,p.second);std::cout<<"{\"x\":"<<p.first<<",\"z\":"<<p.second<<",\"h\":"<<h<<",\"n\":["<<n.x<<','<<n.y<<','<<n.z<<"],\"paint\":{\"lip\":"<<paint.lip<<",\"stripe\":"<<paint.stripe<<",\"rel\":"<<paint.rel<<",\"u\":"<<paint.u<<"}}";}
 g.spin=2*fl::PI;g.flip=-fl::TAU;g.air_time=1.3;g.air.grabs={{1,.5,.3}};g.air.ubers={fl::uber_action(1)};
 std::cout<<"],\"scores\":[";for(int i=0;i<2;++i){if(i)std::cout<<',';auto s=g.score_air(true);if(!s)return 1;std::cout<<"{\"name\":"<<std::quoted(s->name)<<",\"pts\":"<<s->points<<",\"ubers\":"<<s->ubers<<'}';}
 auto trees=g.world.trees(3);std::cout<<"],\"trees\":[";for(int i=0;i<2;++i){if(i)std::cout<<',';const auto& t=trees.at(i);std::cout<<"{\"x\":"<<t.x<<",\"z\":"<<t.z<<",\"y\":"<<t.y<<",\"h\":"<<t.height<<",\"r\":"<<t.radius<<",\"g\":"<<(t.grey?"true":"false")<<'}';}std::cout<<"]}\n";
}
