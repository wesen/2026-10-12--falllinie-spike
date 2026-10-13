#pragma once
#include "game.hpp"
#include <iostream>
#include <stdexcept>
namespace gameplay_test {
inline void require(bool valid,const std::string& label){if(!valid)throw std::runtime_error(label);}
inline void close(double actual,double expected,const std::string& label,double epsilon=1e-8){if(std::abs(actual-expected)>epsilon||!std::isfinite(actual))throw std::runtime_error(label+": actual="+std::to_string(actual)+" expected="+std::to_string(expected));}
inline void vector(std::vector<double>& out,fl::Vec3 v){out.insert(out.end(),{v.x,v.y,v.z});}
inline void quat(std::vector<double>& out,fl::Quat q){out.insert(out.end(),{q.x,q.y,q.z,q.w});}
inline void compare(const std::vector<double>& actual,const std::vector<double>& expected,const std::string& label,double eps=1e-8){require(actual.size()==expected.size(),label+" size");for(size_t i=0;i<actual.size();++i)close(actual[i],expected[i],label+"["+std::to_string(i)+"]",eps);}
inline int events(const fl::Game& g,fl::GameEventKind k){return int(std::count_if(g.events.begin(),g.events.end(),[k](const fl::GameEvent& e){return e.kind==k;}));}
}
