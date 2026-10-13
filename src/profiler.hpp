#pragma once
#include <GLES3/gl3.h>
#include <fstream>
#include <array>
#include <cstdint>
#include <string>
namespace fl {
// Optional asynchronous GPU queries; never wait for a result in the render loop.
class Profiler {
public:
 explicit Profiler(const std::string& path,bool gpu=false);
 ~Profiler();
 void poll();
 void begin_gpu(uint64_t frame,int view);
 void end_gpu(uint64_t frame,int view);
 void cpu(uint64_t frame,double interval,double events,double physics,double player,double stream,double top_submit,double top_swap,double bottom_submit,double bottom_swap);
 bool enabled()const{return cpu_.is_open();}
private:
 struct Query{GLuint id=0;uint64_t frame=0;bool pending=false,active=false,invalid=false;};
 std::array<std::array<Query,2>,8> queries_{};
 std::ofstream cpu_,gpu_;
 bool supported_=false;
 using Gen=void(*)(GLsizei,GLuint*);using Delete=void(*)(GLsizei,const GLuint*);using Begin=void(*)(GLenum,GLuint);using End=void(*)(GLenum);using Get=void(*)(GLuint,GLenum,GLuint*);using Get64=void(*)(GLuint,GLenum,GLuint64*);
 Gen gen_=nullptr;Delete del_=nullptr;Begin begin_=nullptr;End end_=nullptr;Get get_=nullptr;Get64 get64_=nullptr;
};
}
