#include <SDL.h>
#include <GLES3/gl3.h>
#include "gl_api.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

static std::string ipc(const std::string& command) {
    const std::string shell = "timeout 3 swaymsg -r " + command + " 2>&1";
    FILE* pipe = popen(shell.c_str(), "r");
    if (!pipe) return "IPC unavailable";
    std::string out; char buffer[4096];
    while (fgets(buffer, sizeof buffer, pipe)) out += buffer;
    const int status = pclose(pipe);
    if (status != 0) std::cerr << "IPC failed: " << out << '\n';
    return out;
}
static GLuint shader(GLenum kind, const char* source) {
    GLuint sh = glCreateShader(kind); glShaderSource(sh, 1, &source, nullptr); glCompileShader(sh);
    GLint ok = 0; glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[8192]; glGetShaderInfoLog(sh, sizeof log, nullptr, log);
        glDeleteShader(sh); throw std::runtime_error(log);
    }
    return sh;
}
int main(int argc, char** argv) {
    bool place = false; double seconds = 0;
    for (int i=1; i<argc; ++i) {
        const std::string arg=argv[i];
        if (arg=="--place") place=true;
        else if (arg=="--seconds" && i+1<argc) seconds=std::stod(argv[++i]);
        else { std::cerr << "usage: falllinie-spike [--place] [--seconds N]\n"; return 2; }
    }
    if (place) {
        setenv("XDG_RUNTIME_DIR", "/run/0-runtime-dir", 0);
        setenv("WAYLAND_DISPLAY", "wayland-1", 0);
        setenv("SWAYSOCK", "/run/0-runtime-dir/sway-ipc.0.sock", 0);
    }
    SDL_Window* window=nullptr; SDL_GLContext context=nullptr;
    std::atomic<bool> stop{false}; std::thread placement;
    GLuint program=0, vao=0;
    int result=0;
    try {
        if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS|SDL_INIT_JOYSTICK)!=0) throw std::runtime_error(SDL_GetError());
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,0);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
        window=SDL_CreateWindow("falllinie:spike",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,1024,768,SDL_WINDOW_OPENGL);
        if (!window) throw std::runtime_error(SDL_GetError());
        context=SDL_GL_CreateContext(window);
        if (!context) throw std::runtime_error(SDL_GetError());
        glapi::load();
        std::cout << "GL_VERSION=" << glGetString(GL_VERSION) << "\nGL_RENDERER=" << glGetString(GL_RENDERER)
                  << "\nGL_VENDOR=" << glGetString(GL_VENDOR) << "\nGLSL=" << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
        GLint major=0; glGetIntegerv(GL_MAJOR_VERSION,&major);
        if (major<3) throw std::runtime_error("GLES3 required");
        SDL_GL_SetSwapInterval(1);
        GLuint vs=shader(GL_VERTEX_SHADER,"#version 300 es\nvoid main(){vec2 p[3]=vec2[3](vec2(-.7,-.7),vec2(.7,-.7),vec2(0.,.7));gl_Position=vec4(p[gl_VertexID],0.,1.);}");
        GLuint fs=shader(GL_FRAGMENT_SHADER,"#version 300 es\nprecision highp float;out vec4 c;void main(){c=vec4(.05,.2,.5,1.);}");
        program=glCreateProgram(); glAttachShader(program,vs);glAttachShader(program,fs);glLinkProgram(program);
        glDeleteShader(vs);glDeleteShader(fs);
        GLint linked=0;glGetProgramiv(program,GL_LINK_STATUS,&linked);
        if (!linked) throw std::runtime_error("triangle link failed");
        glGenVertexArrays(1,&vao); glBindVertexArray(vao);
        bool quit=false, started=false;
        SDL_Joystick* pad=SDL_NumJoysticks()>0 ? SDL_JoystickOpen(0):nullptr;
        const auto start=std::chrono::steady_clock::now();
        int frames=0;
        while(!quit) {
            SDL_Event e;
            while(SDL_PollEvent(&e)) if(e.type==SDL_QUIT || (e.type==SDL_KEYDOWN && e.key.keysym.sym==SDLK_ESCAPE)) quit=true;
            if(pad && SDL_JoystickGetButton(pad,8) && SDL_JoystickGetButton(pad,9)) quit=true;
            int w,h;SDL_GL_GetDrawableSize(window,&w,&h);glViewport(0,0,w,h);
            glClearColor(.94,.94,.95,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            glUseProgram(program);glDrawArrays(GL_TRIANGLES,0,3);SDL_GL_SwapWindow(window);++frames;
            if(place && !started) {
                started=true;
                placement=std::thread([&stop]{
                    for(int i=0;i<60 && !stop;++i) {
                        if(ipc("-t get_tree").find("falllinie:spike")!=std::string::npos) {
                            std::cout << "placement=" << ipc("'[title=\"^falllinie:spike$\"] fullscreen disable, floating enable, border none, move to output DSI-2, fullscreen enable, focus'") << std::endl;
                            return;
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                    if(!stop) std::cerr << "placement timeout\n";
                });
            }
            const double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
            if(seconds>0 && elapsed>=seconds) quit=true;
        }
        if(pad) SDL_JoystickClose(pad);
        std::cout << "frames=" << frames << " GL_error=" << glGetError() << std::endl;
    }catch(const std::exception& e) {std::cerr << "fatal: " << e.what() << '\n';result=1;}
    stop=true;if(placement.joinable())placement.join();
    if(context) {if(vao)glDeleteVertexArrays(1,&vao);if(program)glDeleteProgram(program);SDL_GL_DeleteContext(context);}
    if(window) SDL_DestroyWindow(window);
    SDL_Quit();
    return result;
}
