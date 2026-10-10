#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

// Owned software path for the axis-aligned, pre-transformed four-vertex strip
// observed in the original TH075 startup. The caller must first check the FVF,
// scene, stage and render state profile; this helper is not a general renderer.
// Pixel centers are integer positions, and right/bottom edges are excluded.
// See Microsoft's Direct3D rasterization rules, nearest-point sampling and
// Directly Mapping Texels to Pixels documentation. No source code was copied.
namespace d3d8_quad {

struct Vertex {
    float x, y, z, rhw;
    uint32_t diffuse;
    float u, v;
};
static_assert(sizeof(Vertex)==28,"XYZRHW/diffuse/UV vertex ABI");

struct Image {
    const uint8_t* bytes;
    size_t size;
    uint32_t width, height, pitch, format;
};
struct Target {
    uint8_t* bytes;
    size_t size;
    uint32_t width, height, pitch, format;
};
struct Viewport { uint32_t x, y, width, height; };
struct Settings {
    uint32_t alpha_ref=1;
    bool alpha_test=true, alpha_blend=true;
    bool replace_blend=false; // D3DBLEND_ONE/ZERO with ADD.
    uint32_t filter=1; // D3DTEXF_POINT=1, D3DTEXF_LINEAR=2.
};

// Storage owns exactly one mip level. MIP NONE/POINT/LINEAR all select that
// level; different min/mag modes and anisotropic modes remain unsupported.
inline bool single_level_filter(uint32_t mag,uint32_t min,uint32_t mip) {
    return (mag==1 || mag==2) && min==mag && mip<=2;
}
enum class Result { Unsupported, Rendered };
enum class Reject { None, Dimensions, Format, Storage, Alias, Vertex, Quad, Settings };
struct Probe {
    uint32_t x=0, y=0, texel_x=0, texel_y=0;
    uint32_t source=0, before=0, after=0;
};
struct Stats {
    Reject reject=Reject::None;
    uint32_t covered=0, alpha_rejected=0, written=0, changed=0;
    // FNV-1a over little-endian ARGB DWORDs in covered-pixel row order.
    // Rejected pixels are included in before/after and modulated-source hashes.
    uint32_t hash_before=2166136261u, hash_after=2166136261u,
        hash_source=2166136261u;
    Probe first{}, last{};
};

inline const char* reject_name(Reject r) {
    switch(r) {
    case Reject::None:return "none";
    case Reject::Dimensions:return "dimensions";
    case Reject::Format:return "format";
    case Reject::Storage:return "storage";
    case Reject::Alias:return "source_target_alias";
    case Reject::Vertex:return "vertex";
    case Reject::Quad:return "quad";
    case Reject::Settings:return "settings";
    }
    return "unknown";
}

namespace detail {
inline Result reject(Stats& stats,Reject reason) {
    stats.reject=reason;
    return Result::Unsupported;
}
inline bool storage(const uint8_t* bytes,size_t size,uint32_t width,
    uint32_t height,uint32_t pitch,unsigned bpp) {
    if(!bytes || uint64_t(pitch)<uint64_t(width)*bpp)return false;
    const uint64_t used=uint64_t(height-1)*pitch+uint64_t(width)*bpp;
    const uintptr_t begin=reinterpret_cast<uintptr_t>(bytes);
    return used<=size && size<=std::numeric_limits<uintptr_t>::max()-begin;
}
inline uint32_t load32(const uint8_t* p) {
    return uint32_t(p[0]) | (uint32_t(p[1])<<8) |
        (uint32_t(p[2])<<16) | (uint32_t(p[3])<<24);
}
inline void store32(uint8_t* p,uint32_t color) {
    p[0]=uint8_t(color);p[1]=uint8_t(color>>8);
    p[2]=uint8_t(color>>16);p[3]=uint8_t(color>>24);
}
inline uint32_t sample(const uint8_t* p,uint32_t format) {
    if(format==21)return load32(p); // A8R8G8B8, little-endian BGRA bytes.
    const uint32_t word=uint32_t(p[0]) | (uint32_t(p[1])<<8);
    const uint32_t r=(word>>10)&31,g=(word>>5)&31,b=word&31;
    // A1R5G5B5: normalize five-bit channels to eight bits with nearest
    // rounding. Alpha stays binary. Format 21 needs no such conversion.
    return ((word&0x8000)?0xFF000000u:0u) | (((r*255+15)/31)<<16) |
        (((g*255+15)/31)<<8) | ((b*255+15)/31);
}
inline uint32_t hash(uint32_t h,uint32_t color) {
    for(unsigned shift=0;shift<32;shift+=8)h=(h^((color>>shift)&255))*16777619u;
    return h;
}
inline uint32_t modulate(uint32_t texture,uint32_t diffuse) {
    if(diffuse==0xFFFFFFFFu)return texture;
    uint32_t color=0;
    for(unsigned shift=0;shift<32;shift+=8) {
        const uint32_t a=(texture>>shift)&255,b=(diffuse>>shift)&255;
        color|=((a*b+127)/255)<<shift;
    }
    return color;
}
inline uint32_t blend(uint32_t source,uint32_t destination) {
    const uint32_t alpha=source>>24;
    if(alpha==255)return source;
    if(!alpha)return destination;
    uint32_t color=0;
    // D3DBLEND_SRCALPHA/INVSRCALPHA apply the source-alpha factor to all
    // four channels. Destination alpha therefore uses As*As+Ad*(1-As).
    for(unsigned shift=0;shift<32;shift+=8) {
        const uint32_t s=(source>>shift)&255,d=(destination>>shift)&255;
        color|=((s*alpha+d*(255-alpha)+127)/255)<<shift;
    }
    return color;
}
inline uint16_t wrapped_point(double coordinate,uint32_t extent) {
    // Nearest-point sampling maps u*N-.5 to the nearest texel; floor(u*N)
    // selects the upper texel at exact boundaries. WRAP keeps the result in
    // the owned level, including negative UVs and the u==1 seam.
    const double fraction=coordinate-std::floor(coordinate);
    const double texel=std::floor(fraction*extent);
    return uint16_t(std::min(double(extent-1),texel));
}
struct LinearAxis { uint16_t first=0,second=0; double weight=0; };
inline LinearAxis wrapped_linear(double coordinate,uint32_t extent) {
    // Texel centers are (i+.5)/N. Wrap EACH neighbor at the texture seam.
    // Reducing UV before scaling also bounds negative/large coordinates.
    const double texel=(coordinate-std::floor(coordinate))*extent-.5;
    const double lower=std::floor(texel);
    const int index=int(lower); // Bounded to [-1,1023].
    const uint16_t first=uint16_t(index<0?int(extent)-1:index);
    return {first,uint16_t((uint32_t(first)+1)%extent),texel-lower};
}
inline uint32_t bilinear(uint32_t a,uint32_t b,uint32_t c,uint32_t d,
    double wx,double wy) {
    uint32_t color=0;
    // Interpolate straight RGBA before MODULATE/alpha test/blending. Round
    // only the final channel, never each intermediate horizontal sample.
    // This is a software model; hardware filter precision can differ by 1.
    for(unsigned shift=0;shift<32;shift+=8) {
        const double top=((a>>shift)&255)*(1-wx)+((b>>shift)&255)*wx,
            bottom=((c>>shift)&255)*(1-wx)+((d>>shift)&255)*wx;
        const uint32_t channel=uint32_t(std::floor(top*(1-wy)+bottom*wy+.5));
        color|=std::min(channel,255u)<<shift;
    }
    return color;
}
} // namespace detail

// Validates every failure condition before the first destination write.
// Accepted geometry: TL,TR,BL,BR with constant Z/diffuse, RHW=1, separable UVs.
// The caller guarantees depth disabled, no shader/fog/lighting, solid fill,
// no culling, MODULATE(TEXTURE,DIFFUSE/CURRENT) at stage zero, WRAP U/V,
// Matching POINT or LINEAR min/mag filters, and only one texture level.
// Linear probes report the upper-left wrapped neighbor and filtered color.
// Alpha comparison is
// GREATEREQUAL; enabled blending is SRCALPHA/INVSRCALPHA or ONE/ZERO, ADD.
// Destinations are A8R8G8B8 or X8R8G8B8. The latter stores X=0xFF on writes;
// it has no destination-alpha channel, so only the blended RGB is retained.
inline Result rasterize(const Vertex (&vertices)[4],const Image& source,
    const Target& target,const Viewport& viewport,const Settings& settings,
    Stats& stats) {
    stats=Stats{};
    if(!source.width || !source.height || source.width>1024 || source.height>1024 ||
        !target.width || !target.height || target.width>1024 || target.height>1024 ||
        !viewport.width || !viewport.height ||
        uint64_t(viewport.x)+viewport.width>target.width ||
        uint64_t(viewport.y)+viewport.height>target.height)
        return detail::reject(stats,Reject::Dimensions);
    if((source.format!=21 && source.format!=25) ||
        (target.format!=21 && target.format!=22))
        return detail::reject(stats,Reject::Format);
    const unsigned bpp=source.format==25?2:4;
    if(!detail::storage(source.bytes,source.size,source.width,source.height,source.pitch,bpp) ||
        !detail::storage(target.bytes,target.size,target.width,target.height,target.pitch,4))
        return detail::reject(stats,Reject::Storage);
    const uintptr_t sb=reinterpret_cast<uintptr_t>(source.bytes),
        tb=reinterpret_cast<uintptr_t>(target.bytes);
    if(sb<tb+target.size && tb<sb+source.size)
        return detail::reject(stats,Reject::Alias);
    if(settings.alpha_ref>255 || (settings.filter!=1 && settings.filter!=2))
        return detail::reject(stats,Reject::Settings);
    for(const auto& v:vertices) {
        if(!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z) ||
            !std::isfinite(v.rhw) || !std::isfinite(v.u) || !std::isfinite(v.v) ||
            std::fabs(v.x)>65536 || std::fabs(v.y)>65536 ||
            std::fabs(v.u)>65536 || std::fabs(v.v)>65536 ||
            v.rhw!=1 || v.z<0 || v.z>1 || v.z!=vertices[0].z ||
            v.diffuse!=vertices[0].diffuse)
            return detail::reject(stats,Reject::Vertex);
    }
    const auto& tl=vertices[0];const auto& tr=vertices[1];
    const auto& bl=vertices[2];const auto& br=vertices[3];
    if(tl.x!=bl.x || tr.x!=br.x || tl.y!=tr.y || bl.y!=br.y ||
        !(tl.x<tr.x) || !(tl.y<bl.y) || tl.u!=bl.u || tr.u!=br.u ||
        tl.v!=tr.v || bl.v!=br.v)
        return detail::reject(stats,Reject::Quad);

