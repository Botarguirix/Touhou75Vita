#pragma once
#include "d3d8_quad_raster.h"
#include <vector>
#include <cstring>

namespace d3d8_quad {
// Native reduced render surface. Guest descriptors/LockRect retain their
// original dimensions. Only color targets have a companion; DAT textures
// stay at their original resolution. This is an explicit lossy FPS profile.
class HalfSurface {
public:
    static size_t required(uint32_t w,uint32_t h) {
        return size_t((w+1)/2)*((h+1)/2)*4;
    }
    void initialize(uint32_t w,uint32_t h,uint32_t format) {
        full_width_=w;full_height_=h;format_=format;
        width_=(w+1)/2;height_=(h+1)/2;bytes_.resize(required(w,h));
    }
    bool active() const {return !bytes_.empty();}
    size_t size() const {return bytes_.size();}
    Image image() const {return {bytes_.data(),bytes_.size(),width_,height_,width_*4,format_};}
    Target target() {return {bytes_.data(),bytes_.size(),width_,height_,width_*4,format_};}
    Viewport viewport(const Viewport& full) const {
        const auto edge=[](uint32_t n,uint32_t reduced,uint32_t extent) {
            // Select reduced integer centers inside the original viewport.
            return uint32_t(std::max(0.,std::min(double(reduced),
                std::ceil((double(n)+.5)*reduced/extent-.5))));
        };
        const uint32_t x=edge(full.x,width_,full_width_),y=edge(full.y,height_,full_height_),
            right=edge(full.x+full.width,width_,full_width_),
            bottom=edge(full.y+full.height,height_,full_height_);
        return {x,y,right-x,bottom-y};
    }
    void vertices(Vertex (&v)[4]) const {
        for(auto& p:v) {
            // D3D8 integer pixel centers: retain the half-texel offset.
            p.x=float((double(p.x)+.5)*width_/full_width_-.5);
            p.y=float((double(p.y)+.5)*height_/full_height_-.5);
        }
    }
    void clear(const Viewport& full,uint32_t color) {
        const auto v=viewport(full);
        for(uint32_t y=v.y;y<v.y+v.height;++y)
            for(uint32_t x=v.x;x<v.x+v.width;++x)
                detail::store32(bytes_.data()+(size_t(y)*width_+x)*4,color);
    }
    bool upload(const Image& full) {
        if(!matches(full.bytes,full.size,full.width,full.height,full.pitch,full.format))return false;
        for(uint32_t y=0;y<height_;++y)for(uint32_t x=0;x<width_;++x) {
            const uint32_t sx=uint32_t((uint64_t(2*x+1)*full_width_)/(2*width_)),
                sy=uint32_t((uint64_t(2*y+1)*full_height_)/(2*height_));
            std::memcpy(bytes_.data()+(size_t(y)*width_+x)*4,
                full.bytes+size_t(sy)*full.pitch+size_t(sx)*4,4);
        }
        return true;
    }
    bool readback(const Target& full) const {
        if(!matches(full.bytes,full.size,full.width,full.height,full.pitch,full.format))return false;
        for(uint32_t y=0;y<full_height_;++y)for(uint32_t x=0;x<full_width_;++x) {
            const uint32_t sx=uint64_t(x)*width_/full_width_,sy=uint64_t(y)*height_/full_height_;
            std::memcpy(full.bytes+size_t(y)*full.pitch+size_t(x)*4,
                bytes_.data()+(size_t(sy)*width_+sx)*4,4);
        }
        return true;
    }
private:
    bool matches(const uint8_t* bytes,size_t size,uint32_t w,uint32_t h,uint32_t pitch,uint32_t format) const {
        return active() && w==full_width_ && h==full_height_ && format==format_ &&
            detail::storage(bytes,size,w,h,pitch,4);
    }
    uint32_t full_width_=0,full_height_=0,width_=0,height_=0,format_=0;
    std::vector<uint8_t> bytes_;
};
}
