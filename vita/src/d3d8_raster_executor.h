#pragma once
#include "d3d8_quad_raster.h"

// Optional synchronous native executor. Returning means every destination write
// has finished; guest execution/resource mutation cannot overlap worker jobs.
namespace d3d8_quad {
using RasterExecutor=Result(*)(void*,const Vertex (&)[4],const Image&,
    const Target&,const Viewport&,const Settings&,Stats&);

inline std::array<Viewport,3> row_bands(const Vertex (&v)[4],const Viewport& vp) {
    double top=v[0].y,bottom=top;
    for(const auto& vertex:v){top=std::min(top,double(vertex.y));bottom=std::max(bottom,double(vertex.y));}
    const uint32_t first=uint32_t(std::max(std::ceil(top),double(vp.y))),
        last=uint32_t(std::max(double(first),std::min(std::ceil(bottom),double(vp.y+vp.height))));
    std::array<Viewport,3> bands{};
    for(unsigned i=0;i<3;++i) {
        const uint32_t begin=first+(last-first)*i/3,end=first+(last-first)*(i+1)/3;
        bands[i]={vp.x,begin,vp.width,end-begin};
    }
    return bands;
}
inline uint64_t clipped_box_pixels(const Vertex (&v)[4],const Viewport& vp) {
    double left=v[0].x,right=left,top=v[0].y,bottom=top;
    for(const auto& vertex:v){left=std::min(left,double(vertex.x));right=std::max(right,double(vertex.x));
        top=std::min(top,double(vertex.y));bottom=std::max(bottom,double(vertex.y));}
    const double width=std::max(0.,std::min(std::ceil(right),double(vp.x+vp.width))-std::max(std::ceil(left),double(vp.x))),
        height=std::max(0.,std::min(std::ceil(bottom),double(vp.y+vp.height))-std::max(std::ceil(top),double(vp.y)));
    return uint64_t(width)*uint64_t(height);
}
// Hashes are intentionally disabled for the parallel path: FNV row streams
// cannot be naively concatenated. Preserve row-major probes/counters exactly.
inline Stats combine_rows(const std::array<Stats,3>& parts) {
    Stats combined{};combined.hashes_valid=false;
    for(const auto& p:parts) {
        if(p.covered){if(!combined.covered)combined.first=p.first;combined.last=p.last;}
        combined.covered+=p.covered;combined.alpha_rejected+=p.alpha_rejected;
        combined.written+=p.written;combined.changed+=p.changed;
        combined.linear_exact+=p.linear_exact;combined.linear_constant+=p.linear_constant;
        combined.linear_reference+=p.linear_reference;
        combined.triangle_strip=combined.triangle_strip || p.triangle_strip;
        if(p.reject!=Reject::None)combined.reject=p.reject;
    }
    return combined;
}
}
