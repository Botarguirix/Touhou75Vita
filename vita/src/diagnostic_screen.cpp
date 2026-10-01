#include "diagnostic_screen.h"
#include <psp2/display.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>

namespace {
// Original 5x7 diagnostic font: rows, bit 4 at the left.
struct Glyph { char c; uint8_t row[7]; };
const Glyph font[] = {
 {'A',{14,17,17,31,17,17,17}}, {'B',{30,17,17,30,17,17,30}},
 {'C',{14,17,16,16,16,17,14}}, {'D',{30,17,17,17,17,17,30}},
 {'E',{31,16,16,30,16,16,31}}, {'F',{31,16,16,30,16,16,16}},
 {'G',{14,17,16,23,17,17,14}}, {'H',{17,17,17,31,17,17,17}},
 {'I',{31,4,4,4,4,4,31}}, {'J',{7,2,2,2,18,18,12}},
 {'K',{17,18,20,24,20,18,17}}, {'L',{16,16,16,16,16,16,31}},
 {'M',{17,27,21,21,17,17,17}}, {'N',{17,25,21,19,17,17,17}},
 {'O',{14,17,17,17,17,17,14}}, {'P',{30,17,17,30,16,16,16}},
 {'Q',{14,17,17,17,21,18,13}}, {'R',{30,17,17,30,20,18,17}},
 {'S',{15,16,16,14,1,1,30}}, {'T',{31,4,4,4,4,4,4}},
 {'U',{17,17,17,17,17,17,14}}, {'V',{17,17,17,17,17,10,4}},
 {'W',{17,17,17,21,21,27,17}}, {'X',{17,17,10,4,10,17,17}},
 {'Y',{17,17,10,4,4,4,4}}, {'Z',{31,1,2,4,8,16,31}},
 {'0',{14,17,19,21,25,17,14}}, {'1',{4,12,4,4,4,4,14}},
 {'2',{14,17,1,2,4,8,31}}, {'3',{30,1,1,14,1,1,30}},
 {'4',{2,6,10,18,31,2,2}}, {'5',{31,16,16,30,1,1,30}},
 {'6',{14,16,16,30,17,17,14}}, {'7',{31,1,2,4,8,8,8}},
 {'8',{14,17,17,14,17,17,14}}, {'9',{14,17,17,15,1,1,14}},
 {':',{0,4,4,0,4,4,0}}, {'.',{0,0,0,0,0,4,4}},
 {'/',{1,2,2,4,8,8,16}}, {'-',{0,0,0,31,0,0,0}}
};
void text(uint32_t* pixels,int x,int y,const std::string& s,uint32_t color,int scale=3) {
    for(char c:s) {
        if(c>='a'&&c<='z') c=char(c-'a'+'A');
        for(const auto& g:font) if(g.c==c) {
            for(int r=0;r<7;++r) for(int col=0;col<5;++col) if(g.row[r]&(16>>col))
                for(int dy=0;dy<scale;++dy) for(int dx=0;dx<scale;++dx) {
                    int px=x+col*scale+dx,py=y+r*scale+dy;
                    if(px>=0&&px<960&&py>=0&&py<544) pixels[py*960+px]=color;
                }
            break;
        }
        x+=6*scale;
    }
}
}

void show_diagnostic_screen(const char* log_path,int result,FILE* log) {
    std::map<std::string,std::string> values;
    FILE* report=fopen(log_path,"rb");
    if(report) {
        char line[512];
        while(fgets(line,sizeof(line),report)) {
            char* equal=strchr(line,'='); if(!equal) continue;
            *equal=0; char* value=equal+1;
            value[strcspn(value,"\r\n")]=0; values[line]=value;
        }
        fclose(report);
    }
    const unsigned bytes=(960u*544u*4u+0x3FFFFu)&~0x3FFFFu;
    SceUID block=sceKernelAllocMemBlock("TH075 diagnostic display",
        SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,bytes,nullptr);
    void* base=nullptr;
    if(block<0||sceKernelGetMemBlockBase(block,&base)<0) {
        fprintf(log,"screen_result=allocation_failed\n");
        if(block>=0) sceKernelFreeMemBlock(block);
        return;
    }
    auto* pixels=static_cast<uint32_t*>(base);
    for(unsigned i=0;i<960u*544u;++i) pixels[i]=0xFF20130D;
    text(pixels,40,38,"TOUHOU 7.5 VITA",0xFFF3EEE8,4);
    text(pixels,40,85,"ITERATION 09 - X86 SERVICES",0xFFE9C975);
    const uint32_t good=0xFF99D877,bad=0xFF8080FF,neutral=0xFFC2B5AB;
    text(pixels,40,137,result==0?"RESULT: PASS":"RESULT: FAIL - CHECK LOG",result==0?good:bad);
    const char* labels[]={"SHA256","X86 CPU","IAT BRIDGE","HEAP","FILE READ"};
    const char* keys[]={"game_sha256_result","dynarec_smoke_result","import_smoke_result",
                        "heap_smoke_result","file_smoke_result"};
    for(unsigned i=0;i<5;++i) {
        std::string status=values.count(keys[i])?values[keys[i]]:"NOT RUN";
        text(pixels,40,184+int(i)*36,std::string(labels[i])+": "+status,
             status=="passed"?good:(status=="failed"?bad:neutral));
    }
    text(pixels,40,385,"GAME STARTUP: NOT ATTEMPTED",neutral);
    text(pixels,40,425,"LOG: UX0:DATA/TH075VITA/ITERATION09.LOG",neutral,2);
    text(pixels,40,477,"PRESS X TO EXIT - AUTO EXIT 120S",0xFFF3EEE8,2);
    SceDisplayFrameBuf fb={}; fb.size=sizeof(fb);fb.base=base;fb.pitch=960;
    fb.pixelformat=SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;fb.width=960;fb.height=544;
    int rc=sceDisplaySetFrameBuf(&fb,SCE_DISPLAY_SETBUF_IMMEDIATE);
    fprintf(log,"screen_present_rc=0x%08X\n",(unsigned)rc);
    fprintf(log,"screen_result=%s\n",rc<0?"failed":"presented");
    if(rc>=0) {
        const uint64_t deadline=sceKernelGetProcessTimeWide()+120000000ull;
        bool released=false;
        while(sceKernelGetProcessTimeWide()<deadline) {
            SceCtrlData pad={};
            if(sceCtrlPeekBufferPositive(0,&pad,1)<0) { fprintf(log,"screen_exit=controller_error\n");break; }
            if(!(pad.buttons&SCE_CTRL_CROSS)) released=true;
            if(released&&(pad.buttons&SCE_CTRL_CROSS)) { fprintf(log,"screen_exit=cross\n");break; }
            sceKernelDelayThread(16000);
        }
        // Detach display before freeing the buffer it scans out.
        sceDisplaySetFrameBuf(nullptr,SCE_DISPLAY_SETBUF_IMMEDIATE);
        sceDisplayWaitVblankStart();
    }
    sceKernelFreeMemBlock(block);
}
