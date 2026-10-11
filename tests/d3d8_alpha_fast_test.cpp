// Standalone pixel oracle and serial performance-path equivalence tests.
// A previous committed header can also be supplied for a differential replay.
#ifdef TH075_RASTER_REFERENCE
#include TH075_RASTER_REFERENCE
#else
#include "../vita/src/d3d8_raster_executor.h"
#endif
#include <cassert>
#include <cstdio>
#include <cstring>
#include <limits>
#include <vector>

using namespace d3d8_quad;
static uint32_t scalar_modulate(uint32_t a,uint32_t b) {
    uint32_t out=0;for(unsigned shift=0;shift<32;shift+=8)
        out|=((((a>>shift)&255)*((b>>shift)&255)+127)/255)<<shift;
    return out;
}
static uint32_t scalar_blend(uint32_t a,uint32_t b) {
    uint32_t out=0,alpha=a>>24;for(unsigned shift=0;shift<32;shift+=8)
        out|=((((a>>shift)&255)*alpha+((b>>shift)&255)*(255-alpha)+127)/255)<<shift;
    return out;
}
static Result fast(const Vertex (&v)[4],const Image& image,const Target& target,
    const Viewport& viewport,const Settings& requested,Stats& stats) {
#ifdef TH075_RASTER_REFERENCE
    Settings settings=requested;settings.hash_pixels=false;
    return rasterize(v,image,target,viewport,settings,stats);
#else
    return execute_serial_fast(nullptr,v,image,target,viewport,requested,stats);
#endif
}
static void same_counts(const Stats& a,const Stats& b) {
    assert(a.reject==b.reject && a.covered==b.covered && a.written==b.written &&
        a.changed==b.changed && a.alpha_rejected==b.alpha_rejected &&
        a.triangle_strip==b.triangle_strip && a.linear_exact==b.linear_exact &&
        a.linear_constant==b.linear_constant && a.linear_reference==b.linear_reference);
}
int main() {
    uint32_t state=0x9233AFA1;
    auto random=[&](){state^=state<<13;state^=state>>17;state^=state<<5;return state;};
    for(uint32_t x=0;x<=65152;++x) {
#ifndef TH075_RASTER_REFERENCE
        assert(detail::divide255(x)==x/255);
        for(uint32_t y:{0u,127u,65025u,65152u,x})
            assert(detail::divide255_pair(x|(y<<16))==((x/255)|((y/255)<<16)));
#endif
        assert(detail::modulate(x*65793u,0x80FFFFFF)==scalar_modulate(x*65793u,0x80FFFFFF));
    }
    puts("PASS exact bounded division and independent packed channel lanes");
    for(unsigned n=0;n<400000;++n) {
        const uint32_t a=random(),b=random();
        assert(detail::modulate(a,b)==scalar_modulate(a,b));
        assert(detail::blend(a,b)==scalar_blend(a,b));
    }
    for(uint32_t alpha=0;alpha<256;++alpha)for(uint32_t value=0;value<256;++value) {
        const uint32_t a=(alpha<<24)|(value*65793u);
        for(uint32_t b:{0u,0xFFFFFFFFu,0x00FF00FFu,0xFF00FF00u})
            assert(detail::blend(a,b)==scalar_blend(a,b));
    }
    puts("PASS modulation and packed SRCALPHA blending match scalar oracle including alpha");

    uint32_t digest=2166136261u;uint64_t shortcuts=0,covered=0;
    // Padding, WRAP seams, clipping, diffuse alpha, all blend modes and both
    // accepted source/target formats. Original targets include undefined X.
    for(unsigned n=0;n<384;++n) {
        const unsigned sw=1+random()%17,sh=1+random()%13;
        const uint32_t sf=n&1?25:21,tf=n&2?22:21;
        const unsigned bpp=sf==25?2:4,sp=sw*bpp+8;
        std::vector<uint8_t> texture(sp*sh+32),out(148*27+64),reference;
        for(auto& byte:texture)byte=uint8_t(random());
        for(unsigned y=0;y<sh;++y)for(unsigned x=0;x<sw;++x)
            if((x+y+n)%4)texture[y*sp+x*bpp+bpp-1]&=sf==21?7:127;
        for(auto& byte:out)byte=uint8_t(random());
        reference=out;
        Image source{texture.data(),texture.size(),sw,sh,sp,sf};
        Target target{out.data(),out.size(),31,27,148,tf},expected=target;
        expected.bytes=reference.data();const Viewport vp{2,1,27,24};
        const uint32_t color=n%3?random():0xFFFFFFFFu;
        Vertex v[4]={{-.75f,-1.25f,.3f,1,color,-.13f,.11f},
            {30.5f,-1.25f,.3f,1,color,1.19f,.11f},
            {-.75f,26.75f,.3f,1,color,-.13f,1.13f},
            {30.5f,26.75f,.3f,1,color,1.19f,1.13f}};
        if(n&4){v[0].x=-3.25f;v[1].y=2.75f;v[2].x=4.75f;v[3].y=24.25f;v[1].v=.02f;}
        Settings settings;settings.filter=n&8?2:1;
        settings.alpha_ref=n%5?random()%256:1;
        settings.alpha_test=n%7!=0;settings.alpha_blend=n%11!=0;settings.replace_blend=n%13==0;
        Stats a,b;
        assert(rasterize(v,source,expected,vp,settings,a)==Result::Rendered);
        assert(fast(v,source,target,vp,settings,b)==Result::Rendered);
        assert(out==reference && !b.hashes_valid);same_counts(a,b);
#ifndef TH075_RASTER_REFERENCE
        const Stats empty{};
        assert(!b.probes_valid && std::memcmp(&b.first,&empty.first,sizeof(Probe))==0);
        assert(b.alpha_shortcuts<=b.alpha_rejected);shortcuts+=b.alpha_shortcuts;
#endif
        covered+=b.covered;
        for(uint8_t byte:out)digest=(digest^byte)*16777619u;
    }
#ifndef TH075_RASTER_REFERENCE
    assert(shortcuts>10000);
#endif
    (void)shortcuts;(void)covered;
    puts("PASS 384 rectangle/convex replays preserve pixels counts padding wrap formats filters and blend modes");
    printf("PIXEL_DIGEST alpha replay:%08X\n",digest);

    const uint32_t colors[]={0x00123456,0x00010203,0x00040506,0x00070809};
    const Image image{reinterpret_cast<const uint8_t*>(colors),sizeof(colors),2,2,8,21};
    uint32_t pixel=0x01ABCDEF;
    const Target target{reinterpret_cast<uint8_t*>(&pixel),4,1,1,4,22};
    Vertex v[4]={{0,0,.5f,1,0xFFFFFFFF,.3f,.4f},{1,0,.5f,1,0xFFFFFFFF,.3f,.4f},
        {0,1,.5f,1,0xFFFFFFFF,.3f,.4f},{1,1,.5f,1,0xFFFFFFFF,.3f,.4f}};
    Settings settings;settings.filter=2;Stats stats;
    assert(fast(v,image,target,{0,0,1,1},settings,stats)==Result::Rendered);
    assert(pixel==0x01ABCDEF && stats.alpha_rejected==1 && stats.written==0);
#ifndef TH075_RASTER_REFERENCE
    assert(stats.alpha_shortcuts==1);
    settings.hash_pixels=false;settings.alpha_shortcuts=true;
    assert(rasterize(v,image,target,{0,0,1,1},settings,stats)==Result::Rendered);
    assert(stats.probes_valid && stats.alpha_shortcuts==0 && stats.first.source!=0);
    settings.hash_pixels=true;settings.record_probes=false;
    assert(rasterize(v,image,target,{0,0,1,1},settings,stats)==Result::Rendered);
    assert(stats.hashes_valid && stats.alpha_shortcuts==0 && stats.hash_before==stats.hash_after);
#endif
    puts("PASS transparent colored texels keep destination X and diagnostics retain complete samples");
    settings.alpha_test=false;
    assert(fast(v,image,target,{0,0,1,1},settings,stats)==Result::Rendered && stats.written==1);
    settings.alpha_test=true;settings.alpha_ref=0;
    assert(fast(v,image,target,{0,0,1,1},settings,stats)==Result::Rendered && stats.written==1);
    puts("PASS disabled alpha test and zero reference never trigger early rejection");
    const uint32_t before=pixel;settings.alpha_ref=256;
    assert(fast(v,image,target,{0,0,1,1},settings,stats)==Result::Unsupported && pixel==before);
    settings.alpha_ref=1;v[0].u=std::numeric_limits<float>::quiet_NaN();
    assert(fast(v,image,target,{0,0,1,1},settings,stats)==Result::Unsupported && pixel==before);
    v[0].u=.3f;Target alias=target;alias.bytes=const_cast<uint8_t*>(image.bytes);alias.size=image.size;
    assert(fast(v,image,alias,{0,0,1,1},settings,stats)==Result::Unsupported);
    puts("PASS invalid state geometry and alias rejected before destination writes");
}
