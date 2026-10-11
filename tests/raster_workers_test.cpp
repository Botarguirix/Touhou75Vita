// Shipping native worker class, host API simulation and real concurrent jobs.
// No Vita, x86 VM, game binary or proprietary texture is executed here.
#include "../vita/src/raster_workers.h"
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <thread>
#include <vector>
#include <limits>

namespace stub {
struct Thread {SceKernelThreadEntry entry=nullptr;int mask=0;std::thread thread;};
std::array<Thread,2> threads;
unsigned creates=0,starts=0,joins=0,deletes=0;
unsigned fail_create=0,fail_start=0,bad_affinity=0;
thread_local unsigned index=0;
void reset(){for(const auto& t:threads)assert(!t.thread.joinable());
    creates=starts=joins=deletes=fail_create=fail_start=bad_affinity=0;}
}
SceUID sceKernelCreateThread(const char*,SceKernelThreadEntry entry,int priority,SceSize stack,unsigned attr,int mask,void*) {
    assert(priority==0x10000110 && stack==128*1024 && attr==0);
    assert(mask==(stub::creates?SCE_KERNEL_CPU_MASK_USER_2:SCE_KERNEL_CPU_MASK_USER_1));
    const unsigned i=stub::creates++;if(stub::fail_create==i+1)return -11;
    assert(i<2);stub::threads[i].entry=entry;stub::threads[i].mask=mask;return 100+int(i);
}
int sceKernelStartThread(SceUID id,SceSize size,void* arg) {
    const unsigned i=unsigned(id-100);assert(i<2 && size==sizeof(void*));
    if(stub::fail_start==i+1)return -12;
    void* copy=nullptr;std::memcpy(&copy,arg,size);++stub::starts;
    stub::threads[i].thread=std::thread([copy,i,size]{stub::index=i;void* local=copy;assert(stub::threads[i].entry(size,&local)==0);});
    return 0;
}
int sceKernelWaitThreadEnd(SceUID id,int*,unsigned*) {
    auto& t=stub::threads.at(unsigned(id-100));assert(t.thread.joinable());t.thread.join();++stub::joins;return 0;
}
int sceKernelDeleteThread(SceUID id){assert(!stub::threads.at(unsigned(id-100)).thread.joinable());++stub::deletes;return 0;}
int sceKernelGetThreadId(){return 100+int(stub::index);}
int sceKernelGetThreadCpuAffinityMask(SceUID id){const unsigned i=unsigned(id-100);assert(i<2);
    return stub::bad_affinity==i+1?SCE_KERNEL_CPU_MASK_USER_0:stub::threads[i].mask;}
int sceKernelGetCpuId(){return int(stub::index)+1;}
[[noreturn]] void sceKernelExitProcess(int){std::abort();}