    const double left=tl.x,right=tr.x,top=tl.y,bottom=bl.y;
    const double x_begin=std::max(std::ceil(left),double(viewport.x)),
        x_end=std::min(std::ceil(right),double(viewport.x+viewport.width)),
        y_begin=std::max(std::ceil(top),double(viewport.y)),
        y_end=std::min(std::ceil(bottom),double(viewport.y+viewport.height));
    if(x_begin>=x_end || y_begin>=y_end)return Result::Rendered;
    // After clipping these integer conversions are in [0,1024].
    const uint32_t x0=uint32_t(x_begin),x1=uint32_t(x_end),
        y0=uint32_t(y_begin),y1=uint32_t(y_end);
    const double du=(double(tr.u)-tl.u)/(right-left),
        dv=(double(bl.v)-tl.v)/(bottom-top);
    // Bounded stack tables remove sampling divides/floors from the pixel loop.
    std::array<detail::LinearAxis,1024> xs{},ys{};
    for(uint32_t x=x0;x<x1;++x) {
        const double u=double(tl.u)+(double(x)-left)*du;
        if(settings.filter==2)xs[x]=detail::wrapped_linear(u,source.width);
        else xs[x].first=detail::wrapped_point(u,source.width);
    }
    for(uint32_t y=y0;y<y1;++y) {
        const double v=double(tl.v)+(double(y)-top)*dv;
        if(settings.filter==2)ys[y]=detail::wrapped_linear(v,source.height);
        else ys[y].first=detail::wrapped_point(v,source.height);
    }

