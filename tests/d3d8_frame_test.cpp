#include "../vita/src/d3d8_quad_raster.h"
#include "../vita/src/d3d8_present_frame.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include <limits>

using namespace d3d8_quad;
static void fill(std::vector<uint8_t>& bytes,uint32_t value) {
    for(size_t i=0;i<bytes.size();i+=4)detail::store32(bytes.data()+i,value);
}
static void quad(Vertex (&v)[4],float vmax,uint32_t color) {
    v[0]={-.5f,-.5f,.5f,1,color,0,0};
    v[1]={639.5f,-.5f,.5f,1,color,.625f,0};
    v[2]={-.5f,479.5f,.5f,1,color,0,vmax};
    v[3]={639.5f,479.5f,.5f,1,color,.625f,vmax};
}
int main() {
    std::vector<uint8_t> source(1024*512*4),destination(1024*1024*4);
    fill(destination,0x0F000000);
    detail::store32(source.data()+4,0xFFC86432);
    detail::store32(source.data()+(479*1024+639)*4,0xFF123456);
    Vertex v[4]{};quad(v,.9375f,0xFFFFFFFF);
    Image image{source.data(),source.size(),1024,512,4096,21};
    Target target{destination.data(),destination.size(),1024,1024,4096,21};
    Stats stats;const Viewport viewport{0,0,640,480};
    assert(rasterize(v,image,target,viewport,Settings{},stats)==Result::Rendered);
    assert(stats.covered==307200 && stats.written==2 && stats.alpha_rejected==307198);
    assert(stats.first.texel_x==0 && stats.first.texel_y==0 && stats.last.texel_x==639 && stats.last.texel_y==479);
    assert(detail::load32(destination.data()+4)==0xFFC86432);
    assert(detail::load32(destination.data()+(479*1024+639)*4)==0xFF123456);
    assert(detail::load32(destination.data()+640*4)==0x0F000000);
    puts("PASS captured_geometry_sampling_and_alpha_test");
    printf("PIXEL_DIGEST logo source:%08X before:%08X after:%08X\n",stats.hash_source,stats.hash_before,stats.hash_after);

    source.resize(1024*1024*4);fill(source,0x0F000000);fill(destination,0xFF000000);
    detail::store32(source.data()+4,0x80C86432);
    quad(v,.46875f,0xFF0F0F0F);
    image={source.data(),source.size(),1024,1024,4096,21};target.format=22;
    assert(rasterize(v,image,target,viewport,Settings{1,true,true,true},stats)==Result::Rendered);
    assert(stats.covered==307200 && stats.written==307200 && stats.changed==1);
    assert(detail::load32(destination.data()+4)==0xFF0C0603);
    puts("PASS captured_fade_one_zero_x8_destination");
    printf("PIXEL_DIGEST fade source:%08X before:%08X after:%08X\n",stats.hash_source,stats.hash_before,stats.hash_after);

    assert(detail::blend(0x80C86432,0xFF102030)==0xBF6C4231);
    puts("PASS source_alpha_blend_channels");
    const auto before=destination;
    v[3].x=std::numeric_limits<float>::quiet_NaN();
    assert(rasterize(v,image,target,viewport,Settings{},stats)==Result::Unsupported && destination==before);
    quad(v,.46875f,0xFFFFFFFF);image.bytes=destination.data();image.size=destination.size();
    assert(rasterize(v,image,target,viewport,Settings{},stats)==Result::Unsupported && destination==before);
    puts("PASS invalid_geometry_and_alias_preserve_destination");

    std::vector<uint8_t> back(640*480*4);fill(back,0x0F112233);
    std::vector<uint32_t> scanout(960*544,0x87654321);uint32_t hash=0;
    image={back.data(),back.size(),640,480,2560,22};
    assert(d3d8_present::prepare(image,scanout.data(),scanout.size(),hash));
    assert(scanout[0]==0xFF000000 && scanout[116]==0xFF000000);
    assert(scanout[117]==0xFF332211 && scanout[841]==0xFF332211 && scanout[842]==0xFF000000);
    assert(scanout[543*960+117]==0xFF332211 && scanout.back()==0xFF000000);
    puts("PASS present_abgr_aspect_and_black_bars");
    printf("PIXEL_DIGEST scanout after:%08X\n",hash);
    const auto unchanged=scanout;image.size-=4;
    assert(!d3d8_present::prepare(image,scanout.data(),scanout.size(),hash) && scanout==unchanged);
    assert(d3d8_present::full_rect({0,0,640,480},640,480));
    assert(!d3d8_present::full_rect({-1,0,640,480},640,480));
    assert(!d3d8_present::full_rect({0,0,639,480},640,480));
    puts("PASS present_bounds_and_full_client_rect");
}