using namespace d3d8_quad;
struct Fixture {
    static constexpr uint32_t width=257,height=193,pitch=width*4+12;
    std::vector<uint8_t> texture=std::vector<uint8_t>((16*4+8)*16),
        bytes=std::vector<uint8_t>(pitch*height+64),reference;
    Vertex vertices[4]={{-.75f,2.5f,.2f,1,0xA0E0C0FF,-.17f,.02f},
        {255.25f,-2.25f,.2f,1,0xA0E0C0FF,1.17f,.09f},
        {7.5f,190.75f,.2f,1,0xA0E0C0FF,-.09f,1.18f},
        {259.5f,195.5f,.2f,1,0xA0E0C0FF,1.21f,1.06f}};
    Image source{texture.data(),texture.size(),16,16,16*4+8,21};
    Target target{bytes.data(),bytes.size(),width,height,pitch,21};
    Viewport viewport{0,0,width,height};Settings settings{};
    FILE* log=std::tmpfile();
    Fixture(){assert(log);uint32_t random=137;
        for(auto& b:texture){random=random*1664525+1013904223;b=uint8_t(random>>24);}
        for(auto& b:bytes){random=random*1664525+1013904223;b=uint8_t(random>>24);}
    }
    ~Fixture(){std::fclose(log);}
    void rectangle(){vertices[0].x=vertices[2].x=-.5f;vertices[1].x=vertices[3].x=260.5f;
        vertices[0].y=vertices[1].y=-.75f;vertices[2].y=vertices[3].y=195.25f;
        vertices[0].u=vertices[2].u=-.3f;vertices[1].u=vertices[3].u=1.2f;
        vertices[0].v=vertices[1].v=.04f;vertices[2].v=vertices[3].v=1.1f;}
    Stats expected(){reference=bytes;Target output=target;output.bytes=reference.data();Stats stats;
        assert(rasterize(vertices,source,output,viewport,settings,stats)==Result::Rendered);return stats;}
    static void same(const Stats& a,const Stats& b){
        assert(a.reject==b.reject && a.covered==b.covered && a.written==b.written &&
            a.changed==b.changed && a.alpha_rejected==b.alpha_rejected && a.triangle_strip==b.triangle_strip);
        assert(a.linear_constant==b.linear_constant && a.linear_exact==b.linear_exact && a.linear_reference==b.linear_reference);
        if(a.covered){assert(std::memcmp(&a.first,&b.first,sizeof(Probe))==0);assert(std::memcmp(&a.last,&b.last,sizeof(Probe))==0);}
    }
    void check(RasterWorkers& workers){const auto wanted=expected();Stats got;
        assert(RasterWorkers::execute(&workers,vertices,source,target,viewport,settings,got)==Result::Rendered);
        assert(bytes==reference && !got.hashes_valid);same(got,wanted);}
};
int main(){
    {
        Fixture f;for(unsigned geometry=0;geometry<2;++geometry){if(geometry)f.rectangle();
            for(unsigned filter:{1u,2u}){f.settings.filter=filter;const auto original=f.bytes;
                const auto wanted=f.expected();Stats got;f.settings.hash_pixels=false;
                assert(rasterize(f.vertices,f.source,f.target,f.viewport,f.settings,got)==Result::Rendered);
                assert(f.bytes==f.reference && !got.hashes_valid);Fixture::same(got,wanted);
                f.bytes=original;f.target.bytes=f.bytes.data();f.settings.hash_pixels=true;
            }}
        puts("PASS disabling per-pixel diagnostic hashes preserves rectangle/triangle pixels, counts and probes");
    }
    {
        Fixture f;const auto original=f.bytes;Stats stats;
        assert(rasterize(f.vertices,f.source,f.target,f.viewport,f.settings,stats,true)==Result::Rendered && f.bytes==original);
        f.rectangle();assert(rasterize(f.vertices,f.source,f.target,f.viewport,f.settings,stats,true)==Result::Rendered && f.bytes==original);
        f.vertices[3]=f.vertices[0];assert(rasterize(f.vertices,f.source,f.target,f.viewport,f.settings,stats,true)==Result::Unsupported && f.bytes==original);
        puts("PASS complete rectangle/convexity dry validation rejects invalid geometry before any writes");
    }
    stub::reset();
    {
        Fixture f;RasterWorkers workers(f.log);f.rectangle();
        for(unsigned filter:{1u,2u})for(uint32_t format:{21u,22u}){
            f.settings.filter=filter;f.target.format=format;f.check(workers);
        }
        f.source.format=25;f.source.pitch=16*2+8;f.check(workers);
        workers.shutdown();workers.report();assert(stub::creates==2 && stub::starts==2 && stub::joins==2 && stub::deletes==2);
        puts("PASS shipping worker rectangles match serial renderer for POINT/LINEAR A8/A1 and X8 with padded rows");
    }
    stub::reset();
    {
        Fixture f;RasterWorkers workers(f.log);f.viewport={5,11,247,175};
        for(unsigned i=0;i<48;++i){f.settings.filter=(i%2)+1;f.vertices[0].u+=.01f;f.check(workers);}
        workers.shutdown();assert(stub::starts==2 && stub::joins==2 && stub::deletes==2);
        puts("PASS 48 concurrent affine strips preserve clipping, alpha, wrap, row-major probes and draw order");
    }
    stub::reset();
    {
        Fixture f;RasterWorkers workers(f.log);f.viewport={8,17,31,23};f.check(workers);
        assert(stub::creates==0);f.viewport={0,0,257,193};f.check(workers);
        const auto original=f.bytes;Stats stats;auto source=f.source;
        source.bytes=f.target.bytes;source.size=f.target.size;
        assert(RasterWorkers::execute(&workers,f.vertices,source,f.target,f.viewport,f.settings,stats)==Result::Unsupported && stats.reject==Reject::Alias && f.bytes==original);
        f.settings.alpha_ref=256;
        assert(RasterWorkers::execute(&workers,f.vertices,f.source,f.target,f.viewport,f.settings,stats)==Result::Unsupported && f.bytes==original);
        f.settings.alpha_ref=1;f.vertices[0].x=std::numeric_limits<float>::quiet_NaN();
        assert(RasterWorkers::execute(&workers,f.vertices,f.source,f.target,f.viewport,f.settings,stats)==Result::Unsupported && f.bytes==original);
        workers.shutdown();assert(stub::creates==2 && stub::joins==2);
        puts("PASS small draws stay serial and invalid live-pool calls cannot dispatch or mutate storage");
    }
    for(unsigned fail:{1u,2u}){
        stub::reset();stub::fail_create=fail;Fixture f;RasterWorkers workers(f.log);f.check(workers);
        f.check(workers);workers.shutdown();assert(stub::creates==fail && stub::starts==fail-1 && stub::joins==fail-1 && stub::deletes==fail-1);
    }
    puts("PASS first/second worker creation failures join prior workers and fall back before drawing");
    for(unsigned fail:{1u,2u}){
        stub::reset();stub::fail_start=fail;Fixture f;RasterWorkers workers(f.log);f.check(workers);
        workers.shutdown();assert(stub::starts==fail-1 && stub::joins==fail-1 && stub::deletes==fail);
    }
    puts("PASS first/second start failures balance started and unstarted native thread lifetimes");
    for(unsigned fail:{1u,2u}){
        stub::reset();stub::bad_affinity=fail;Fixture f;RasterWorkers workers(f.log);f.check(workers);
        workers.shutdown();assert(stub::starts==fail && stub::joins==fail && stub::deletes==fail);
    }
    puts("PASS incorrect affinity readback refuses parallel mode and preserves serial output");
    stub::reset();
    {
        Fixture f;f.vertices[0].y=f.vertices[1].y=700;f.vertices[2].y=f.vertices[3].y=800;
        RasterWorkers workers(f.log);const auto original=f.bytes;f.check(workers);
        assert(f.bytes==original && stub::creates==0);workers.shutdown();
        puts("PASS fully clipped draw starts no workers, writes no bytes and returns genuine empty coverage");
    }
}