    for(uint32_t y=y0;y<y1;++y) {
        const uint8_t* source_row=source.bytes+size_t(ys[y].first)*source.pitch;
        const uint8_t* next_row=settings.filter==2?
            source.bytes+size_t(ys[y].second)*source.pitch:source_row;
        uint8_t* target_row=target.bytes+size_t(y)*target.pitch;
        for(uint32_t x=x0;x<x1;++x) {
            uint32_t texel=detail::sample(source_row+size_t(xs[x].first)*bpp,source.format);
            if(settings.filter==2)texel=detail::bilinear(texel,
                detail::sample(source_row+size_t(xs[x].second)*bpp,source.format),
                detail::sample(next_row+size_t(xs[x].first)*bpp,source.format),
                detail::sample(next_row+size_t(xs[x].second)*bpp,source.format),
                xs[x].weight,ys[y].weight);
            const uint32_t color=detail::modulate(texel,tl.diffuse),
                before=detail::load32(target_row+size_t(x)*4);
            uint32_t after=before;
            if(settings.alpha_test && (color>>24)<settings.alpha_ref)++stats.alpha_rejected;
            else {
                after=settings.alpha_blend && !settings.replace_blend?detail::blend(color,before):color;
                // X8 has no stored alpha result. Source alpha still governs
                // the alpha test and RGB blend; rejected pixels are untouched.
                // Hashes/probes retain the actual DWORD before and after,
                // including normalization of an existing undefined X byte.
                if(target.format==22)after|=0xFF000000u;
                detail::store32(target_row+size_t(x)*4,after);
                ++stats.written;
                if(after!=before)++stats.changed;
            }
            const Probe probe{x,y,xs[x].first,ys[y].first,color,before,after};
            if(!stats.covered)stats.first=probe;
            stats.last=probe;
            ++stats.covered;
            stats.hash_source=detail::hash(stats.hash_source,color);
            stats.hash_before=detail::hash(stats.hash_before,before);
            stats.hash_after=detail::hash(stats.hash_after,after);
        }
    }
    return Result::Rendered;
}
} // namespace d3d8_quad
