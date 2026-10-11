#pragma once
#include "d3d8_present_frame.h"
#include "d3d8_present_cache.h"
#include <psp2/display.h>
#include <psp2/kernel/sysmem.h>
#include <cstdio>

// Two owned CDRAM buffers: never rewrite the buffer currently scanned out.
// Each successful Present waits for actual scanout and checks the active FB.
class ExePresenter {
public:
    explicit ExePresenter(FILE* log):log_(log){}
    ~ExePresenter(){release();}
    ExePresenter(const ExePresenter&)=delete;
    ExePresenter& operator=(const ExePresenter&)=delete;
    void set_hash_diagnostics(bool enabled){hash_scanout_=enabled;cache_.invalidate();}
    bool present(const d3d8_quad::Image& source) {
        reused_source_=false;
        if(!allocate())return false;
        unsigned index=next_;
        uint32_t hash=0;
        const bool reused=cache_.lookup(source,index,hash);
        // On a hit the confirmed scanout is reused without any writes. A miss
        // always prepares the other owned buffer, leaving active scanout intact.
        if(!reused && !d3d8_present::prepare(source,bases_[index],960u*544u,hash,hash_scanout_))return false;
        fprintf(log_,"startup_d3d8_present_conversion=%s slot:%u equality:%s\n",
            reused?"reused":"prepared",index,reused?"all_source_bytes":"not_cached");
        SceDisplayFrameBuf fb={};fb.size=sizeof(fb);fb.base=bases_[index];fb.pitch=960;
        fb.pixelformat=SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;fb.width=960;fb.height=544;
        const int rc=sceDisplaySetFrameBuf(&fb,SCE_DISPLAY_SETBUF_NEXTFRAME);
        const int wait_rc=rc>=0?sceDisplayWaitVblankStart():rc;
        SceDisplayFrameBuf active={};active.size=sizeof(active);
        const int query_rc=wait_rc>=0?sceDisplayGetFrameBuf(&active,SCE_DISPLAY_SETBUF_IMMEDIATE):wait_rc;
        const bool matches=query_rc>=0 && active.base==fb.base && active.pitch==fb.pitch &&
            active.width==fb.width && active.height==fb.height && active.pixelformat==fb.pixelformat;
        fprintf(log_,"startup_d3d8_present_native=set_rc:0x%08X wait_rc:0x%08X query_rc:0x%08X matches:%s slot:%u source:640x480 output:960x544 image:725x544\n",
            unsigned(rc),unsigned(wait_rc),unsigned(query_rc),matches?"yes":"no",index);
        if(hash_scanout_)fprintf(log_,"startup_d3d8_scanout_hash=fnv1a:0x%08X\n",hash);
        else fprintf(log_,"startup_d3d8_scanout_hash=disabled equality:all_source_bytes scanout:%s\n",matches?"confirmed":"failed");
        if(rc<0 || wait_rc<0 || !matches){cache_.invalidate();return false;}
        if(!reused && !cache_.remember_confirmed(source,index,hash))
            fprintf(log_,"startup_d3d8_present_cache=unavailable fallback:normal_conversion\n");
        reused_source_=reused;next_=index^1u;return true;
    }
    bool reused_source() const{return reused_source_;}
private:
    bool allocate() {
        if(bases_[0] && bases_[1])return true;
        constexpr unsigned size=(960u*544u*4u+0x3FFFFu)&~0x3FFFFu;
        for(unsigned i=0;i<2;++i) {
            blocks_[i]=sceKernelAllocMemBlock(i?"TH075 EXE frame B":"TH075 EXE frame A",
                SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,size,nullptr);
            void* base=nullptr;
            if(blocks_[i]<0 || sceKernelGetMemBlockBase(blocks_[i],&base)<0) {
                fprintf(log_,"startup_d3d8_present_allocation=failed slot:%u\n",i);return false;
            }
            bases_[i]=static_cast<uint32_t*>(base);
        }
        fprintf(log_,"startup_d3d8_present_allocation=owned_double_buffer bytes:%u\n",size*2);
        return true;
    }
    void release() {
        if(blocks_[0]<0 && blocks_[1]<0)return;
        // A failed SetFrameBuf may still leave a pending request. Wait before
        // checking ownership, and defer freeing if scanout cannot be checked.
        SceDisplayFrameBuf active={};active.size=sizeof(active);
        if(sceDisplayWaitVblankStart()<0 ||
            sceDisplayGetFrameBuf(&active,SCE_DISPLAY_SETBUF_IMMEDIATE)<0) {
            fprintf(log_,"startup_d3d8_present_release=deferred_to_process_exit\n");return;
        }
        if((bases_[0] && active.base==bases_[0]) || (bases_[1] && active.base==bases_[1])) {
            if(sceDisplaySetFrameBuf(nullptr,SCE_DISPLAY_SETBUF_NEXTFRAME)<0 || sceDisplayWaitVblankStart()<0) {
                fprintf(log_,"startup_d3d8_present_release=deferred_to_process_exit\n");return;
            }
        }
        for(unsigned i=0;i<2;++i)if(blocks_[i]>=0) {
            const int rc=sceKernelFreeMemBlock(blocks_[i]);
            fprintf(log_,"startup_d3d8_present_release=slot:%u rc:0x%08X\n",i,unsigned(rc));
            blocks_[i]=-1;bases_[i]=nullptr;
        }
    }
    FILE* log_;SceUID blocks_[2]={-1,-1};uint32_t* bases_[2]={nullptr,nullptr};unsigned next_=0;
    d3d8_present::FrameCache cache_;bool reused_source_=false,hash_scanout_=true;
};
