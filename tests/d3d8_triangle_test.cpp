#include "../vita/src/d3d8_quad_raster.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace d3d8_quad;

// Independent long-double edge/barycentric oracle. No hardware EXE is run.
static long double cross(const Vertex& a,const Vertex& b,long double x,long double y) {
    return (static_cast<long double>(b.x)-a.x)*(y-a.y)-(static_cast<long double>(b.y)-a.y)*(x-a.x);
}
static bool oracle(const Vertex (&q)[4],unsigned which,unsigned x,unsigned y,
    long double& u,long double& v) {
    unsigned a=which?2:0,b=1,c=which?3:2;
    if(cross(q[a],q[b],q[c].x,q[c].y)<0)std::swap(b,c);
    const unsigned aa[]={a,b,c},bb[]={b,c,a};
    for(unsigned i=0;i<3;++i) {
        const auto& from=q[aa[i]];const auto& to=q[bb[i]];
        const long double e=cross(from,to,x,y),dy=static_cast<long double>(to.y)-from.y,
            dx=static_cast<long double>(to.x)-from.x;
        if(e<0 || (e==0 && !(dy<0 || (dy==0 && dx>0))))return false;
    }
    const long double area=cross(q[a],q[b],q[c].x,q[c].y),
        wb=cross(q[c],q[a],x,y)/area,wc=cross(q[a],q[b],x,y)/area;
    u=q[a].u+(static_cast<long double>(q[b].u)-q[a].u)*wb+(static_cast<long double>(q[c].u)-q[a].u)*wc;
    v=q[a].v+(static_cast<long double>(q[b].v)-q[a].v)*wb+(static_cast<long double>(q[c].v)-q[a].v)*wc;
    return true;
}
static uint32_t oracle_sample(const Image& image,long double u,long double v,unsigned filter) {
    u=(u-std::floor(u))*image.width;v=(v-std::floor(v))*image.height;
    auto texel=[&](int x,int y) {
        x=(x+int(image.width))%int(image.width);y=(y+int(image.height))%int(image.height);
        return detail::sample(image.bytes+y*image.pitch+x*(image.format==25?2:4),image.format);
    };
    if(filter==1)return texel(int(std::floor(u)),int(std::floor(v)));
    u-=.5L;v-=.5L;const int x=int(std::floor(u)),y=int(std::floor(v));
    const long double fx=u-x,fy=v-y;uint32_t color=0;
    for(unsigned shift=0;shift<32;shift+=8) {
        long double channel=0;
        for(unsigned j=0;j<2;++j)for(unsigned i=0;i<2;++i)
            channel+=((texel(x+int(i),y+int(j))>>shift)&255)*(i?fx:1-fx)*(j?fy:1-fy);
        color|=uint32_t(std::floor(channel+.5L))<<shift;
    }
    return color;
}
static void close_color(uint32_t a,uint32_t b) {
    for(unsigned shift=0;shift<32;shift+=8)
        assert(std::abs(int((a>>shift)&255)-int((b>>shift)&255))<=1);
}
static void check_oracle(const Vertex (&q)[4],const Image& source,
    const Target& target,const Viewport& view,const Settings& settings,uint32_t before,const Stats& stats) {
    unsigned count=0;
    for(unsigned y=0;y<target.height;++y)for(unsigned x=0;x<target.width;++x) {
        long double u=0,v=0,u2=0,v2=0;
        const bool one=oracle(q,0,x,y,u,v),two=oracle(q,1,x,y,u2,v2);
        assert(!(one && two));
        const bool inside=(one || two) && x>=view.x && x<view.x+view.width && y>=view.y && y<view.y+view.height;
        uint32_t expected=before;
        if(inside) {
            ++count;if(two){u=u2;v=v2;}
            const uint32_t color=detail::modulate(oracle_sample(source,u,v,settings.filter),q[0].diffuse);
            if(!settings.alpha_test || (color>>24)>=settings.alpha_ref) {
                expected=settings.alpha_blend && !settings.replace_blend?detail::blend(color,before):color;
                if(target.format==22)expected|=0xFF000000;
            }
        }
        const uint32_t actual=detail::load32(target.bytes+y*target.pitch+x*4);
        if(!inside)assert(actual==before);else close_color(actual,expected);
    }
    assert(stats.covered==count);
}
static void clear(std::vector<uint8_t>& bytes,uint32_t color) {
    for(size_t i=0;i<bytes.size();i+=4)detail::store32(bytes.data()+i,color);
}
static void quad(Vertex (&q)[4]) {
    q[0]={1,1,.5f,1,0xFFFFFFFF,0,0};q[1]={9,3,.5f,1,0xFFFFFFFF,1,0};
    q[2]={-1,9,.5f,1,0xFFFFFFFF,0,1};q[3]={7,11,.5f,1,0xFFFFFFFF,1,1};
}
int main() {
    uint32_t pixel=0x80C04020;const Image solid{reinterpret_cast<uint8_t*>(&pixel),4,1,1,4,21};
    std::vector<uint8_t> bytes(16*16*4);Target target{bytes.data(),bytes.size(),16,16,64,21};
    const Viewport view{0,0,16,16};Stats stats;Vertex q[4];quad(q);const Settings point{1,true,true,false,1};
    clear(bytes,0xFF102030);
    assert(rasterize(q,solid,target,view,point,stats)==Result::Rendered && stats.triangle_strip);
    check_oracle(q,solid,target,view,point,0xFF102030,stats);
    assert(stats.covered==stats.written && stats.covered>50);
    puts("PASS rotated strip top-left coverage and single alpha blend per pixel");

    // Force the triangle path on an exact integer rectangle with nonseparable
    // UVs; constant source checks all edge/diagonal pixels without UV effects.
    q[0]={0,0,.5f,1,0xFFFFFFFF,0,0};q[1]={8,0,.5f,1,0xFFFFFFFF,1,0};
    q[2]={0,8,.5f,1,0xFFFFFFFF,.25f,1};q[3]={8,8,.5f,1,0xFFFFFFFF,1,1};
    clear(bytes,0xFF102030);assert(rasterize(q,solid,target,view,point,stats)==Result::Rendered);
    assert(stats.triangle_strip && stats.covered==64);
    check_oracle(q,solid,target,view,point,0xFF102030,stats);
    const auto a=detail::triangle(q,0,1,2),b=detail::triangle(q,2,1,3);
    for(unsigned y=0;y<8;++y)for(unsigned x=0;x<8;++x) {
        double u=0,v=0;const unsigned owners=unsigned(a.uv(x,y,u,v))+unsigned(b.uv(x,y,u,v));
        assert(owners==1);
    }
    puts("PASS integer rectangle diagonal owned once with nonseparable UVs");
    const auto baseline=bytes;std::swap(q[0],q[1]);std::swap(q[2],q[3]);
    clear(bytes,0xFF102030);assert(rasterize(q,solid,target,view,point,stats)==Result::Rendered);
    assert(bytes==baseline);check_oracle(q,solid,target,view,point,0xFF102030,stats);
    puts("PASS reversed strip winding with culling disabled");

    std::vector<uint8_t> gradient(8*8*4);
    for(unsigned y=0;y<8;++y)for(unsigned x=0;x<8;++x)
        detail::store32(gradient.data()+(y*8+x)*4,0xFF000000|(x*29<<16)|(y*31<<8)|((x+y)*17));
    const Image source{gradient.data(),gradient.size(),8,8,32,21};
    quad(q);q[0].u=-.125f;q[3].u=.875f;
    for(auto& vertex:q)vertex.diffuse=0xFFB0D0F0;
    const Settings linear{1,true,true,false,2};clear(bytes,0xFF102030);
    assert(rasterize(q,source,target,view,linear,stats)==Result::Rendered);
    check_oracle(q,source,target,view,linear,0xFF102030,stats);
    assert(stats.linear_exact+stats.linear_constant+stats.linear_reference==stats.covered);
    printf("PIXEL_DIGEST triangles source:%08X before:%08X after:%08X\n",stats.hash_source,stats.hash_before,stats.hash_after);
    puts("PASS affine LINEAR WRAP modulation against independent long-double oracle within one channel unit");

    uint32_t random=0xA8123483u;
    auto next=[&](){random^=random<<13;random^=random>>17;random^=random<<5;return random;};
    for(unsigned n=0;n<500;++n) {
        const float ox=float(int(next()%17)-5)+.25f,oy=float(int(next()%17)-5)+.25f;
        const float bx=float(3+next()%9),by=float(next()%7),cx=-float(next()%7),cy=float(3+next()%9);
        q[0]={ox,oy,.5f,1,0xFFFFFFFF,0,0};q[1]={ox+bx,oy+by,.5f,1,0xFFFFFFFF,0,0};
        q[2]={ox+cx,oy+cy,.5f,1,0xFFFFFFFF,0,0};q[3]={ox+bx+cx,oy+by+cy,.5f,1,0xFFFFFFFF,0,0};
        for(auto& vertex:q){vertex.u=float(int(next()%129)-64)/32;vertex.v=float(int(next()%129)-64)/32;}
        if(n&1){std::swap(q[0],q[1]);std::swap(q[2],q[3]);}
        clear(bytes,0xFF102030);assert(rasterize(q,source,target,view,linear,stats)==Result::Rendered);
        check_oracle(q,source,target,view,linear,0xFF102030,stats);
    }
    puts("PASS 500 deterministic affine strips both windings UV gradients clipping and edge ownership against independent oracle");

    quad(q);for(auto& vertex:q)vertex.diffuse=0x80FFFFFF;
    clear(bytes,0xFF102030);assert(rasterize(q,solid,target,view,Settings{65,true,true,false,2},stats)==Result::Rendered);
    assert(stats.alpha_rejected==stats.covered && stats.written==0 && stats.hash_after==stats.hash_before);
    clear(bytes,0xFF102030);assert(rasterize(q,solid,target,view,Settings{64,true,true,false,2},stats)==Result::Rendered);
    assert(stats.written==stats.covered);check_oracle(q,solid,target,view,Settings{64,true,true,false,2},0xFF102030,stats);
    puts("PASS triangle interpolated alpha test precedes blending and rejected pixels remain intact");

    std::vector<uint8_t> padded(16*80);target={padded.data(),padded.size(),16,16,80,22};
    quad(q);clear(padded,0xA5A5A5A5);const Viewport sub{2,3,5,4};
    assert(rasterize(q,source,target,sub,linear,stats)==Result::Rendered);
    check_oracle(q,source,target,sub,linear,0xA5A5A5A5,stats);
    for(unsigned y=0;y<16;++y)for(unsigned x=16;x<20;++x)assert(detail::load32(padded.data()+y*80+x*4)==0xA5A5A5A5);
    puts("PASS triangle viewport clipping padded storage and X8 alpha normalization");

    const auto unchanged=padded;
    auto reject=[&]() {assert(rasterize(q,source,target,sub,linear,stats)==Result::Unsupported && padded==unchanged);};
    quad(q);q[3]=q[0];reject();quad(q);q[3].x=1;q[3].y=3;reject();
    quad(q);std::swap(q[2],q[3]);reject();quad(q);q[0].rhw=.5f;reject();
    quad(q);q[0].diffuse=0;reject();quad(q);q[3].u=std::numeric_limits<float>::infinity();reject();
    puts("PASS concave folded degenerate perspective varying diffuse and nonfinite strips reject before writing");

    quad(q);for(auto& vertex:q){vertex.x+=1000;vertex.y+=1000;}
    assert(rasterize(q,source,target,sub,linear,stats)==Result::Rendered && stats.covered==0 && padded==unchanged);
    quad(q);const Image alias{target.bytes,target.size,target.width,target.height,target.pitch,21};
    assert(rasterize(q,alias,target,sub,linear,stats)==Result::Unsupported && stats.reject==Reject::Alias && padded==unchanged);
    puts("PASS fully clipped strips and alias protection preserve all bytes");

    // Rounded %g coordinates from physical83 request 696. Raw float bits were
    // not logged then, so this is representative geometry, not an exact replay.
    q[0]={-.504439f,142.673f,.5f,1,0xFFFFFFFF,0,0};q[1]={639.503f,142.675f,.5f,1,0xFFFFFFFF,.625f,0};
    q[2]={-.505992f,654.679f,.5f,1,0xFFFFFFFF,0,1};q[3]={639.501f,654.681f,.5f,1,0xFFFFFFFF,.625f,1};
    bytes.resize(640*480*4);clear(bytes,0xFF010101);target={bytes.data(),bytes.size(),640,480,2560,22};
    assert(rasterize(q,source,target,{0,0,640,480},linear,stats)==Result::Rendered && stats.triangle_strip);
    assert(stats.covered==215680 && stats.written==215680);
    check_oracle(q,source,target,{0,0,640,480},linear,0xFF010101,stats);
    printf("PIXEL_DIGEST physical83_rounded source:%08X before:%08X after:%08X\n",stats.hash_source,stats.hash_before,stats.hash_after);
    puts("PASS representative physical83 skewed geometry retained without snapping");
}
