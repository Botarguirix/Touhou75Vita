#include "../vita/src/exe_draw_preview.h"
#include <cassert>
#include <vector>

std::array<uint32_t,260*180> reference(const uint8_t* data,unsigned pitch,unsigned x0,unsigned y0,unsigned width,unsigned height) {
    std::array<uint32_t,260*180> out{};out.fill(0xFF20130D);
    unsigned dw=260,dh=uint64_t(height)*260/width;
    if(dh>180){dh=180;dw=uint64_t(width)*180/height;}
    dw=std::max(1u,dw);dh=std::max(1u,dh);
    for(unsigned y=0;y<dh;++y)for(unsigned x=0;x<dw;++x) {
        unsigned sx=x0+uint64_t(x)*width/dw,sy=y0+uint64_t(y)*height/dh;
        uint32_t argb;std::memcpy(&argb,data+size_t(sy)*pitch+sx*4,4);
        out[((180-dh)/2+y)*260+(260-dw)/2+x]=0xFF000000u|((argb&255)<<16)|(argb&0xFF00)|((argb>>16)&255);
    }
    return out;
}
int main() {
    FILE* log=std::tmpfile();assert(log);
    for(auto shape:{std::array<unsigned,2>{640,480},{320,240},{1,1},{17,31},{31,17},{260,180},{1024,2},{2,1024}}) {
        const auto w=shape[0],h=shape[1],pitch=w*4+12;std::vector<uint8_t> bytes(size_t(pitch)*h);
        uint32_t random=123;for(auto& b:bytes){random=random*1664525u+1013904223u;b=random>>24;}
        for(unsigned inset=0;inset<std::min(4u,std::min(w,h));++inset) {
            const auto expected=reference(bytes.data(),pitch,inset,inset,w-inset,h-inset);
            th075::capture_first_exe_quad(bytes.data(),bytes.size(),w,h,pitch,inset,inset,w-inset,h-inset,log,true);
            assert(expected==th075::exe_draw_preview && th075::exe_draw_ready);
        }
    }
    puts("PASS preview matches original scalar pixel oracle across full half odd and tiny targets");
    std::vector<uint8_t> pixels(320*240*4,0x51);auto saved=th075::exe_draw_preview;
    th075::capture_first_exe_quad(nullptr,0,320,240,1280,0,0,320,240,log,true);
    th075::capture_first_exe_quad(pixels.data(),pixels.size(),320,240,1,0,0,320,240,log,true);
    th075::capture_first_exe_quad(pixels.data(),pixels.size()-1,320,240,1280,0,0,320,240,log,true);
    th075::capture_first_exe_quad(pixels.data(),pixels.size(),320,240,1280,1,0,320,240,log,true);
    assert(th075::exe_draw_preview==saved);
    puts("PASS invalid preview spans pitch and viewport preserve the previous capture");
    th075::capture_presented_exe_frame(pixels.data(),pixels.size(),320,240,1280,1799,log);
    assert(th075::exe_present_frames==1799 && th075::exe_draw_preview==reference(pixels.data(),1280,0,0,320,240));
    saved=th075::exe_draw_preview;
    th075::capture_presented_exe_frame(pixels.data(),pixels.size(),320,240,1280,1800,log,true);
    assert(th075::exe_present_frames==1800 && saved==th075::exe_draw_preview);
    puts("PASS confirmed identical preview advances extended session Present count without regeneration");
    std::vector<uint32_t> screen(960*544,0);th075::draw_exe_snapshot(screen.data());
    for(unsigned y=0;y<180;++y)assert(!std::memcmp(screen.data()+(228+y)*960+690,saved.data()+y*260,260*4));
    std::fclose(log);puts("PASS diagnostic thumbnail placement and opaque color conversion unchanged");
}
