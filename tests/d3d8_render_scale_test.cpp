#include "../vita/src/d3d8_render_scale.h"
#include "../vita/src/d3d8_present_frame.h"
#include "../vita/src/d3d8_present_cache.h"
#include "../vita/src/diagnostic_log.h"
#include "../vita/src/performance_clocks.h"
#include <cassert>
#include <cstdio>
#include <chrono>

using namespace d3d8_quad;
static int arm=333,bus=166,arm_calls=0,bus_calls=0,fail_clock=0;
int scePowerGetArmClockFrequency(){return arm;}
int scePowerGetBusClockFrequency(){return bus;}
int scePowerGetGpuClockFrequency(){return 111;}
int scePowerGetGpuXbarClockFrequency(){return 111;}
int scePowerSetArmClockFrequency(int value){++arm_calls;if(fail_clock)return -1;arm=value;return 0;}
int scePowerSetBusClockFrequency(int value){++bus_calls;if(fail_clock)return -1;bus=value;return 0;}
static void quad(Vertex (&v)[4],float w,float h) {
    v[0]={-.5f,-.5f,.5f,1,0xFFFFFFFF,0,0};v[1]={w-.5f,-.5f,.5f,1,0xFFFFFFFF,1,0};
    v[2]={-.5f,h-.5f,.5f,1,0xFFFFFFFF,0,1};v[3]={w-.5f,h-.5f,.5f,1,0xFFFFFFFF,1,1};
}
int main() {
    {
        HalfSurface half;half.initialize(640,480,22);Vertex v[4];quad(v,640,480);half.vertices(v);
        assert(half.size()==320*240*4 && v[0].x==-.5f && v[3].x==319.5f && v[3].y==239.5f);
        const auto view=half.viewport({0,0,640,480});assert(view.width==320 && view.height==240);
        const auto crop=half.viewport({1,3,9,13});assert(crop.x==1 && crop.y==2 && crop.width==4 && crop.height==6);
        HalfSurface odd;odd.initialize(5,3,21);quad(v,5,3);odd.vertices(v);
        assert(odd.size()==3*2*4 && v[3].x==2.5f && v[3].y==1.5f);
        puts("PASS reduced geometry, pixel centers, viewport crop and odd dimensions");
    }
    {
        HalfSurface half;half.initialize(7,5,21);
        constexpr unsigned pitch=36;std::vector<uint8_t> full(pitch*5,0xC5),back(pitch*5,0xD6);
        for(unsigned y=0;y<5;++y)for(unsigned x=0;x<7;++x)detail::store32(full.data()+y*pitch+x*4,0xFF000000+y*16+x);
        assert(half.upload({full.data(),full.size(),7,5,pitch,21}));
        const auto image=half.image();assert(detail::load32(image.bytes)==0xFF000000 && detail::load32(image.bytes+(2*4+3)*4)==0xFF000046);
        assert(half.readback({back.data(),back.size(),7,5,pitch,21}));
        for(unsigned y=0;y<5;++y)for(unsigned x=28;x<pitch;++x)assert(back[y*pitch+x]==0xD6);
        const auto before=back;assert(!half.readback({back.data(),4,7,5,pitch,21}) && back==before);
        assert(!half.upload({full.data(),full.size(),7,5,pitch,22}));
        half.clear({0,0,7,5},0x80ABCDEF);assert(half.readback({back.data(),back.size(),7,5,pitch,21}));
        for(unsigned y=0;y<5;++y)for(unsigned x=0;x<7;++x)assert(detail::load32(back.data()+y*pitch+x*4)==0x80ABCDEF);
        puts("PASS upload/readback, pitched padding, alpha and invalid spans");
    }
    {
        HalfSurface half;half.initialize(640,480,22);
        std::vector<uint8_t> source(16*16*4);for(size_t i=0;i<source.size();i+=4)detail::store32(source.data()+i,0x80112233);
        Image image{source.data(),source.size(),16,16,64,21};Settings settings;settings.hash_pixels=false;
        for(unsigned filter:{1u,2u}) {
            Vertex v[4];quad(v,640,480);half.vertices(v);half.clear({0,0,640,480},0xFF102030);
            settings.filter=filter;Stats stats;
            assert(rasterize(v,image,half.target(),{0,0,320,240},settings,stats)==Result::Rendered);
            assert(stats.covered==76800 && stats.written==76800 && detail::load32(half.image().bytes)==(detail::blend(0x80112233,0xFF102030)|0xFF000000));
        }
        puts("PASS actual raster POINT/LINEAR and source alpha at quarter pixel count");
    }
    {
        HalfSurface half;half.initialize(640,480,22);half.clear({0,0,640,480},0xFF112233);
        auto image=half.image();std::vector<uint32_t> screen(960*544);uint32_t hash=1;
        assert(d3d8_present::prepare(image,screen.data(),screen.size(),hash,false) && hash==0);
        assert(screen[116]==0xFF000000 && screen[117]==0xFF332211 && screen[841]==0xFF332211 && screen[842]==0xFF000000);
        detail::store32(half.target().bytes+(239*320+319)*4,0xFFABCDEF);
        assert(d3d8_present::prepare(image,screen.data(),screen.size(),hash,false));
        assert(screen[543*960+841]==0xFFEFCDAB && screen[542*960+841]==0xFFEFCDAB && screen[841]==0xFF332211);
        d3d8_present::FrameCache cache;unsigned slot=7;
        assert(cache.remember_confirmed(image,1,0) && cache.lookup(image,slot,hash) && slot==1);
        std::vector<uint8_t> full(640*480*4);Image original{full.data(),full.size(),640,480,2560,22};
        assert(!cache.lookup(original,slot,hash));half.clear({0,0,640,480},0xFF445566);
        assert(!cache.lookup(half.image(),slot,hash));
        const auto before=screen;image.size-=1;assert(!d3d8_present::prepare(image,screen.data(),screen.size(),hash,false) && screen==before);
        puts("PASS reduced native scanout, byte cache, metadata separation and invalid inputs");
    }
    {
        FILE* file=std::tmpfile();assert(file);th075::compact_diagnostics=true;th075::suppressed_diagnostics=0;
        assert(th075::diagnostic_log(file,"startup_d3d8_arg0=%d\n",7)==0);
        assert(th075::diagnostic_log(file,"startup_service_error=bad_frame\n")>0);
        assert(th075::diagnostic_log(file,"startup_d3d8_draw_elapsed_us=123\n")>0);
        assert(th075::suppressed_diagnostics==1);th075::compact_diagnostics=false;
        assert(th075::diagnostic_log(file,"startup_d3d8_arg0=%d\n",7)>0);std::fclose(file);
        puts("PASS compact diagnostic policy retains errors/timing and defaults to full traces");
    }
    {
        FILE* file=std::tmpfile();assert(file);
        {PerformanceClocks clocks(file);assert(arm==444 && bus==222);}assert(arm==333 && bus==166 && arm_calls==2 && bus_calls==2);
        arm=500;bus=222;arm_calls=bus_calls=0;
        {PerformanceClocks clocks(file);assert(arm==500 && bus==222);}assert(!arm_calls && !bus_calls);
        arm=333;bus=166;{PerformanceClocks clocks(file);arm=500;}assert(arm==500 && bus==166);
        arm=333;bus=166;fail_clock=1;{PerformanceClocks clocks(file);assert(arm==333 && bus==166);}fail_clock=0;
        arm=-1;bus=-1;arm_calls=bus_calls=0;{PerformanceClocks clocks(file);}assert(!arm_calls && !bus_calls);
        std::fclose(file);puts("PASS clock floor, restore, plugin changes, rejected and unavailable frequencies");
    }
}
