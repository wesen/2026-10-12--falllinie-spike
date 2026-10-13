// Compile the implementation once in this test executable, not source-systems.
// The friend fixture inspects real ownership without physical SDL scheduling.
#include "audio.cpp"
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
namespace {
thread_local bool watch=false;
std::atomic<size_t> allocations{0};
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
}
namespace fl {
struct AudioEngineTestAccess {
 static void start(AudioEngine& engine,size_t target=5120){
  auto& a=*engine.impl_;require(a.prepare(48000,1),"worker prepare");
  a.mode=AudioRenderMode::BufferedWorker;a.pcm_target=target;a.perf_frequency=SDL_GetPerformanceFrequency();
  a.worker=std::thread([&a]{a.produce();});
  std::unique_lock<std::mutex> lock(a.worker_mutex);
  require(a.worker_cv.wait_for(lock,std::chrono::seconds(2),[&]{return a.pcm.depth()>=target;}),"worker prefill timeout");
 }
 static void callback(AudioEngine& engine,float* output,size_t frames){
  watch=true;AudioEngine::Impl::callback(engine.impl_.get(),reinterpret_cast<Uint8*>(output),int(frames*8));watch=false;
 }
 static void await_depth(AudioEngine& engine,size_t frames){
  for(int i=0;i<2000&&engine.impl_->pcm.depth()<frames;++i)std::this_thread::sleep_for(std::chrono::milliseconds(1));
  require(engine.impl_->pcm.depth()>=frames,"worker refill timeout");
 }
};
}
void* operator new(std::size_t n){if(watch)allocations.fetch_add(1);if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
int main(){try{
 using Access=fl::AudioEngineTestAccess;
 constexpr size_t total=32768;
 fl::AudioEngine reference,worker;
 require(reference.prepare_offline(48000,1),"reference prepare");
 std::vector<float> expected(total*2),actual(total*2);reference.render_offline(expected.data(),total);
 Access::start(worker);require(!worker.prepare_offline(),"offline with worker accepted");
 std::array<float,8192> request{};size_t at=0,turn=0;constexpr size_t requests[]={137,256,1024,4096,513,1};
 while(at<total){
  size_t n=std::min(requests[turn++%6],total-at);Access::await_depth(worker,n);
  // Pause after partial retirement, retain all unheard PCM, resume repeatedly.
  if(turn%5==0){worker.pause(true);auto before=worker.diagnostics();std::this_thread::sleep_for(std::chrono::milliseconds(5));require(worker.diagnostics().frames==before.frames,"acknowledged producer did not freeze");worker.pause(false);}
  Access::callback(worker,request.data(),n);std::copy_n(request.data(),n*2,actual.data()+at*2);at+=n;
 }
 require(expected==actual,"actual worker/callback/pause sequence not bit-identical");
 worker.pause(true);auto paused=worker.diagnostics();
 require(paused.frames==paused.delivered_frames+paused.pcm_depth,"worker frontier identity");
 // Drain retained data while acknowledged producer is suspended, leaving empty.
 std::array<float,16384> drain{};Access::callback(worker,drain.data(),paused.pcm_depth);
 auto empty=worker.diagnostics();require(empty.pcm_depth==0,"empty shutdown fixture");
 const auto generated=empty.frames;Access::callback(worker,drain.data(),4096);
 auto starved=worker.diagnostics();require(starved.starved_callbacks==1&&starved.missing_frames==4096&&starved.frames==generated,"actual callback shortage counters/frontier");
 for(size_t i=0;i<8192;++i)require(drain[i]==0,"actual callback shortage not silence");
 worker.close();require(worker.diagnostics().pcm_depth==0,"empty close changed PCM");
 {fl::AudioEngine full;Access::start(full,8192);require(full.diagnostics().pcm_depth==8192,"full storage fixture");full.close();}
 {fl::AudioEngine full;Access::start(full,8192);full.pause(true);} // paused-full destructor
 {fl::AudioEngine empty_running;Access::start(empty_running);empty_running.pause(true);auto d=empty_running.diagnostics();Access::callback(empty_running,drain.data(),d.pcm_depth);empty_running.pause(false);empty_running.close();} // refill/empty stop
 require(allocations.load()==0,"actual buffered callback allocation");
 std::cout<<"actual worker+callback bit identity over partial/wrap/pause; empty/full/paused-full/refill shutdown; shortage 1/4096; callback allocations 0\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
