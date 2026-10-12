#pragma once
// Deterministic host API substitute: captured blocks, bounded waits, no device.
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <mutex>
#include <thread>
namespace native_stub {
inline unsigned opened=0,closed=0,starts=0,joins=0,deletes=0,drains=0;
inline int fail_volume=0,fail_create=0,fail_start=0,fail_output=0;
inline std::atomic<int> rest{0};
inline SceKernelThreadEntry entry=nullptr;
inline std::thread thread;
inline std::mutex mutex;
inline std::condition_variable cv;
inline bool free_run=false;
inline std::vector<dsound_pcm::Block> captured;
inline void reset(){
    assert(!thread.joinable());std::lock_guard<std::mutex> guard(mutex);
    captured.clear();free_run=false;fail_volume=fail_create=fail_start=fail_output=rest=0;
}
inline void wait_block(){
    std::unique_lock<std::mutex> guard(mutex);
    assert(cv.wait_for(guard,std::chrono::seconds(5),[]{return !captured.empty();}));
}
inline void finish(){std::lock_guard<std::mutex> guard(mutex);free_run=true;cv.notify_all();}
}
int sceAudioOutOpenPort(int type,int len,int hz,int mode){
    assert(type==0 && len==1024 && hz==48000 && mode==1);++native_stub::opened;return 7;
}
int sceAudioOutGetConfig(int port,int type){assert(port==7);return type==0?1024:type==1?48000:1;}
int sceAudioOutReleasePort(int port){assert(port==7 && !native_stub::thread.joinable());++native_stub::closed;return 0;}
int sceAudioOutSetVolume(int port,SceAudioOutChannelFlag flags,int* volume){
    assert(port==7 && int(flags)==3 && volume[0]==32768 && volume[1]==32768);return native_stub::fail_volume;
}
int sceAudioOutGetRestSample(int port){assert(port==7);return native_stub::rest;}
int sceAudioOutOutput(int port,const void* samples){
    assert(port==7);
    if(!samples){++native_stub::drains;return 0;}
    assert(reinterpret_cast<uintptr_t>(samples)%64==0);
    std::unique_lock<std::mutex> guard(native_stub::mutex);
    dsound_pcm::Block block;std::memcpy(block.data(),samples,sizeof(block));
    native_stub::captured.push_back(block);native_stub::cv.notify_all();
    assert(native_stub::cv.wait_for(guard,std::chrono::seconds(5),[]{return native_stub::free_run;}));
    guard.unlock();std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return native_stub::fail_output;
}
uint64_t sceKernelGetProcessTimeWide(){
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
[[noreturn]] void sceKernelExitProcess(int){std::abort();}
SceUID sceKernelCreateThread(const char*,SceKernelThreadEntry entry,int priority,SceSize stack,unsigned,int,void*){
    assert(priority==0x10000100 && stack==65536);native_stub::entry=entry;return native_stub::fail_create?native_stub::fail_create:8;
}
int sceKernelStartThread(SceUID id,SceSize size,void* arg){
    assert(id==8 && size==sizeof(void*));if(native_stub::fail_start)return native_stub::fail_start;
    void* self=nullptr;std::memcpy(&self,arg,size);++native_stub::starts;
    native_stub::thread=std::thread([self,size]{void* copy=self;native_stub::entry(size,&copy);});return 0;
}
int sceKernelWaitThreadEnd(SceUID id,int*,unsigned*){assert(id==8 && native_stub::thread.joinable());native_stub::thread.join();++native_stub::joins;return 0;}
int sceKernelDeleteThread(SceUID id){assert(id==8 && !native_stub::thread.joinable());++native_stub::deletes;return 0;}
int sceKernelDelayThread(unsigned us){std::this_thread::sleep_for(std::chrono::microseconds(us));return 0;}
