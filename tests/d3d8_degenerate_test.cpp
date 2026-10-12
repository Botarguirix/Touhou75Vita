#include "../vita/src/d3d8_raster_executor.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace d3d8_quad;

int main() {
    uint32_t pixel=0x80402010;
    Image source{reinterpret_cast<const uint8_t*>(&pixel),4,1,1,4,21};
    std::vector<uint8_t> bytes(16*16*4,0xA5);const auto original=bytes;
    Target target{bytes.data(),bytes.size(),16,16,64,21};Viewport view{0,0,16,16};
    Settings settings;Stats stats;Vertex q[4];
    auto point=[&](float x,float y) {for(auto& v:q)v={x,y,.5f,1,0xFFFFFFFF,0,0};};
    auto no_op=[&] {
        assert(rasterize(q,source,target,view,settings,stats,true)==Result::Rendered);
        assert(stats.degenerate_noop && stats.triangle_strip && stats.reject==Reject::None);
        assert(rasterize(q,source,target,view,settings,stats)==Result::Rendered);
        assert(stats.degenerate_noop && stats.covered==0 && stats.written==0 && stats.changed==0 && bytes==original);
        assert(stats.hash_before==2166136261u && stats.hash_after==2166136261u && stats.hash_source==2166136261u);
    };
    point(5,7);no_op();point(-.5f,-.5f);no_op();
    // Selection's original transform is (vertex - pivot)*scale + pivot;
    // its initialized offset 80 and denominator 80 produce scale zero.
    for(auto& v:q){v.x=(v.x-5)*0+5;v.y=(v.y-7)*0+7;}
    no_op();
    puts("PASS collapsed point and original zero-scale transform produce no fragments or writes");

    for(unsigned axis=0;axis<3;++axis) {
        point(0,0);
        for(unsigned i=0;i<4;++i){q[i].x=axis==1?3:float(i*2);q[i].y=axis==0?5:float(i*2);q[i].u=float(i)/4;}
        no_op();
    }
    puts("PASS horizontal vertical and oblique collinear strips remain valid no-ops");

    point(0,0);q[0].x=2;q[0].y=3;q[1].x=q[2].x=7;q[1].y=q[2].y=8;q[3].x=12;q[3].y=1;
    no_op();
    puts("PASS repeated shared edge gives two zero-area triangles without inventing a polygon");

    point(0,0);q[1].x=8;q[2].y=8;q[3]=q[1];
    assert(rasterize(q,source,target,view,settings,stats)==Result::Unsupported && stats.reject==Reject::Quad && bytes==original);
    puts("PASS partially degenerate strips stay unsupported before all destination writes");

    auto rejected=[&](Reject reason){assert(rasterize(q,source,target,view,settings,stats)==Result::Unsupported && stats.reject==reason && bytes==original);};
    point(2,2);q[0].rhw=.5f;rejected(Reject::Vertex);
    point(2,2);q[1].u=std::numeric_limits<float>::infinity();rejected(Reject::Vertex);
    point(2,2);q[0].diffuse=0;rejected(Reject::Vertex);
    point(2,2);settings.filter=3;rejected(Reject::Settings);settings.filter=1;
    Image saved=source;source={target.bytes,target.size,16,16,64,21};rejected(Reject::Alias);source=saved;
    source.size=3;rejected(Reject::Storage);source=saved;
    source.format=80;rejected(Reject::Format);source=saved;
    puts("PASS no-op geometry still validates finite vertices settings formats storage and aliasing");

    point(0,0);q[1].x=q[3].x=0.000001f;q[2].y=q[3].y=4;
    assert(rasterize(q,source,target,view,settings,stats)==Result::Rendered && !stats.degenerate_noop);
    bytes=original;point(4,4);
    assert(execute_serial_fast(nullptr,q,source,target,view,settings,stats)==Result::Rendered);
    assert(stats.degenerate_noop && !stats.hashes_valid && !stats.probes_valid && bytes==original);
    puts("PASS tiny nonzero area is not snapped and shipping serial no-op preserves the target");
}
