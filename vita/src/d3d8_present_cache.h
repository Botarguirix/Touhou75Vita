#pragma once
#include "d3d8_quad_raster.h"
#include <cstring>
#include <memory>
#include <new>

namespace d3d8_present {
// A copy of the last *confirmed* native Present source. Equality is checked
// across every source byte, not with a hash. Scanout stays owned by ExePresenter.
// Only the observed tightly packed X8R8G8B8 backbuffer can use this cache.
class FrameCache {
public:
    static constexpr size_t source_bytes=640u*480u*4u;
    bool lookup(const d3d8_quad::Image& source,unsigned& slot,uint32_t& hash) const {
        if(!valid_ || !profile(source) ||
            std::memcmp(snapshot_.get(),source.bytes,source_bytes))return false;
        slot=slot_;hash=hash_;return true;
    }
    bool remember_confirmed(const d3d8_quad::Image& source,unsigned slot,uint32_t hash) {
        invalidate();
        if(slot>1 || !profile(source))return false;
        if(!snapshot_)snapshot_.reset(new(std::nothrow) uint8_t[source_bytes]);
        if(!snapshot_)return false; // Normal conversion remains available.
        std::memcpy(snapshot_.get(),source.bytes,source_bytes);
        slot_=slot;hash_=hash;valid_=true;return true;
    }
    void invalidate(){valid_=false;}
private:
    static bool profile(const d3d8_quad::Image& source) {
        return source.width==640 && source.height==480 && source.pitch==2560 &&
            source.format==22 && source.size==source_bytes &&
            d3d8_quad::detail::storage(source.bytes,source.size,640,480,2560,4);
    }
    std::unique_ptr<uint8_t[]> snapshot_;
    bool valid_=false;unsigned slot_=0;uint32_t hash_=0;
};
}
