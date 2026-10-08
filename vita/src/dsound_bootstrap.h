#pragma once
#include "startup_services.h"
#include "runtime/cpu.h"
#include <psp2/audioout.h>
#include <array>
#include <limits>

// Restricted original TH075 COM activation and native audio-port ownership.
// Buffer allocation, mixing and playback remain explicit unsupported methods.
class DirectSoundBootstrap {
public:
    static constexpr uint32_t object=0x00AD1000, vtable=object+0x100, trap_base=0x00BF6000,
        primary=0x00AD2000,primary_table=primary+0x100,primary_trap=0x00BF5000;
    DirectSoundBootstrap(d2rt::Cpu& cpu,FILE* log):cpu_(cpu),log_(log) {}
    ~DirectSoundBootstrap(){close_port();}
    bool owns(uint32_t t)const{return (t>=trap_base && t<trap_base+12*16 && (t-trap_base)%16==0) || (t>=primary_trap && t<primary_trap+21*16 && (t-primary_trap)%16==0);}
    const char* interface_name(uint32_t t)const{return t< trap_base?"IDirectSoundBuffer":"IDirectSound8";}
    const char* method_name(uint32_t t)const{
        if(t<trap_base){static const char* names[]={"QueryInterface","AddRef","Release","GetCaps","GetCurrentPosition","GetFormat","GetVolume","GetPan","GetFrequency","GetStatus","Initialize","Lock","Play","SetCurrentPosition","SetFormat","SetVolume","SetPan","SetFrequency","Stop","Unlock","Restore"};return names[(t-primary_trap)/16];}
        static const char* names[]={"QueryInterface","AddRef","Release","CreateSoundBuffer","GetCaps","DuplicateSoundBuffer","SetCooperativeLevel","Compact","GetSpeakerConfig","SetSpeakerConfig","Initialize","VerifyCertification"};
        return names[(t-trap_base)/16];
    }
    unsigned serviced_calls()const{return calls_;}
    StartupServiceResult create(bool apartment_ready){
        uint32_t w[6]{};const uint32_t esp=cpu_.reg(d2rt::R_ESP);
        if(!frame(esp,sizeof(w)) || !cpu_.read(esp,w,sizeof(w)))return failure();
        Guid clsid{},iid{};
        if(!guid(w[1],clsid) || !guid(w[4],iid) || !write(w[5],0u))return failure();
        fprintf(log_,"startup_dsound_create=clsid:%08X-%08X-%08X-%08X iid:%08X-%08X-%08X-%08X outer:0x%08X context:%u output:0x%08X apartment:%s\n",clsid[0],clsid[1],clsid[2],clsid[3],iid[0],iid[1],iid[2],iid[3],w[2],w[3],w[5],apartment_ready?"ready":"missing");
        if(!apartment_ready)return finish(0x800401F0,sizeof(w),w[0],esp);
        if(clsid!=sound_clsid || iid!=sound_iid || w[2] || w[3]!=3 || refs_)return unsupported();
        if(!cpu_.map(object,0x1000,nullptr,d2rt::P_RW))return failure();
        std::array<uint32_t,12> table{},check{};
        for(unsigned i=0;i<table.size();++i)table[i]=trap_base+16*i;
        if(!cpu_.write(vtable,table.data(),sizeof(table)) || !cpu_.read(vtable,check.data(),sizeof(check)) || table!=check || !write(object,vtable) || !write(w[5],object))return failure();
        refs_=1;
        fprintf(log_,"startup_dsound_root=owned vtable_readback=passed initialized:no playback:no\n");
        return finish(0,sizeof(w),w[0],esp);
    }
    StartupServiceResult call(uint32_t t){
        if(t<trap_base)return primary_call(t);
        const unsigned slot=(t-trap_base)/16;
        static const unsigned argc[]={3,1,1,4,2,3,3,1,2,2,2,2};
        const uint32_t esp=cpu_.reg(d2rt::R_ESP),cleanup=4*(argc[slot]+1);
        uint32_t w[5]{},table=0;
        fprintf(log_,"startup_dsound_method=IDirectSound8::%s slot=%u\n",method_name(t),slot);
        if(!frame(esp,cleanup) || !cpu_.read(esp,w,cleanup) || !refs_ || w[1]!=object || !cpu_.read(object,&table,4) || table!=vtable)return failure();
        for(unsigned i=1;i<=argc[slot];++i)fprintf(log_,"startup_dsound_arg%u=0x%08X\n",i-1,w[i]);
        uint32_t hr=0;
        if(slot==0){
            Guid iid{};if(!guid(w[2],iid))return failure();
            const bool match=iid==unknown_iid || iid==sound_iid;
            if(match && refs_==std::numeric_limits<uint32_t>::max())return failure();
            if(!write(w[3],match?object:0u))return failure();
            if(match)++refs_;else hr=0x80004002;
        }else if(slot==1){
            if(refs_==std::numeric_limits<uint32_t>::max())return failure();
            hr=++refs_;
        }else if(slot==2){
            hr=--refs_;
            if(!refs_){if(!write(object,0u))return failure();close_port();cooperative_=false;}
        }else if(slot==10){
            if(w[2])return unsupported(); // Only the original default playback request.
            if(port_>=0)hr=0x88780082; // DSERR_ALREADYINITIALIZED
            else{
                port_=sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN,1024,48000,SCE_AUDIO_OUT_MODE_STEREO);
                fprintf(log_,"startup_dsound_native_open_rc=0x%08X\n",unsigned(port_));
                if(port_<0)return failure();
                const int len=sceAudioOutGetConfig(port_,SCE_AUDIO_OUT_CONFIG_TYPE_LEN);
                const int freq=sceAudioOutGetConfig(port_,SCE_AUDIO_OUT_CONFIG_TYPE_FREQ);
                const int mode=sceAudioOutGetConfig(port_,SCE_AUDIO_OUT_CONFIG_TYPE_MODE);
                fprintf(log_,"startup_dsound_native_config=frames:%d hz:%d mode:%d playback:no\n",len,freq,mode);
                if(len!=1024 || freq!=48000 || mode!=SCE_AUDIO_OUT_MODE_STEREO){close_port();return failure();}
                fprintf(log_,"startup_dsound_native_port=owned readback:passed\n");
            }
        }else if(slot==6){
            if(port_<0)return failure();
            if(w[2]!=0x00AB7000 || (w[3]!=1 && w[3]!=2))return unsupported();
            cooperative_=true;
            fprintf(log_,"startup_dsound_cooperative=owned_window level:%u\n",w[3]);
        }else if(slot==3){
            if(port_<0 || !cooperative_)return failure();
            std::array<uint32_t,9> desc{};
            if(w[2]<0x10000 || !cpu_.read(w[2],desc.data(),sizeof(desc)))return failure();
            fprintf(log_,"startup_dsound_buffer_boundary=size:%u flags:0x%08X bytes:%u reserved:%u format:0x%08X output:0x%08X aggregate:0x%08X\n",desc[0],desc[1],desc[2],desc[3],desc[4],w[3],w[4]);
            if(desc[0]!=36 || desc[1]!=0x00040001 || desc[2] || desc[3] || desc[4] || desc[5] || desc[6] || desc[7] || desc[8] || w[4] || primary_refs_)return unsupported();
            if(refs_==std::numeric_limits<uint32_t>::max() || !cpu_.map(primary,0x1000,nullptr,d2rt::P_RW))return failure();
            std::array<uint32_t,21> table{},check{};for(unsigned i=0;i<21;++i)table[i]=primary_trap+i*16;
            if(!cpu_.write(primary_table,table.data(),sizeof(table)) || !cpu_.read(primary_table,check.data(),sizeof(check)) || table!=check || !write(primary,primary_table) || !write(w[3],primary))return failure();
            primary_refs_=1;++refs_;
            fprintf(log_,"startup_dsound_primary=owned_native_output_port vtable_readback:passed bytes:0 playback:no\n");
        }else return unsupported();
        return finish(hr,cleanup,w[0],esp);
    }
