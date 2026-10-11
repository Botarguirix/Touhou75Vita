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
        if(!valid_ || !profile(source) || source.width!=stored_width_ || source.height!=stored_height_ ||
            std::memcmp(snapshot_.get(),source.bytes,source.size))return false;
        slot=slot_;hash=hash_;return true;
    }
    bool remember_confirmed(const d3d8_quad::Image& source,unsigned slot,uint32_t hash) {
        invalidate();
        if(slot>1 || !profile(source))return false;
        if(!snapshot_)snapshot_.reset(new(std::nothrow) uint8_t[source_bytes]);
        if(!snapshot_)return false; // Normal conversion remains available.
        std::memcpy(snapshot_.get(),source.bytes,source.size);
        stored_width_=source.width;stored_height_=source.height;
        slot_=slot;hash_=hash;valid_=true;return true;
    }
    void invalidate(){valid_=false;}
private:
    static bool profile(const d3d8_quad::Image& source) {
        return ((source.width==640 && source.height==480) || (source.width==320 && source.height==240)) && source.pitch==source.width*4 &&
            source.format==22 && source.size==size_t(source.width)*source.height*4 &&
            d3d8_quad::detail::storage(source.bytes,source.size,source.width,source.height,source.pitch,4);
    }
    std::unique_ptr<uint8_t[]> snapshot_;
    bool valid_=false;unsigned slot_=0;uint32_t hash_=0,stored_width_=0,stored_height_=0;
};
}
