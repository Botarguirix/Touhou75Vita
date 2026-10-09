#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace th075 {
// A diagnostic snapshot of the first rendered EXE quad, not a guest Present.
inline std::array<uint32_t,260*180> exe_draw_preview{};
inline bool exe_draw_ready=false;
inline uint32_t exe_draw_width=0,exe_draw_height=0;

inline void capture_first_exe_quad(const uint8_t* bytes,size_t size,
    uint32_t width,uint32_t height,uint32_t pitch,uint32_t x0,uint32_t y0,
    uint32_t view_width,uint32_t view_height,FILE* log) {
    if(exe_draw_ready || !bytes || !view_width || !view_height ||
        uint64_t(x0)+view_width>width || uint64_t(y0)+view_height>height ||
        uint64_t(pitch)<uint64_t(width)*4 ||
        uint64_t(height-1)*pitch+uint64_t(width)*4>size)return;
    exe_draw_preview.fill(0xFF20130D);
    unsigned dw=260,dh=uint64_t(view_height)*260/view_width;
    if(dh>180){dh=180;dw=uint64_t(view_width)*180/view_height;}
    dw=std::max(dw,1u);dh=std::max(dh,1u);
    for(unsigned y=0;y<dh;++y)for(unsigned x=0;x<dw;++x){
        const unsigned sx=x0+uint64_t(x)*view_width/dw,sy=y0+uint64_t(y)*view_height/dh;
        uint32_t argb=0;std::memcpy(&argb,bytes+size_t(sy)*pitch+size_t(sx)*4,4);
        // Display stored RGB opaquely; intermediate render-target alpha remains
        // intact in owned storage and is not recomposited by this preview.
        exe_draw_preview[((180-dh)/2+y)*260+(260-dw)/2+x]=
            0xFF000000u | ((argb&255u)<<16) | (argb&0xFF00u) | ((argb>>16)&255u);
    }
    exe_draw_width=view_width;exe_draw_height=view_height;exe_draw_ready=true;
    fprintf(log,"startup_d3d8_draw_capture=ready viewport:%u,%u,%u,%u preview:260x180 scope:first_quad_rgb_not_present\n",x0,y0,view_width,view_height);
}
inline void draw_exe_snapshot(uint32_t* target){
    if(!exe_draw_ready)return;
    for(unsigned y=0;y<180;++y)std::memcpy(target+(228+y)*960+690,exe_draw_preview.data()+y*260,260*4);
}
}
