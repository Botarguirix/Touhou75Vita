// Synthetic serial CPU benchmark. This does not execute the EXE or Vita APIs.
#ifdef TH075_RASTER_REFERENCE
#include TH075_RASTER_REFERENCE
#else
#include "../vita/src/d3d8_raster_executor.h"
#endif
#include <cassert>
#include <chrono>
#include <cstdio>
#include <vector>

using namespace d3d8_quad;
int main() {
    std::vector<uint8_t> texture(256*256*4),pixels(320*240*4);
    const Image image{texture.data(),texture.size(),256,256,1024,21};
    const Target target{pixels.data(),pixels.size(),320,240,1280,22};
    const char* names[]={"point_mixed_alpha","linear_transparent_rgb","linear_mixed_alpha"};
    puts("{\"scope\":\"synthetic_host_cpu_not_vita_fps\",\"cases\":[");
    for(unsigned test=0;test<3;++test) {
        uint32_t state=1975;
        for(size_t i=0;i<texture.size();i+=4) {
            state=state*1664525+1013904223;
            const uint32_t alpha=test==1?0:((state>>24)&1)?255:((state>>24)&2)?128:0;
            detail::store32(texture.data()+i,(state&0x00FFFFFFu)|(alpha<<24));
        }
        for(size_t i=0;i<pixels.size();i+=4)detail::store32(pixels.data()+i,0x00123456u);
        Vertex v[4]={{-.5f,-.5f,.5f,1,0xA0FFFFFF,-.03f,.01f},
            {319.5f,-.5f,.5f,1,0xA0FFFFFF,.97f,.01f},
            {-.5f,239.5f,.5f,1,0xA0FFFFFF,-.03f,1.01f},
            {319.5f,239.5f,.5f,1,0xA0FFFFFF,.97f,1.01f}};
        Settings settings;settings.filter=test?2:1;
        uint64_t shortcuts=0,covered=0;
        const auto started=std::chrono::steady_clock::now();
        for(unsigned draw=0;draw<64;++draw) {
            Stats stats;
#ifdef TH075_RASTER_REFERENCE
            settings.hash_pixels=false;
            assert(rasterize(v,image,target,{0,0,320,240},settings,stats)==Result::Rendered);
#else
            assert(execute_serial_fast(nullptr,v,image,target,{0,0,320,240},settings,stats)==Result::Rendered);
            shortcuts+=stats.alpha_shortcuts;
#endif
            covered+=stats.covered;
        }
        const auto us=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count();
        uint32_t digest=2166136261u;for(uint8_t byte:pixels)digest=(digest^byte)*16777619u;
        printf("{\"name\":\"%s\",\"us\":%llu,\"pixels\":%llu,\"shortcuts\":%llu,\"digest\":\"%08X\"}%s\n",
            names[test],(unsigned long long)us,(unsigned long long)covered,
            (unsigned long long)shortcuts,digest,test==2?"":",");
    }
    puts("]}");
}