private:
    StartupServiceResult primary_call(uint32_t t){
        const unsigned slot=(t-primary_trap)/16;
        fprintf(log_,"startup_dsound_method=IDirectSoundBuffer::%s slot=%u\n",method_name(t),slot);
        if(slot>2)return unsupported();
        const uint32_t esp=cpu_.reg(d2rt::R_ESP),cleanup=slot==0?16:8;uint32_t w[4]{},vt=0;
        if(!frame(esp,cleanup) || !cpu_.read(esp,w,cleanup) || !primary_refs_ || w[1]!=primary || !cpu_.read(primary,&vt,4) || vt!=primary_table)return failure();
        uint32_t hr=0;
        if(slot==0){Guid iid{};if(!guid(w[2],iid))return failure();
            const bool match=iid==unknown_iid || iid==buffer_iid;
            if(match && primary_refs_==std::numeric_limits<uint32_t>::max())return failure();
            if(!write(w[3],match?primary:0u))return failure();
            if(match)++primary_refs_;else hr=0x80004002;
        }else if(slot==1){if(primary_refs_==std::numeric_limits<uint32_t>::max())return failure();hr=++primary_refs_;}
        else{hr=--primary_refs_;if(!primary_refs_){if(!write(primary,0u) || !refs_)return failure();--refs_;if(!refs_)close_port();}}
        return finish(hr,cleanup,w[0],esp);
    }
    using Guid=std::array<uint32_t,4>;
    inline static constexpr Guid sound_clsid={0x3901CC3F,0x4FA484B5,0x81AA35BA,0x9BA0B872};
    inline static constexpr Guid sound_iid={0xC50A7E93,0x4834F395,0xA97FF69E,0x6609E59D};
    inline static constexpr Guid unknown_iid={0,0,0x000000C0,0x46000000};
    inline static constexpr Guid buffer_iid={0x279AFA85,0x11CE4981,0x200021A5,0x60E50BAF};
    static bool frame(uint32_t esp,uint32_t size){return esp>=0x00800000 && uint64_t(esp)+size<=0x00A00000;}
    bool guid(uint32_t p,Guid& value){return p>=0x10000 && cpu_.read(p,value.data(),sizeof(value));}
    bool write(uint32_t p,uint32_t value){return p>=0x10000 && uint64_t(p)+4<=0x02000000 && cpu_.write(p,&value,4);}
    void close_port(){
        if(port_>=0){const int rc=sceAudioOutReleasePort(port_);fprintf(log_,"startup_dsound_native_release_rc=0x%08X\n",unsigned(rc));port_=-1;}
    }
    StartupServiceResult finish(uint32_t hr,uint32_t cleanup,uint32_t ret,uint32_t esp){
        const int regs[]={d2rt::R_EBX,d2rt::R_EBP,d2rt::R_ESI,d2rt::R_EDI};
        std::array<uint32_t,4> before{};for(unsigned i=0;i<4;++i)before[i]=cpu_.reg(regs[i]);
        cpu_.trap_epilogue(hr,cleanup,ret);
        bool valid=cpu_.reg(d2rt::R_EAX)==hr && cpu_.reg(d2rt::R_ESP)==esp+cleanup && cpu_.reg(d2rt::R_EIP)==ret;
        for(unsigned i=0;i<4;++i)valid=valid && cpu_.reg(regs[i])==before[i];
        fprintf(log_,"startup_dsound_abi=%s cleanup=%u hresult=0x%08X return=0x%08X\n",valid?"passed":"failed",cleanup,hr,ret);
        if(valid)++calls_;
        return valid?StartupServiceResult::Serviced:failure();
    }
    StartupServiceResult unsupported(){fprintf(log_,"startup_dsound_method_executed=no playback:no\n");return StartupServiceResult::Unsupported;}
    static StartupServiceResult failure(){return StartupServiceResult::ContractFailure;}
    d2rt::Cpu& cpu_;FILE* log_;uint32_t refs_=0,primary_refs_=0;unsigned calls_=0;int port_=-1;bool cooperative_=false;
};
