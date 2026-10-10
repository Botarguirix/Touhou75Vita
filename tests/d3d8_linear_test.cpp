#include "../vita/src/d3d8_quad_raster.h"
#include <cassert>
#include <cstdio>
#include <vector>

using namespace d3d8_quad;
static void rect(Vertex (&v)[4],float left,float top,float right,float bottom,
    float u0=0,float v0=0,float u1=1,float v1=1,uint32_t color=0xFFFFFFFFu) {
    v[0]={left,top,.5f,1,color,u0,v0};v[1]={right,top,.5f,1,color,u1,v0};
    v[2]={left,bottom,.5f,1,color,u0,v1};v[3]={right,bottom,.5f,1,color,u1,v1};
}
static uint32_t at(const std::vector<uint8_t>& b,unsigned w,unsigned x,unsigned y) {
    return detail::load32(b.data()+(size_t(y)*w+x)*4);
}
static uint32_t one(const Image& image,float u,float v,Settings settings={0,false,false,false,2}) {
    uint32_t pixel=0;Vertex q[4];rect(q,0,0,1,1,u,v,u,v);
    Target t{reinterpret_cast<uint8_t*>(&pixel),4,1,1,4,21};Stats stats;
    assert(rasterize(q,image,t,{0,0,1,1},settings,stats)==Result::Rendered);
    assert(stats.covered==1 && stats.written==1);return pixel;
}
int main() {
    const uint32_t colors[]={0xFFFF0000,0xFF00FF00,0xFF0000FF,0xFFFFFFFF};
    const Image tiny{reinterpret_cast<const uint8_t*>(colors),sizeof(colors),2,2,8,21};
    for(unsigned y=0;y<2;++y)for(unsigned x=0;x<2;++x)
        assert(one(tiny,float(x*.5+.25),float(y*.5+.25))==colors[y*2+x]);
    puts("PASS LINEAR exact texel centers and constant UV geometry");

    assert(one(tiny,.5f,.5f)==0xFF808080);
    assert(one(tiny,.5f,.25f)==0xFF808000);
    assert(one(tiny,.375f,.375f)==0xFF9F4040);
    puts("PASS LINEAR four-neighbor weights and final channel rounding");

    for(float u:{-2.f,-1.f,0.f,1.f,2.f})assert(one(tiny,u,0)==0xFF808080);
    assert(one(tiny,-.125f,.25f)==0xFF40BF00);
    assert(one(tiny,65536.f,-65536.f)==0xFF808080);
    puts("PASS WRAP each bilinear neighbor at positive and negative seams");

    const uint32_t transparent[]={0x00FF0000,0xFF0000FF};
    const Image alpha{reinterpret_cast<const uint8_t*>(transparent),8,2,1,8,21};
    // Straight alpha interpolation, diffuse modulation, then alpha test and blend.
    assert(one(alpha,.5f,.5f)==0x80800080);
    assert(one(alpha,.5f,.5f,Settings{0,false,false,false,2})==0x80800080);
    uint32_t pixel=0xFF102030;Vertex q[4];rect(q,0,0,1,1,.5f,.5f,.5f,.5f,0x80FFFFFF);
    Target out{reinterpret_cast<uint8_t*>(&pixel),4,1,1,4,21};Stats stats;
    assert(rasterize(q,alpha,out,{0,0,1,1},Settings{65,true,true,false,2},stats)==Result::Rendered);
    assert(stats.alpha_rejected==1 && pixel==0xFF102030 && stats.hash_before==stats.hash_after);
    assert(rasterize(q,alpha,out,{0,0,1,1},Settings{64,true,true,false,2},stats)==Result::Rendered);
    assert(stats.written==1 && stats.first.source==0x40800080 && pixel==0xCF2C1844);
    puts("PASS interpolated alpha precedes modulation GREATEREQUAL test and blend");

    const uint16_t words[]={0xFC00,0x83E0,0x801F,0xFFFF};
    const Image rgb555{reinterpret_cast<const uint8_t*>(words),sizeof(words),2,2,4,25};
    assert(one(rgb555,.5f,.5f)==0xFF808080 && one(rgb555,.25f,.25f)==0xFFFF0000);
    const uint32_t constant=0x87654321;
    const Image singleton{reinterpret_cast<const uint8_t*>(&constant),4,1,1,4,21};
    assert(one(singleton,-.123f,12.75f)==constant);
    puts("PASS A1R5G5B5 normalization and one-texel degenerate wrap");

    // Latest physical81 quad geometry, synthetic gradient. Reference below
    // independently computes coordinates and four weighted channels per pixel.
    std::vector<uint8_t> source(512*512*4),target(640*480*4,0x55);
    for(unsigned y=0;y<512;++y)for(unsigned x=0;x<512;++x)
        detail::store32(source.data()+(y*512+x)*4,0xFF000000u|((x&255)<<16)|((y&255)<<8)|((x+y)&255));
    const Image image{source.data(),source.size(),512,512,2048,21};
    out={target.data(),target.size(),640,480,2560,22};
    rect(q,-128.495f,-116.5f,383.505f,395.5f);
    assert(rasterize(q,image,out,{0,0,640,480},Settings{1,true,true,false,2},stats)==Result::Rendered);
    assert(stats.covered==152064 && stats.written==152064 && stats.alpha_rejected==0);
    for(unsigned y=0;y<480;++y)for(unsigned x=0;x<640;++x) {
        if(x>=384 || y>=396){assert(at(target,640,x,y)==0x55555555);continue;}
        const double s=(double(x)-q[0].x)*512/(double(q[1].x)-q[0].x)-.5,
            t=(double(y)-q[0].y)*512/(double(q[2].y)-q[0].y)-.5;
        const int sx=int(std::floor(s)),sy=int(std::floor(t));const double fx=s-sx,fy=t-sy;
        uint32_t expected=0;
        for(unsigned shift=0;shift<32;shift+=8) {
            double channel=0;
            for(unsigned j=0;j<2;++j)for(unsigned i=0;i<2;++i) {
                const unsigned xx=unsigned((sx+int(i)+512)%512),yy=unsigned((sy+int(j)+512)%512);
                channel+=((at(source,512,xx,yy)>>shift)&255)*(i?fx:1-fx)*(j?fy:1-fy);
            }
            expected|=uint32_t(std::floor(channel+.5))<<shift;
        }
        assert(at(target,640,x,y)==expected);
    }
    puts("PASS fractional clipped physical81 geometry against independent pixel oracle");
    printf("PIXEL_DIGEST linear source:%08X before:%08X after:%08X\n",stats.hash_source,stats.hash_before,stats.hash_after);

    // Point/linear single-level MIP cases accepted; unsupported settings never write.
    for(uint32_t filter:{1u,2u})for(uint32_t mip:{0u,1u,2u})
        assert(single_level_filter(filter,filter,mip));
    for(uint32_t f:{0u,3u,6u,7u,8u,0xFFFFFFFFu})assert(!single_level_filter(f,f,0));
    assert(!single_level_filter(1,2,1) && !single_level_filter(2,1,2) && !single_level_filter(2,2,3));
    const auto before=target;
    assert(rasterize(q,image,out,{0,0,640,480},Settings{1,true,true,false,3},stats)==Result::Unsupported);
    assert(stats.reject==Reject::Settings && target==before);
    out.bytes=source.data();out.size=source.size();out.width=512;out.height=512;out.pitch=2048;
    const auto source_before=source;
    assert(rasterize(q,image,out,{0,0,512,512},Settings{1,true,true,false,2},stats)==Result::Unsupported);
    assert(stats.reject==Reject::Alias && source==source_before);
    puts("PASS bounded filter profile and alias rejection preserve destination");

    // Pitch padding and a subviewport are untouched by LINEAR, including X8 alpha.
    std::vector<uint8_t> padded(4*32,0xA5);
    out={padded.data(),padded.size(),4,4,32,22};rect(q,-.5f,-.5f,3.5f,3.5f);
    assert(rasterize(q,tiny,out,{1,1,2,2},Settings{0,false,false,false,2},stats)==Result::Rendered);
    assert(stats.covered==4);
    for(unsigned y=0;y<4;++y)for(unsigned x=0;x<8;++x) {
        const auto value=detail::load32(padded.data()+y*32+x*4);
        if(y>=1 && y<3 && x>=1 && x<3)assert(value>>24==255);
        else assert(value==0xA5A5A5A5);
    }
    puts("PASS LINEAR padded pitch subviewport and X8 normalization");
}
