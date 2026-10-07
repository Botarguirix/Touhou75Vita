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
#include "pe_resources.h"
#include "th075_assets.h"

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
    text(pixels,40,85,"ITERATION 47 - BOOT STARTUP",0xFFE9C975);
    if(th075::icon_preview.size()==1024) {
        for(unsigned y=0;y<32;++y)for(unsigned x=0;x<32;++x) {
            const uint32_t color=th075::icon_preview[y*32+x];
            if(color>>24)for(unsigned dy=0;dy<2;++dy)for(unsigned dx=0;dx<2;++dx)
                pixels[(48+y*2+dy)*960+850+x*2+dx]=color;
        }
        fprintf(log,"screen_original_exe_icon=decoded_resource_presented\n");
    }
    const uint32_t good=0xFF99D877,bad=0xFF8080FF,neutral=0xFFC2B5AB;
    if(th075::title_preview.size()==640u*480u) {
        text(pixels,700,205,"DAT RESOURCE",neutral,2);
        for(unsigned y=0;y<180;++y)for(unsigned x=0;x<240;++x)
            pixels[(228+y)*960+700+x]=th075::title_preview[(y*480/180)*640+x*640/240];
        fprintf(log,"screen_original_dat_title=decoded_frame_preview_presented\n");
    }
    text(pixels,40,137,result==0?"RESULT: STARTUP CHECKPOINT PASS":"RESULT: FAIL - CHECK LOG",result==0?good:bad);
    const char* labels[]={"SHA256","X86 CPU","IAT BRIDGE","HEAP","FILE READ","TEB FS","TLS","PROCESS"};
    const char* keys[]={"game_sha256_result","dynarec_smoke_result","import_smoke_result",
                        "heap_smoke_result","file_smoke_result","teb_fs_smoke_result",
                        "tls_smoke_result","process_smoke_result"};
    for(unsigned i=0;i<8;++i) {
        std::string status=values.count(keys[i])?values[keys[i]]:"NOT RUN";
        text(pixels,40,174+int(i)*24,std::string(labels[i])+": "+status,
             status=="passed"?good:(status=="failed"?bad:neutral));
    }
    const std::string startup=values.count("startup_result")?values["startup_result"]:"not_attempted";
    const bool reached=startup=="reached_next_import_after_heap";
    const bool game_entry=values.count("startup_game_entry_verified") && values["startup_game_entry_verified"]=="yes";
    const char* entry=reached?(game_entry?"EXE: GAME ENTRY CONFIRMED":"EXE: STARTUP SERVICES RETURNED"):
        (startup=="cpu_fault"?"EXE: CPU FAULT":
        (startup=="budget_exhausted"?"EXE: EXECUTION LIMIT REACHED":
        (startup=="watchdog_unavailable"?"EXE: WATCHDOG NOT READY":
        (startup=="service_contract_failed"?"EXE: SERVICE CONTRACT FAILED":"EXE: CHECK LOG"))));
    text(pixels,40,377,entry,reached?good:bad);
    std::string boundary=values.count("startup_stop_import") ? values["startup_stop_import"]:"SEE LOG";
    if (startup=="budget_exhausted" || startup=="cpu_fault")
        boundary=(startup=="budget_exhausted"?"CPU BUDGET ":"CPU FAULT ")+
            (values.count("startup_final_eip")?values["startup_final_eip"]:"SEE LOG");
    text(pixels,40,408,"STOP: "+boundary,neutral,2);
    text(pixels,40,437,"GAME BOOT: NOT YET VERIFIED",neutral,2);
    std::string displayed_log=log_path;
    for(char& ch:displayed_log)if(ch>='a' && ch<='z')ch=char(ch-'a'+'A');
    text(pixels,40,467,"LOG: "+displayed_log,neutral,2);
    text(pixels,40,500,"PRESS X TO EXIT - AUTO EXIT 120S",0xFFF3EEE8,2);
    SceDisplayFrameBuf fb={}; fb.size=sizeof(fb);fb.base=base;fb.pitch=960;
    fb.pixelformat=SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;fb.width=960;fb.height=544;
    // r1's immediate update was rejected by hardware (0x80290006).
    // Schedule scanout at the next frame boundary instead.
    fprintf(log,"screen_present_sync=nextframe\n");
    int rc=sceDisplaySetFrameBuf(&fb,SCE_DISPLAY_SETBUF_NEXTFRAME);
    fprintf(log,"screen_present_rc=0x%08X\n",(unsigned)rc);
    if(rc>=0) {
        int wait_rc=sceDisplayWaitVblankStart();
        fprintf(log,"screen_vblank_rc=0x%08X\n",(unsigned)wait_rc);
        SceDisplayFrameBuf active={};active.size=sizeof(active);
        int get_rc=sceDisplayGetFrameBuf(&active,SCE_DISPLAY_SETBUF_IMMEDIATE);
        fprintf(log,"screen_query_rc=0x%08X\n",(unsigned)get_rc);
        bool matches=get_rc>=0&&active.base==base&&active.pitch==960&&
            active.width==960&&active.height==544&&active.pixelformat==fb.pixelformat;
        fprintf(log,"screen_active_matches=%s\n",matches?"yes":"no");
        fprintf(log,"screen_result=%s\n",wait_rc>=0&&matches?"presented":"confirmation_failed");
    } else fprintf(log,"screen_result=failed\n");
    if(rc>=0) {
        const uint64_t deadline=sceKernelGetProcessTimeWide()+120000000ull;
        bool released=false;const char* exit_reason="timeout";
        while(sceKernelGetProcessTimeWide()<deadline) {
            SceCtrlData pad={};
            if(sceCtrlPeekBufferPositive(0,&pad,1)<0) { exit_reason="controller_error";break; }
            if(!(pad.buttons&SCE_CTRL_CROSS)) released=true;
            if(released&&(pad.buttons&SCE_CTRL_CROSS)) { exit_reason="cross";break; }
            sceKernelDelayThread(16000);
        }
        fprintf(log,"screen_exit=%s\n",exit_reason);
        // Retain the allocation until process exit if detaching is refused;
        // never free a buffer that may still be scanned out by the display.
        int detach_rc=sceDisplaySetFrameBuf(nullptr,SCE_DISPLAY_SETBUF_NEXTFRAME);
        fprintf(log,"screen_detach_rc=0x%08X\n",(unsigned)detach_rc);
        if(detach_rc<0||sceDisplayWaitVblankStart()<0) {
            fprintf(log,"screen_buffer_release=deferred_to_process_exit\n");return;
        }
    }
    sceKernelFreeMemBlock(block);
}
