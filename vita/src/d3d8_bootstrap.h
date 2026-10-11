#pragma once
#include "startup_services.h"
#include "runtime/cpu.h"
#include <array>
#include <cstring>
#include <limits>
#include "d3d8_storage.h"

// Owned x86 COM root and partial software device/resource bridge.
class D3D8Bootstrap {
public:
    static constexpr uint32_t trap_base=0x00BFE000, object=0x00AB9000, vtable=object+0x100;
    static constexpr unsigned slots=16;
    D3D8Bootstrap(d2rt::Cpu& cpu, FILE* log):cpu_(cpu),log_(log),storage_(cpu,log,references_) {}
    unsigned serviced_calls() const { return serviced_calls_; }
    void set_raster_executor(d3d8_quad::RasterExecutor executor,void* context) {
        storage_.set_raster_executor(executor,context);
    }
    void report_usage() const { storage_.report_usage(); }
    bool owns_trap(uint32_t trap) const {
        return storage_.owns(trap) || (trap>=trap_base && trap<trap_base+slots*16 && (trap-trap_base)%16==0);
    }
    const char* interface_name(uint32_t trap) const {return storage_.owns(trap)?D3D8Storage::interface_name(trap):"IDirect3D8";}
    const char* method_name(uint32_t trap) const {return storage_.owns(trap)?D3D8Storage::method(trap):name((trap-trap_base)/16);}
    static const char* name(unsigned slot) {
        static const char* names[]={"QueryInterface","AddRef","Release","RegisterSoftwareDevice",
            "GetAdapterCount","GetAdapterIdentifier","GetAdapterModeCount","EnumAdapterModes",
            "GetAdapterDisplayMode","CheckDeviceType","CheckDeviceFormat","CheckDeviceMultiSampleType",
            "CheckDepthStencilMatch","GetDeviceCaps","GetAdapterMonitor","CreateDevice"};
        return slot<slots ? names[slot]:"Unknown";
    }
    StartupServiceResult create() {
        uint32_t ret=0, sdk=0; const uint32_t esp=cpu_.reg(d2rt::R_ESP);
        if(!frame(esp,8) || !cpu_.read(esp,&ret,4) || !cpu_.read(esp+4,&sdk,4))
            return StartupServiceResult::ContractFailure;
        if(sdk!=220 || references_) return StartupServiceResult::Unsupported;
        std::array<uint32_t,slots> methods{};
        for(unsigned i=0;i<slots;++i)methods[i]=trap_base+i*16;
        if(!cpu_.map(object,0x10000,nullptr,d2rt::P_RW) || !cpu_.write(object,&vtable,4) ||
            !cpu_.write(vtable,methods.data(),sizeof(methods)))return StartupServiceResult::ContractFailure;
        uint32_t copy=0;
        if(!cpu_.read(object,&copy,4) || copy!=vtable)return StartupServiceResult::ContractFailure;
        std::array<uint32_t,slots> readback{};
        if(!cpu_.read(vtable,readback.data(),sizeof(readback)) || readback!=methods)
            return StartupServiceResult::ContractFailure;
        references_=1;
        fprintf(log_,"startup_d3d8_root=owned sdk=%u object=0x%08X vtable=0x%08X slots=%u\n",sdk,object,vtable,slots);
        fprintf(log_,"startup_d3d8_vtable_readback=passed\nstartup_d3d8_renderer=software_storage_partial\n");
        return finish(object,8,ret,esp);
    }
    StartupServiceResult call(uint32_t trap) {
        if(storage_.owns(trap)) {
            const auto result=storage_.call(trap);
            if(result==StartupServiceResult::Serviced)++serviced_calls_;
            return result;
        }
        const unsigned slot=(trap-trap_base)/16;
        const unsigned args[]={3,1,1,2,1,4,2,4,3,6,7,6,6,4,2,7};
        const uint32_t esp=cpu_.reg(d2rt::R_ESP), cleanup=(args[slot]+1)*4;
        uint32_t words[8]{};
        if(!frame(esp,cleanup) || !cpu_.read(esp,words,cleanup) || !references_ || words[1]!=object)
            return StartupServiceResult::ContractFailure;
        uint32_t table=0;
        if(!cpu_.read(object,&table,4) || table!=vtable)return StartupServiceResult::ContractFailure;
        fprintf(log_,"startup_d3d8_method=IDirect3D8::%s return=0x%08X esp=0x%08X\n",name(slot),words[0],esp);
        for(unsigned i=1;i<=args[slot];++i)fprintf(log_,"startup_d3d8_arg%u=0x%08X\n",i-1,words[i]);
        uint32_t value=0;
        if(slot==0) {
            // IID_IUnknown and IID_IDirect3D8; packed Windows GUID byte order.
            const uint32_t unknown[]={0,0,0x000000C0,0x46000000};
            const uint32_t direct3d[]={0x1DD9E8DA,0x4D401C77,0xFE98CFB0,0x1295FFFD};
            uint32_t iid[4]{};
            if(!words[2] || !words[3] || !cpu_.read(words[2],iid,sizeof(iid)))return StartupServiceResult::ContractFailure;
            const bool supported=!std::memcmp(iid,unknown,sizeof(iid)) || !std::memcmp(iid,direct3d,sizeof(iid));
            value=supported ? 0:0x80004002;
            const uint32_t output=supported ? object:0;
            if(supported && references_==std::numeric_limits<uint32_t>::max())return StartupServiceResult::ContractFailure;
            if(!cpu_.write(words[3],&output,4))return StartupServiceResult::ContractFailure;
            if(supported)++references_;
        } else if(slot==1) {
            if(references_==std::numeric_limits<uint32_t>::max())return StartupServiceResult::ContractFailure;
            value=++references_;
        } else if(slot==2) {
            value=--references_;
            if(!references_) {uint32_t zero=0;if(!cpu_.write(object,&zero,4))return StartupServiceResult::ContractFailure;}
        } else if(slot==4) {
            value=1;
        } else if(slot==6) {
            value=words[2]?0:1;
        } else if(slot==7 || slot==8) {
            if(words[2] || (slot==7 && words[3]))value=0x8876086C;
            else {
                const auto mode=D3D8Storage::mode();
                if(!words[slot==7?4:3] || !cpu_.write(words[slot==7?4:3],mode.data(),sizeof(mode)))return StartupServiceResult::ContractFailure;
                fprintf(log_,"startup_d3d8_display_mode=logical_640x480_60_X8R8G8B8\n");
            }
        } else if(slot==10) {
            // Storage formats only; draw and shader capabilities remain zero.
            const bool valid=words[2]==0 && (words[3]==1 || words[3]==2) && words[4]==22 &&
                ((words[6]==3 && ((words[5]==0 && (words[7]==21 || words[7]==25)) ||
                   (words[5]==1 && words[7]==21))) || (words[6]==1 && words[5]==2 && words[7]==80));
            value=valid?0:0x8876086A;
        } else if(slot==13) {
            if(words[2] || (words[3]!=1 && words[3]!=2))value=0x8876086C;
            else {
                const auto caps=D3D8Storage::caps(words[3]);
                if(!words[4] || !cpu_.write(words[4],caps.data(),sizeof(caps)))return StartupServiceResult::ContractFailure;
                fprintf(log_,"startup_d3d8_caps=resource_limits_1024 raster_caps:0 shader_versions:0 bytes:212\n");
            }
        } else if(slot==15) {
            const auto result=storage_.create(words,value);
            if(result!=StartupServiceResult::Serviced)return result;
        } else {
            fprintf(log_,"startup_d3d8_method_executed=no\n");
            return StartupServiceResult::Unsupported;
        }
        fprintf(log_,"startup_d3d8_references=%u\n",references_);
        return finish(value,cleanup,words[0],esp);
    }
private:
    bool frame(uint32_t esp,uint32_t bytes) const {
        return esp>=0x00800000 && uint64_t(esp)+bytes<=0x00A00000;
    }
    StartupServiceResult finish(uint32_t value,uint32_t cleanup,uint32_t ret,uint32_t esp) {
        uint32_t before[4]{}; const int regs[]={d2rt::R_EBX,d2rt::R_EBP,d2rt::R_ESI,d2rt::R_EDI};
        for(unsigned i=0;i<4;++i)before[i]=cpu_.reg(regs[i]);
        cpu_.trap_epilogue(value,cleanup,ret);
        bool valid=cpu_.reg(d2rt::R_EAX)==value && cpu_.reg(d2rt::R_ESP)==esp+cleanup && cpu_.reg(d2rt::R_EIP)==ret;
        for(unsigned i=0;i<4;++i)valid=valid && before[i]==cpu_.reg(regs[i]);
        fprintf(log_,"startup_d3d8_abi=%s cleanup=%u\n",valid ? "passed":"failed",cleanup);
        if(valid)++serviced_calls_;
        return valid ? StartupServiceResult::Serviced:StartupServiceResult::ContractFailure;
    }
    d2rt::Cpu& cpu_; FILE* log_; uint32_t references_=0; unsigned serviced_calls_=0;
    D3D8Storage storage_;
};
