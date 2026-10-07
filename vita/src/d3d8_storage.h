#pragma once
#include "startup_services.h"
#include "runtime/cpu.h"
#include <array>
#include <vector>
#include <new>
#include <algorithm>
#include <limits>
#include <cstring>

// Partial software graphics bridge. Implements owned storage and state;
// Draw/Present remain explicit boundaries. ABI follows Wine's d3d8.h.
class D3D8Storage {
public:
    static constexpr uint32_t device_trap=0x00BFC000, texture_trap=0x00BFB000,
        surface_trap=0x00BFA000, device=0x00ABA000, device_table=device+0x100,
        texture_table=0x00ABB000, surface_table=0x00ABB100,
        handles=0x00ABC000, staging=0x01400000, staging_size=0x00400000,
        budget=32u*1024u*1024u;
    static constexpr unsigned capacity=512;
    // Slots counted from IUnknown in the primary IDirect3DDevice8 declaration.
    enum DeviceSlot : unsigned {
        CreateRenderTarget=25, CreateDepthStencilSurface=26,
        GetTexture=60, SetTexture=61, GetTextureStageState=62,
        SetTextureStageState=63, ValidateDevice=64
    };
    D3D8Storage(d2rt::Cpu& cpu,FILE* log,uint32_t& root_refs):cpu_(cpu),log_(log),root_refs_(root_refs) {}
    static bool range(uint32_t t,uint32_t b,unsigned n) {return t>=b && t<b+n*16 && (t-b)%16==0;}
    bool owns(uint32_t t) const {return range(t,device_trap,97)||range(t,texture_trap,19)||range(t,surface_trap,11);}
    static const char* interface_name(uint32_t t) {
        return range(t,device_trap,97)?"IDirect3DDevice8":range(t,texture_trap,19)?"IDirect3DTexture8":"IDirect3DSurface8";
    }
    static const char* method(uint32_t t) {
        unsigned s=range(t,device_trap,97)?(t-device_trap)/16:
            range(t,texture_trap,19)?(t-texture_trap)/16:(t-surface_trap)/16;
        if(s<3) {static const char* n[]={"QueryInterface","AddRef","Release"};return n[s];}
        if(range(t,texture_trap,19)) {
            static const char* n[]={"GetDevice","SetPrivateData","GetPrivateData","FreePrivateData","SetPriority","GetPriority","PreLoad","GetType","SetLOD","GetLOD","GetLevelCount","GetLevelDesc","GetSurfaceLevel","LockRect","UnlockRect","AddDirtyRect"};return n[s-3];
        }
        if(range(t,surface_trap,11)) {
            static const char* n[]={"GetDevice","SetPrivateData","GetPrivateData","FreePrivateData","GetContainer","GetDesc","LockRect","UnlockRect"};return n[s-3];
        }
        switch(s) {
        case 3:return "TestCooperativeLevel";case 4:return "GetAvailableTextureMem";
        case 6:return "GetDirect3D";case 7:return "GetDeviceCaps";case 8:return "GetDisplayMode";
        case 9:return "GetCreationParameters";case 15:return "Present";case 16:return "GetBackBuffer";
        case 20:return "CreateTexture";case 31:return "SetRenderTarget";case 32:return "GetRenderTarget";
        case CreateRenderTarget:return "CreateRenderTarget";
        case CreateDepthStencilSurface:return "CreateDepthStencilSurface";
        case 33:return "GetDepthStencilSurface";case 34:return "BeginScene";case 35:return "EndScene";
        case 36:return "Clear";case 50:return "SetRenderState";case 51:return "GetRenderState";
        case 52:return "BeginStateBlock";case 53:return "EndStateBlock";case 54:return "ApplyStateBlock";
        case 55:return "CaptureStateBlock";case 56:return "DeleteStateBlock";case 57:return "CreateStateBlock";
        case GetTexture:return "GetTexture";case SetTexture:return "SetTexture";case GetTextureStageState:return "GetTextureStageState";
        case SetTextureStageState:return "SetTextureStageState";case ValidateDevice:return "ValidateDevice";
        case 72:return "DrawPrimitiveUP";case 76:return "SetVertexShader";
        case 77:return "GetVertexShader";case 88:return "CreatePixelShader";case 89:return "SetPixelShader";
        default:return "UnimplementedSlot";
        }
    }
    static std::array<uint32_t,53> caps(uint32_t type) {
        std::array<uint32_t,53> c{};c[0]=type;
        // Resource limits only. No raster/blend/filter/shader/hardware caps.
        c[22]=c[23]=c[25]=c[26]=1024;
        return c;
    }
    static std::array<uint32_t,4> mode() {return {640,480,60,22};}
    StartupServiceResult create(const uint32_t* w,uint32_t& hr) {
        hr=0x8876086A; // D3DERR_NOTAVAILABLE; let EXE use REF/software fallback.
        if(!w[7] || !put(w[7],0u))return failure();
        std::array<uint32_t,13> p{};
        if(!w[6] || !cpu_.read(w[6],p.data(),sizeof(p)))return failure();
        fprintf(log_,"startup_d3d8_device_request=adapter:%u type:%u behavior:0x%08X width:%u height:%u format:%u depth:%u\n",w[2],w[3],w[5],p[0],p[1],p[2],p[9]);
        if(w[2] || w[3]!=2 || w[5]!=0x20) {
            fprintf(log_,"startup_d3d8_device_policy=requires_REF_software_vertex_processing\n");return serviced();
        }
        if(refs_ || w[4]!=0x00AB7000 || p[6]!=w[4] || p[0]!=640 || p[1]!=480 || p[2]!=22 ||
            p[3]!=1 || p[4] || p[5]!=1 || p[7]!=1 || p[8]!=1 || p[9]!=80 || p[10]!=1 || p[11] || p[12])
            return unsupported();
        if(!table(device_table,device_trap,97) || !table(texture_table,texture_trap,19) ||
            !table(surface_table,surface_trap,11) || !cpu_.write(device,&device_table,4) ||
            !cpu_.map(staging,staging_size,nullptr,d2rt::P_RW))return failure();
        uint32_t back=0,depth=0;
        if(!allocate(640,480,22,1,0,false,back) || !allocate(640,480,80,2,0,false,depth))return unsupported();
        back_=target_=back;depth_=depth;focus_=w[4];refs_=1;++root_refs_;
        if(!put(w[7],device))return failure();
        hr=0;
        fprintf(log_,"startup_d3d8_device=owned_software_storage object=0x%08X backbuffer=0x%08X depth=0x%08X bytes=%u\n",device,back_,depth_,used_);
        fprintf(log_,"startup_d3d8_rasterizer=not_implemented\nstartup_d3d8_caps_shader_versions=zero\n");
        return serviced();
    }
    StartupServiceResult call(uint32_t t) {
        const bool dev=range(t,device_trap,97),tex=range(t,texture_trap,19);
        const unsigned s=(t-(dev?device_trap:tex?texture_trap:surface_trap))/16;
        uint32_t w[10]{};const uint32_t esp=cpu_.reg(d2rt::R_ESP);
        // Unsupported slots stop before touching output/state or guessing ABI.
        const unsigned argc=arguments(dev,tex,s);
        fprintf(log_,"startup_d3d8_method=%s::%s slot=%u\n",interface_name(t),method(t),s);
        if(!argc)return unsupported();
        const unsigned cleanup=(argc+1)*4;
        if(esp<0x00800000 || uint64_t(esp)+cleanup>0x00A00000 || !cpu_.read(esp,w,cleanup) || !refs_)return failure();
        Resource* r=dev?nullptr:find(w[1]);
        const uint32_t expected=dev?device_table:tex?texture_table:surface_table;
        uint32_t actual=0;
        if((dev?w[1]!=device:!r || r->texture!=tex) || !cpu_.read(w[1],&actual,4) || actual!=expected)return failure();
        for(unsigned i=1;i<=argc;++i)fprintf(log_,"startup_d3d8_arg%u=0x%08X\n",i-1,w[i]);
        uint32_t value=0;StartupServiceResult result=serviced();
        if(s==0)return unsupported(); // exact IID support is a later boundary.
        if(s==1) {
            uint32_t& refs=dev?refs_:r->refs;
            if(refs==std::numeric_limits<uint32_t>::max())return failure();
            value=++refs;
        } else if(s==2) {
            // Full device/resource destruction is an explicit boundary for now.
            uint32_t& refs=dev?refs_:r->refs;
            if(refs<=1)return unsupported();
            value=--refs;
        } else if(dev)result=device_call(s,w,value);
        else result=resource_call(tex,s,*r,w,value);
        if(result!=StartupServiceResult::Serviced)return result;
        const int registers[]={d2rt::R_EBX,d2rt::R_EBP,d2rt::R_ESI,d2rt::R_EDI};uint32_t before[4]{};
        for(unsigned i=0;i<4;++i)before[i]=cpu_.reg(registers[i]);
        cpu_.trap_epilogue(value,cleanup,w[0]);
        bool valid=cpu_.reg(d2rt::R_EAX)==value && cpu_.reg(d2rt::R_ESP)==esp+cleanup && cpu_.reg(d2rt::R_EIP)==w[0];
        for(unsigned i=0;i<4;++i)valid=valid && cpu_.reg(registers[i])==before[i];
        fprintf(log_,"startup_d3d8_abi=%s cleanup=%u hresult=0x%08X\n",valid?"passed":"failed",cleanup,value);
        return valid?serviced():failure();
    }
private:
    struct Resource {
        uint32_t handle=0,refs=0,width=0,height=0,format=0,usage=0,pool=0,pitch=0,surface=0,parent=0;
        bool texture=false,locked=false;std::vector<uint8_t> bytes;
    };
    static StartupServiceResult serviced(){return StartupServiceResult::Serviced;}
    static StartupServiceResult failure(){return StartupServiceResult::ContractFailure;}
    StartupServiceResult unsupported(){fprintf(log_,"startup_d3d8_method_executed=no\n");return StartupServiceResult::Unsupported;}
    template<class T> bool put(uint32_t address,const T& value){
        // Reject low integer values before Cpu::write: the emulated zero page
        // may be mapped for FS/SEH, but is not a COM output buffer.
        if(address<0x10000 || uint64_t(address)+sizeof(value)>0x02000000) {
            fprintf(log_,"startup_d3d8_output_rejected=address:0x%08X bytes:%u\n",address,unsigned(sizeof(value)));
            return false;
        }
        return cpu_.write(address,&value,sizeof(value));
    }
    bool table(uint32_t address,uint32_t traps,unsigned count) {
        std::array<uint32_t,97> a{},b{};for(unsigned i=0;i<count;++i)a[i]=traps+i*16;
        return cpu_.write(address,a.data(),count*4) && cpu_.read(address,b.data(),count*4) && a==b;
    }
    Resource* find(uint32_t h) {
        if(h<handles || (h-handles)%8 || (h-handles)/8>=count_)return nullptr;
        auto& r=resources_[(h-handles)/8];return r.refs?&r:nullptr;
    }
    bool allocate(uint32_t width,uint32_t height,uint32_t format,uint32_t usage,uint32_t pool,bool texture,uint32_t& output) {
        if(count_==capacity)return false;
        const unsigned bpp=(format==25 || format==80)?2:4;
        const uint64_t bytes=uint64_t(width)*height*bpp;
        if(bytes>budget-used_)return false;
        auto& r=resources_[count_];
        try {r.bytes.resize(bytes);}catch(const std::bad_alloc&){return false;}
        r.handle=handles+count_*8;r.refs=1;r.width=width;r.height=height;r.format=format;
        r.usage=usage;r.pool=pool;r.texture=texture;r.pitch=width*bpp;
        const uint32_t vt=texture?texture_table:surface_table;
        if(!put(r.handle,vt)){r.bytes.clear();r.refs=0;return false;}
        ++count_;used_+=bytes;output=r.handle;return true;
    }
    // Counts include `this`, as required by stdcall COM on x86.
    static unsigned arguments(bool dev,bool tex,unsigned s) {
        if(s==0)return 3;
        if(s==1 || s==2)return 1;
        if(!dev) {
            if(s==3)return 2;
            if(tex){switch(s){case 10:case 13:return 1;case 14:case 15:return 3;case 16:return 5;case 17:return 2;default:return 0;}}
            switch(s){case 8:return 2;case 9:return 4;case 10:return 1;default:return 0;}
        }
        switch(s){case 3:case 4:case 34:case 35:return 1;
        case 6:case 7:case 8:case 9:case 32:case 33:case 76:case 77:return 2;
        case 16:return 4;case 20:return 8;case CreateDepthStencilSurface:return 6;case 36:return 7;
        case 50:case 51:return 3;case GetTextureStageState:case SetTextureStageState:return 4;default:return 0;}
    }
    StartupServiceResult device_call(unsigned s,const uint32_t* w,uint32_t& value) {
        switch(s) {
        case 3:return serviced(); // Owned device has no lost-device transition yet.
        case 4:value=budget-used_;return serviced();
        case 6:if(!put(w[2],0x00AB9000u))return failure();++root_refs_;return serviced();
        case 7:return put(w[2],caps(2))?serviced():failure();
        case 8:return put(w[2],mode())?serviced():failure();
        case 9:return put(w[2],std::array<uint32_t,4>{0,2,focus_,0x20})?serviced():failure();
        case 16:if(w[2] || w[3])return unsupported();return surface_output(back_,w[4]);
        case 32:return surface_output(target_,w[2]);
        case 33:return surface_output(depth_,w[2]);
        case CreateDepthStencilSurface: {
            if(!w[6] || !put(w[6],0u))return failure();
            if(!w[2] || !w[3] || w[2]>1024 || w[3]>1024 || w[4]!=80 || w[5])return unsupported();
            uint32_t h=0;if(!allocate(w[2],w[3],80,2,0,false,h)){value=0x8876017C;return serviced();}
            if(!put(w[6],h))return failure();
            ++refs_;
            fprintf(log_,"startup_d3d8_depth_surface=owned object=0x%08X width=%u height=%u format=D16\n",h,w[2],w[3]);
            return serviced();
        }
        case 20: {
            if(!w[8] || !put(w[8],0u))return failure();
            if(!w[2] || !w[3] || w[2]>1024 || w[3]>1024 || (w[2]&(w[2]-1)) || (w[3]&(w[3]-1)) ||
                w[4]!=1 || (w[5]!=0 && w[5]!=1) || (w[6]!=21 && w[6]!=25) || w[7]>1 ||
                (w[5] && (w[6]!=21 || w[7]!=0)))return unsupported();
            uint32_t h=0;if(!allocate(w[2],w[3],w[6],w[5],w[7],true,h)) {value=0x8876017C;return serviced();}
            ++refs_;if(!put(w[8],h))return failure();
            fprintf(log_,"startup_d3d8_texture=allocated object=0x%08X width=%u height=%u format=%u pitch=%u bytes=%u used=%u\n",h,w[2],w[3],w[6],find(h)->pitch,unsigned(find(h)->bytes.size()),used_);
            return serviced();
        }
        case 50:if(w[2]>=render_.size() || !render_state(w[2],w[3]))return unsupported();render_[w[2]]=w[3];return serviced();
        case 51:if(w[2]>=render_.size())return unsupported();return put(w[3],render_[w[2]])?serviced():failure();
        case GetTextureStageState:
            if(w[2] || w[3]>=stage_.size() || !stage_valid_[w[3]])return unsupported();
            fprintf(log_,"startup_d3d8_stage_get=stage:%u type:%u output:0x%08X value:%u\n",w[2],w[3],w[4],stage_[w[3]]);
            return put(w[4],stage_[w[3]])?serviced():failure();
        case SetTextureStageState:
            if(w[2] || w[3]>=stage_.size() || !stage_state(w[3],w[4]))return unsupported();
            stage_[w[3]]=w[4];stage_valid_[w[3]]=true;
            fprintf(log_,"startup_d3d8_stage_set=stage:%u type:%u value:%u output_write:none\n",w[2],w[3],w[4]);
            return serviced();
        case 76:if(w[2]!=0x144)return unsupported();fvf_=w[2];return serviced();
        case 77:return put(w[2],fvf_)?serviced():failure();
        case 34:if(scene_)return failure();scene_=true;return serviced();
        case 35:if(!scene_)return failure();scene_=false;return serviced();
        case 36: {
            if(w[2] || w[3] || !w[4] || (w[4]&~3u))return unsupported();
            auto* color=find(target_);auto* depth=find(depth_);
            if(!color || !depth || color->format!=22)return failure();
            float z=0;std::memcpy(&z,&w[6],4);if((w[4]&2) && !(z>=0 && z<=1))return failure();
            if(w[4]&1)for(size_t i=0;i<color->bytes.size();i+=4)std::memcpy(color->bytes.data()+i,&w[5],4);
            if(w[4]&2) {
                const uint16_t d=uint16_t(z*65535.0f);
                for(size_t i=0;i<depth->bytes.size();i+=2)std::memcpy(depth->bytes.data()+i,&d,2);
            }
            fprintf(log_,"startup_d3d8_clear=owned_storage flags=%u\n",w[4]);return serviced();
        }
        default:return unsupported();
        }
    }
    StartupServiceResult surface_output(uint32_t h,uint32_t out) {
        auto* r=find(h);if(!r || r->texture || !put(out,h))return failure();++r->refs;return serviced();
    }
    StartupServiceResult desc(Resource& r,uint32_t out) {
        Resource* data=r.parent?find(r.parent):&r;if(!data)return failure();
        const std::array<uint32_t,8> d={data->format,1,data->usage,data->pool,uint32_t(data->bytes.size()),0,data->width,data->height};
        return put(out,d)?serviced():failure();
    }
    StartupServiceResult resource_call(bool tex,unsigned s,Resource& r,const uint32_t* w,uint32_t& value) {
        if(s==3){if(!put(w[2],device))return failure();++refs_;return serviced();}
        if(tex && s==10){value=3;return serviced();} // D3DRTYPE_TEXTURE
        if(tex && s==13){value=1;return serviced();}
        if((tex && s==14)||(!tex && s==8)) {
            if(tex && w[2])return unsupported();
            return desc(r,w[tex?3:2]);
        }
        if(tex && s==15) {
            if(w[2] || !w[3])return unsupported();
            if(!r.surface) {
                if(count_==capacity)return unsupported();
                auto& child=resources_[count_];
                child.handle=handles+count_*8;child.refs=1;child.parent=r.handle;
                if(!put(child.handle,surface_table))return failure();
                ++count_;r.surface=child.handle;++r.refs;
            }
            return surface_output(r.surface,w[3]);
        }
        Resource* data=r.parent?find(r.parent):&r;if(!data)return failure();
        if((tex && s==16)||(!tex && s==9)) {
            const uint32_t output=w[tex?3:2],rect=w[tex?4:3],flags=w[tex?5:4];
            if((tex && w[2]) || rect || flags || data->locked || locked_ || data->format==80)return unsupported();
            if(data->bytes.size()>staging_size || !cpu_.write(staging,data->bytes.data(),data->bytes.size()) ||
                !put(output,std::array<uint32_t,2>{data->pitch,staging}))return failure();
            data->locked=true;locked_=data->handle;
            fprintf(log_,"startup_d3d8_texture_lock=object:0x%08X pitch:%u guest_bits:0x%08X bytes:%u\n",data->handle,data->pitch,staging,unsigned(data->bytes.size()));return serviced();
        }
        if((tex && s==17)||(!tex && s==10)) {
            if((tex && w[2]) || !data->locked || locked_!=data->handle)return failure();
            if(!cpu_.read(staging,data->bytes.data(),data->bytes.size()))return failure();
            uint32_t hash=2166136261u,alpha_zero=0,alpha_full=0,alpha_partial=0;
            for(uint8_t b:data->bytes)hash=(hash^b)*16777619u;
            for(size_t i=0;i<data->bytes.size();i+=data->format==25?2:4) {
                const uint8_t a=data->format==25?(data->bytes[i+1]&128?255:0):data->bytes[i+3];
                if(!a)++alpha_zero;else if(a==255)++alpha_full;else ++alpha_partial;
            }
            data->locked=false;locked_=0;
            fprintf(log_,"startup_d3d8_texture_upload=object:0x%08X format:%u bytes:%u fnv1a:0x%08X alpha_zero:%u alpha_full:%u alpha_partial:%u\n",data->handle,data->format,unsigned(data->bytes.size()),hash,alpha_zero,alpha_full,alpha_partial);
            return serviced();
        }
        return unsupported();
    }
    static bool render_state(uint32_t state,uint32_t value) {
        switch(state){case 7:case 137:return value==0;case 15:case 27:return value<=1;
        case 19:return value==5;case 20:return value==6;case 22:return value==1;
        case 24:return value<=255;case 25:return value==7;default:return false;}
    }
    static bool stage_state(uint32_t state,uint32_t value) {
        switch(state){case 1:case 4:return value==4;case 2:case 5:return value==2;
        case 3:return value==0;case 6:return value==1;case 13:case 14:return value==1;
        case 15:return value==0;case 16:case 17:case 18:return value==1;default:return false;}
    }
    d2rt::Cpu& cpu_;FILE* log_;uint32_t& root_refs_;
    std::array<Resource,capacity> resources_{};
    std::array<uint32_t,256> render_{};std::array<uint32_t,32> stage_{};
    std::array<bool,32> stage_valid_{};
    uint32_t refs_=0,used_=0,count_=0,back_=0,depth_=0,target_=0,focus_=0,locked_=0,fvf_=0;
    bool scene_=false;
};
