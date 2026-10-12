#include "../vita/src/d3d8_render_scale.h"
#include "../vita/src/d3d8_raster_executor.h"
#include <chrono>
#include <cstdio>
#include <cassert>

using namespace d3d8_quad;
int main() {
    std::vector<uint8_t> source(512*512*4),full(640*480*4);
    uint32_t random=75;for(size_t i=0;i<source.size();i+=4) {
        random=random*1664525u+1013904223u;detail::store32(source.data()+i,random|0x80000000);
    }
    Image image{source.data(),source.size(),512,512,2048,21};
    HalfSurface half;half.initialize(640,480,22);uint64_t pixels[2]={},us[2]={};uint32_t digests[2]={};
    for(unsigned mode=0;mode<2;++mode) {
        Target target=mode?half.target():Target{full.data(),full.size(),640,480,2560,22};
        const auto started=std::chrono::steady_clock::now();
        // Synthetic CPU raster workload, not an EXE or a Vita FPS claim.
        for(unsigned frame=0;frame<16;++frame)for(unsigned draw=0;draw<12;++draw) {
            const float x=draw<2?-.5f:float((draw*29)%400)-.5f,
                y=draw<2?-.5f:float((draw*19)%300)-.5f,
                width=draw<2?640.f:160.f,height=draw<2?480.f:48.f;
            Vertex v[4]={{x,y,.5f,1,0xFFCCCCCC,0,0},{x+width,y,.5f,1,0xFFCCCCCC,1,0},
                {x,y+height,.5f,1,0xFFCCCCCC,0,1},{x+width,y+height,.5f,1,0xFFCCCCCC,1,1}};
            if(mode)half.vertices(v);
            Settings settings;settings.filter=2;Stats stats;
            assert(execute_serial_no_hashes(nullptr,v,image,target,{0,0,target.width,target.height},settings,stats)==Result::Rendered);
            pixels[mode]+=stats.covered;
        }
        us[mode]=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count();
        uint32_t hash=2166136261;for(size_t i=0;i<target.size;i+=4)hash=detail::hash(hash,detail::load32(target.bytes+i));digests[mode]=hash;
    }
    assert(pixels[0]==pixels[1]*4);
    printf("{\"scope\":\"synthetic_host_cpu_not_vita_fps\",\"full_us\":%llu,\"half_us\":%llu,\"full_pixels\":%llu,\"half_pixels\":%llu,\"ratio\":%.4f,\"full_digest\":\"%08X\",\"half_digest\":\"%08X\"}\n",
        (unsigned long long)us[0],(unsigned long long)us[1],(unsigned long long)pixels[0],(unsigned long long)pixels[1],double(us[0])/us[1],digests[0],digests[1]);
}
