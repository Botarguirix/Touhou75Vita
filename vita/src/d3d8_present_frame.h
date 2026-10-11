#pragma once
#include "d3d8_quad_raster.h"
#include <array>
#include <algorithm>

namespace d3d8_present {
constexpr unsigned width=960,height=544;
inline bool full_rect(const std::array<int32_t,4>& rect,uint32_t w,uint32_t h) {
    return rect==std::array<int32_t,4>{0,0,int32_t(w),int32_t(h)};
}
// Current window contract: complete 640x480 client/backbuffer only. Prepare
// an opaque ABGR scanout with preserved aspect ratio and black side bars.
// Validation precedes writes. No guest memory or Vita calls in this helper.
inline bool prepare(const d3d8_quad::Image& source,uint32_t* output,
    size_t output_pixels,uint32_t& hash,bool hash_pixels=true) {
    if(source.width!=640 || source.height!=480 ||
        (source.format!=21 && source.format!=22) || !output ||
        output_pixels<width*height ||
        !d3d8_quad::detail::storage(source.bytes,source.size,640,480,source.pitch,4))return false;
    const uintptr_t sb=reinterpret_cast<uintptr_t>(source.bytes),
        ob=reinterpret_cast<uintptr_t>(output);
    const size_t output_bytes=width*height*sizeof(uint32_t);
    if(output_bytes>std::numeric_limits<uintptr_t>::max()-ob ||
        (sb<ob+output_bytes && ob<sb+source.size))return false;
    std::fill_n(output,width*height,0xFF000000u);
    constexpr unsigned draw_width=height*640/480,left=(width-draw_width)/2;
    std::array<uint16_t,draw_width> xs{};
    for(unsigned x=0;x<draw_width;++x)xs[x]=x*640/draw_width;
    for(unsigned y=0;y<height;++y) {
        const uint8_t* row=source.bytes+size_t(y*480/height)*source.pitch;
        for(unsigned x=0;x<draw_width;++x) {
            const uint32_t argb=d3d8_quad::detail::load32(row+xs[x]*4);
            output[y*width+left+x]=0xFF000000u | ((argb&255)<<16) |
                (argb&0xFF00u) | ((argb>>16)&255);
        }
    }
    hash=0; // Explicitly absent in performance mode; never used for equality.
    if(hash_pixels) {
        hash=2166136261u;
        for(unsigned i=0;i<width*height;++i)hash=d3d8_quad::detail::hash(hash,output[i]);
    }
    return true;
}
}
