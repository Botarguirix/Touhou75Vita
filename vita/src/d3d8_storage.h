#pragma once
#include "startup_services.h"
#include "runtime/cpu.h"
#include "d3d8_quad_raster.h"
#include "exe_draw_preview.h"
#include "exe_presenter.h"
#include <array>
#include <vector>
#include <new>
#include <algorithm>
#include <limits>
#include <cstring>
#include <cmath>

// Partial software graphics bridge: owned storage/state and the observed point
// textured quads and bounded native Present. Other profiles remain boundaries.
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
    D3D8Storage(d2rt::Cpu& cpu,FILE* log,uint32_t& root_refs):cpu_(cpu),log_(log),root_refs_(root_refs),presenter_(log) { initialize_states(); }
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
        case 40:return "SetViewport";case 41:return "GetViewport";case 36:return "Clear";case 50:return "SetRenderState";case 51:return "GetRenderState";
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
        if(!cpu_.hostptr(staging,staging_size)) {
            fprintf(log_,"startup_d3d8_storage_error=staging_outside_guest_arena address:0x%08X bytes:%u\n",staging,staging_size);
            return failure();
        }
        if(!table(device_table,device_trap,97) || !table(texture_table,texture_trap,19) ||
            !table(surface_table,surface_trap,11) || !cpu_.write(device,&device_table,4) ||
            !cpu_.map(staging,staging_size,nullptr,d2rt::P_RW))return failure();
        uint32_t back=0,depth=0;
        if(!allocate(640,480,22,1,0,false,back) || !allocate(640,480,80,2,0,false,depth))return unsupported();
        back_=target_=back;depth_=depth;++find(back)->refs;++find(depth)->refs;focus_=w[4];refs_=1;++root_refs_;
        if(!put(w[7],device))return failure();
        hr=0;
        fprintf(log_,"startup_d3d8_device=owned_software_storage object=0x%08X backbuffer=0x%08X depth=0x%08X bytes=%u\n",device,back_,depth_,used_);
        fprintf(log_,"startup_d3d8_rasterizer=bounded_point_quad\nstartup_d3d8_caps_shader_versions=zero\n");
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
        fprintf(log_,"startup_d3d8_return=0x%08X esp:0x%08X\n",w[0],esp);
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
        switch(s){case 3:case 4:case 34:case 35:case 52:return 1;
        case 6:case 7:case 8:case 9:case 32:case 33:case 40:case 41:case 53:case 54:case 55:case 56:case 76:case 77:return 2;
        case 16:return 4;case 20:return 8;case CreateDepthStencilSurface:return 6;case 36:return 7;
        case 31:case 50:case 51:case GetTexture:case SetTexture:return 3;case 15:case 72:return 5;case GetTextureStageState:case SetTextureStageState:return 4;default:return 0;}
    }
    StartupServiceResult device_call(unsigned s,const uint32_t* w,uint32_t& value) {
        switch(s) {
        case 3:return serviced(); // Owned device has no lost-device transition yet.
        case 4:value=budget-used_;return serviced();
        case 6:if(!put(w[2],0x00AB9000u))return failure();++root_refs_;return serviced();
        case 7:return put(w[2],caps(2))?serviced():failure();
        case 8:return put(w[2],mode())?serviced():failure();
        case 9:return put(w[2],std::array<uint32_t,4>{0,2,focus_,0x20})?serviced():failure();
        case 15:return present(w);
        case 16:if(w[2] || w[3])return unsupported();return surface_output(back_,w[4]);
        case 31: {
            const uint32_t color_handle=w[2]?w[2]:target_;
            auto* color=find(color_handle);auto* depth=w[3]?find(w[3]):nullptr;
            auto* color_data=color && color->parent?find(color->parent):color;
            auto* depth_data=depth && depth->parent?find(depth->parent):depth;
            if(!color || color->texture || !color_data || !(color_data->usage&1u) ||
                (color_data->format!=21 && color_data->format!=22) ||
                (w[3] && (!depth || depth->texture || !depth_data || depth_data->format!=80 ||
                 depth_data->width<color_data->width || depth_data->height<color_data->height))){value=0x8876086C;return serviced();}
            if(color_handle!=target_){auto* old=find(target_);if(!old || old->refs<2 || color->refs==0xFFFFFFFFu)return failure();++color->refs;--old->refs;target_=color_handle;}
            if(w[3]!=depth_){auto* old=depth_?find(depth_):nullptr;if((old && old->refs<2) || (depth && depth->refs==0xFFFFFFFFu))return failure();if(depth)++depth->refs;if(old)--old->refs;depth_=w[3];}
            viewport_={0,0,color_data->width,color_data->height,0,0x3F800000};
            fprintf(log_,"startup_d3d8_render_target=color:0x%08X depth:0x%08X dimensions:%ux%u viewport_reset:yes\n",target_,depth_,color_data->width,color_data->height);return serviced();
        }
        case 40: {
            std::array<uint32_t,6> v{};float minz=0,maxz=0;
            if(w[2]<0x10000 || !cpu_.read(w[2],v.data(),sizeof(v)))return failure();
            std::memcpy(&minz,&v[4],4);std::memcpy(&maxz,&v[5],4);
            auto* target=find(target_);auto* data=target && target->parent?find(target->parent):target;
            const uint32_t width=data?data->width:0,height=data?data->height:0;
            fprintf(log_,"startup_d3d8_viewport=request x:%u y:%u width:%u height:%u minz:%g maxz:%g recording:%s\n",v[0],v[1],v[2],v[3],double(minz),double(maxz),recording_on_?"yes":"no");
            if(!v[2] || !v[3] || uint64_t(v[0])+v[2]>width || uint64_t(v[1])+v[3]>height ||
                !std::isfinite(minz) || !std::isfinite(maxz) || minz<0 || maxz>1 || minz>maxz){value=0x8876086C;return serviced();}
            if(recording_on_){recording_.viewport=v;recording_.viewport_mask=true;}else viewport_=v;
            return serviced();
        }
        case 41:return put(w[2],viewport_)?serviced():failure();
        case 32:return surface_output(target_,w[2]);
        case 33:if(!depth_){if(!put(w[2],0u))return failure();value=0x88760866;return serviced();}return surface_output(depth_,w[2]);
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
        case 50:
            fprintf(log_,"startup_d3d8_render_state=state:%u value:0x%08X recording:%s\n",w[2],w[3],recording_on_?"yes":"no");
            if(w[2]>=render_.size() || !render_valid_[w[2]] || !render_state(w[2],w[3]))return unsupported();
            if(recording_on_){recording_.render[w[2]]=w[3];recording_.render_mask[w[2]]=true;}else render_[w[2]]=w[3];return serviced();
        case 51:if(w[2]>=render_.size() || !render_valid_[w[2]])return unsupported();return put(w[3],render_[w[2]])?serviced():failure();
        case SetTexture:
            if(w[2])return unsupported();
            if(!replace_texture(recording_on_?recording_.texture:bound_texture_,w[3]))return failure();
            if(recording_on_)recording_.texture_mask=true;
            fprintf(log_,"startup_d3d8_texture_binding=stage:0 object:0x%08X recording:%s\n",w[3],recording_on_?"yes":"no");return serviced();
        case GetTexture: {
            if(w[2])return unsupported();
            auto* texture=bound_texture_?find(bound_texture_):nullptr;
            if(bound_texture_ && (!texture || !texture->texture || texture->refs==0xFFFFFFFFu))return failure();
            if(!put(w[3],bound_texture_))return failure();
            if(texture)++texture->refs;
            fprintf(log_,"startup_d3d8_texture_get=stage:0 object:0x%08X addref:%s\n",bound_texture_,texture?"yes":"no");return serviced();
        }
        case 72:return draw(w);
        case 52:
            if(recording_on_){value=0x8876086C;return serviced();}
            recording_=StateBlock{};recording_on_=true;
            fprintf(log_,"startup_d3d8_stateblock=begin scope:render_stage_fvf_viewport_texture0\n");return serviced();
        case 53:
            if(!recording_on_){value=0x8876086C;return serviced();}
            if(state_block_count_>=state_blocks_.size())return unsupported();
            if(!put(w[2],state_block_count_+1))return failure();
            recording_.alive=true;state_blocks_[state_block_count_++]=recording_;recording_on_=false;
            fprintf(log_,"startup_d3d8_stateblock=end token:%u render:%u stage:%u fvf:%u\n",state_block_count_,
                unsigned(std::count(recording_.render_mask.begin(),recording_.render_mask.end(),true)),
                unsigned(std::count(recording_.stage_mask.begin(),recording_.stage_mask.end(),true)),recording_.fvf_mask?1:0);
            fprintf(log_,"startup_d3d8_stateblock_texture=token:%u mask:%u object:0x%08X ownership:transferred\n",state_block_count_,recording_.texture_mask?1:0,recording_.texture);
            recording_=StateBlock{};return serviced();
        case 54:case 55:case 56: {
            if(recording_on_ || !w[2] || w[2]>state_block_count_ || !state_blocks_[w[2]-1].alive){value=0x8876086C;return serviced();}
            auto& block=state_blocks_[w[2]-1];
            if(s==56){if(block.texture_mask && !replace_texture(block.texture,0u))return failure();block=StateBlock{};}
            else {
                if(block.texture_mask && !(s==54?replace_texture(bound_texture_,block.texture):replace_texture(block.texture,bound_texture_)))return failure();
                for(unsigned i=0;i<render_.size();++i)if(block.render_mask[i]){
                    if(s==54)render_[i]=block.render[i];else block.render[i]=render_[i];}
                for(unsigned i=0;i<stage_.size();++i)if(block.stage_mask[i]){
                    if(s==54){stage_[i]=block.stage[i];stage_valid_[i]=true;}else block.stage[i]=stage_[i];}
                if(block.fvf_mask){if(s==54)fvf_=block.fvf;else block.fvf=fvf_;}
                if(block.viewport_mask){if(s==54)viewport_=block.viewport;else block.viewport=viewport_;}
            }
            fprintf(log_,"startup_d3d8_stateblock=method:%s token:%u\n",method(device_trap+16*s),w[2]);return serviced();
        }
        case GetTextureStageState:
            if(w[2] || w[3]>=stage_.size() || !stage_valid_[w[3]])return unsupported();
            fprintf(log_,"startup_d3d8_stage_get=stage:%u type:%u output:0x%08X value:%u\n",w[2],w[3],w[4],stage_[w[3]]);
            return put(w[4],stage_[w[3]])?serviced():failure();
        case SetTextureStageState:
            if(w[2] || w[3]>=stage_.size() || !stage_state(w[3],w[4]))return unsupported();
            if(recording_on_){recording_.stage[w[3]]=w[4];recording_.stage_mask[w[3]]=true;}else{stage_[w[3]]=w[4];stage_valid_[w[3]]=true;}
            fprintf(log_,"startup_d3d8_stage_set=stage:%u type:%u value:%u output_write:none\n",w[2],w[3],w[4]);
            return serviced();
        case 76:if(w[2]!=0x144)return unsupported();if(recording_on_){recording_.fvf=w[2];recording_.fvf_mask=true;}else fvf_=w[2];return serviced();
        case 77:return put(w[2],fvf_)?serviced():failure();
        case 34:if(scene_)return failure();scene_=true;return serviced();
        case 35:if(!scene_)return failure();scene_=false;return serviced();
        case 36: {
            if(w[2] || w[3] || !w[4] || (w[4]&~3u))return unsupported();
            auto* target=find(target_);auto* color=target && target->parent?find(target->parent):target;
            auto* depth=find(depth_);
            if(!color || (color->format!=21 && color->format!=22) || ((w[4]&2) && !depth))return failure();
            float z=0;std::memcpy(&z,&w[6],4);if((w[4]&2) && !(z>=0 && z<=1))return failure();
            const uint64_t right=uint64_t(viewport_[0])+viewport_[2],bottom=uint64_t(viewport_[1])+viewport_[3];
            if(right>color->width || bottom>color->height || ((w[4]&2) && (right>depth->width || bottom>depth->height)))return failure();
            const uint16_t d=(w[4]&2)?uint16_t(z*65535.0f):0;
            const uint32_t clear_color=color->format==22?(w[5]|0xFF000000u):w[5];
            for(uint32_t y=viewport_[1];y<bottom;++y)for(uint32_t x=viewport_[0];x<right;++x){
                if(w[4]&1)std::memcpy(color->bytes.data()+size_t(y)*color->pitch+size_t(x)*4,&clear_color,4);
                if(w[4]&2)std::memcpy(depth->bytes.data()+size_t(y)*depth->pitch+size_t(x)*2,&d,2);
            }
            fprintf(log_,"startup_d3d8_clear=owned_storage flags:%u viewport:%u,%u,%u,%u pixels:%u\n",w[4],viewport_[0],viewport_[1],viewport_[2],viewport_[3],viewport_[2]*viewport_[3]);return serviced();
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
            if(data->bytes.size()>staging_size){fprintf(log_,"startup_d3d8_lock_error=staging_capacity_exceeded\n");return failure();}
            if(!cpu_.write(staging,data->bytes.data(),data->bytes.size())){fprintf(log_,"startup_d3d8_lock_error=staging_write_failed\n");return failure();}
            if(!put(output,std::array<uint32_t,2>{data->pitch,staging})){fprintf(log_,"startup_d3d8_lock_error=locked_rect_output_failed\n");return failure();}
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
    void initialize_states() {
        const uint32_t render[][2]={{7,1},{8,3},{9,2},{14,1},{15,0},{16,1},{19,2},{20,1},{22,3},{23,4},{24,0},{25,8},{26,0},{27,0},{28,0},{29,0},{60,0xFFFFFFFF},{136,1},{137,1},{168,15},{171,1}};
        const uint32_t stage[][2]={{1,4},{2,2},{3,1},{4,2},{5,2},{6,1},{11,0},{13,1},{14,1},{15,0},{16,1},{17,1},{18,0}};
        for(const auto& v:render){render_[v[0]]=v[1];render_valid_[v[0]]=true;}
        for(const auto& v:stage){stage_[v[0]]=v[1];stage_valid_[v[0]]=true;}
    }
    static bool render_state(uint32_t state,uint32_t value) {
        switch(state){
        case 7:case 14:case 15:case 16:case 26:case 27:case 28:case 29:case 136:case 137:return value<=1;
        case 8:case 9:case 22:return value>=1 && value<=3;
        case 19:case 20:return value>=1 && value<=13;
        case 23:case 25:return value>=1 && value<=8;
        case 24:return value<=255;case 60:return true;case 168:return value<=15;
        case 171:return value>=1 && value<=5;default:return false;}
    }
    static bool stage_state(uint32_t state,uint32_t value) {
        switch(state){case 1:case 4:return (value>=1 && value<=4) || value==7;
        case 2:case 3:case 5:case 6:return (value&~0x30u)<=3;
        case 11:return value==0; // TEXCOORDINDEX: first FVF coordinate set only.
        case 13:case 14:return value>=1 && value<=5;
        case 15:return true;
        // Preserve legacy filter enums used by the original helper (0/1/2/4).
        // This is state storage only; no sampling implementation is claimed.
        case 16:case 17:case 18:return value<=5;default:return false;}
    }
    bool replace_texture(uint32_t& current,uint32_t next) {
        auto* incoming=next?find(next):nullptr;auto* outgoing=current?find(current):nullptr;
        if((next && (!incoming || !incoming->texture)) || (current && (!outgoing || !outgoing->texture)))return false;
        if(current==next)return true;
        if((incoming && incoming->refs==0xFFFFFFFFu) || (outgoing && outgoing->refs<=1))return false;
        if(incoming)++incoming->refs;
        if(outgoing)--outgoing->refs;
        current=next;return true;
    }
    StartupServiceResult present(const uint32_t* w) {
        fprintf(log_,"startup_d3d8_present_request=source_rect:0x%08X destination_rect:0x%08X window:0x%08X dirty:0x%08X scene:%s\n",w[2],w[3],w[4],w[5],scene_?"yes":"no");
        if(scene_ || recording_on_ || w[5] || (w[4] && w[4]!=focus_))return unsupported();
        std::array<int32_t,4> source={0,0,640,480},destination=source;
        if((w[2] && (w[2]<0x10000 || !cpu_.read(w[2],source.data(),sizeof(source)))) ||
            (w[3] && (w[3]<0x10000 || !cpu_.read(w[3],destination.data(),sizeof(destination)))))return failure();
        fprintf(log_,"startup_d3d8_present_rect=source:%d,%d,%d,%d destination:%d,%d,%d,%d\n",source[0],source[1],source[2],source[3],destination[0],destination[1],destination[2],destination[3]);
        if(!d3d8_present::full_rect(source,640,480) || !d3d8_present::full_rect(destination,640,480))return unsupported();
        auto* back=find(back_);
        if(!back || back->locked || back->width!=640 || back->height!=480 || back->format!=22)return failure();
        if(present_calls_>=8){fprintf(log_,"startup_d3d8_present_scope_limit=frames:%u maximum:8\n",present_calls_);return unsupported();}
        const d3d8_quad::Image image={back->bytes.data(),back->bytes.size(),back->width,back->height,back->pitch,back->format};
        if(!presenter_.present(image))return failure();
        ++present_calls_;
        th075::capture_presented_exe_frame(back->bytes.data(),back->bytes.size(),back->width,back->height,back->pitch,present_calls_,log_);
        fprintf(log_,"startup_d3d8_present=executed frame:%u backbuffer:0x%08X\n",present_calls_,back_);
        return serviced();
    }
    StartupServiceResult draw(const uint32_t* w) {
        fprintf(log_,"startup_d3d8_draw_request=type:%u primitives:%u vertices:0x%08X stride:%u fvf:0x%08X texture:0x%08X target:0x%08X scene:%s\n",w[2],w[3],w[4],w[5],fvf_,bound_texture_,target_,scene_?"yes":"no");
        for(unsigned i=0;i<render_.size();++i)if(render_valid_[i])fprintf(log_,"startup_d3d8_draw_state=state:%u value:0x%08X\n",i,render_[i]);
        for(unsigned i=0;i<stage_.size();++i)if(stage_valid_[i])fprintf(log_,"startup_d3d8_draw_stage=type:%u value:0x%08X\n",i,stage_[i]);
        uint32_t count=0;
        if(w[3]<=16){switch(w[2]){case 1:count=w[3];break;case 2:count=w[3]*2;break;case 3:count=w[3]+1;break;case 4:count=w[3]*3;break;case 5:case 6:count=w[3]+2;break;}}
        if(fvf_==0x144 && w[5]==28 && count && w[4]>=0x10000 && uint64_t(w[4])+uint64_t(count)*28<=0x02000000){
            std::array<uint32_t,48*7> vertices{};
            if(!cpu_.read(w[4],vertices.data(),count*28))return failure();
            uint32_t hash=2166136261u;const auto* bytes=reinterpret_cast<const uint8_t*>(vertices.data());for(unsigned i=0;i<count*28;++i)hash=(hash^bytes[i])*16777619u;
            fprintf(log_,"startup_d3d8_draw_vertices=count:%u bytes:%u fnv1a:0x%08X layout:XYZ_RHW_ARGB_UV\n",count,count*28,hash);
            for(unsigned i=0;i<std::min(count,8u);++i){const auto* v=vertices.data()+i*7;float f[7]{};std::memcpy(f,v,28);
                fprintf(log_,"startup_d3d8_vertex=%u x:%g y:%g z:%g rhw:%g color:0x%08X u:%g v:%g\n",i,double(f[0]),double(f[1]),double(f[2]),double(f[3]),v[4],double(f[5]),double(f[6]));}
            const bool profile=scene_ && !recording_on_ && w[2]==5 && w[3]==2 &&
                render_[7]==0 && render_[8]==3 && render_[9]==2 && render_[22]==1 &&
                render_[26]==0 && render_[28]==0 && render_[29]==0 && render_[137]==0 && render_[168]==15 && render_[171]==1 &&
                (!render_[15] || render_[25]==7) &&
                (!render_[27] || (render_[19]==5 && render_[20]==6) || (render_[19]==2 && render_[20]==1)) &&
                stage_[1]==4 && stage_[2]==2 && (stage_[3]==0 || stage_[3]==1) &&
                stage_[4]==4 && stage_[5]==2 && (stage_[6]==0 || stage_[6]==1) &&
                stage_[11]==0 && stage_[13]==1 && stage_[14]==1 && stage_[16]==1 && stage_[17]==1 && stage_[18]<=1;
            auto* texture=find(bound_texture_);auto* surface=find(target_);
            auto* target=surface && surface->parent?find(surface->parent):surface;
            if(profile && texture && texture->texture && target && !texture->locked && !target->locked){
                const uint64_t pixels=uint64_t(viewport_[2])*viewport_[3];
                if(draw_calls_>=64 || pixels>16u*1024u*1024u-draw_pixels_){
                    fprintf(log_,"startup_d3d8_draw_scope_limit=calls:%u covered:%u max_calls:64 max_pixels:16777216\n",draw_calls_,draw_pixels_);return unsupported();
                }
                d3d8_quad::Vertex quad[4]{};std::memcpy(quad,vertices.data(),sizeof(quad));
                const d3d8_quad::Image image={texture->bytes.data(),texture->bytes.size(),texture->width,texture->height,texture->pitch,texture->format};
                const d3d8_quad::Target output={target->bytes.data(),target->bytes.size(),target->width,target->height,target->pitch,target->format};
                const d3d8_quad::Viewport view={viewport_[0],viewport_[1],viewport_[2],viewport_[3]};
                const d3d8_quad::Settings settings={render_[24],render_[15]!=0,render_[27]!=0,render_[19]==2 && render_[20]==1};
                d3d8_quad::Stats stats;
                if(d3d8_quad::rasterize(quad,image,output,view,settings,stats)==d3d8_quad::Result::Rendered){
                    ++draw_calls_;draw_pixels_+=stats.covered;
                    fprintf(log_,"startup_d3d8_draw=executed renderer:point_quad call:%u covered:%u alpha_rejected:%u written:%u changed:%u\n",draw_calls_,stats.covered,stats.alpha_rejected,stats.written,stats.changed);
                    fprintf(log_,"startup_d3d8_draw_blend=mode:%s\n",!render_[27]?"disabled":settings.replace_blend?"one_zero":"source_alpha");
                    fprintf(log_,"startup_d3d8_draw_hash=source:0x%08X before:0x%08X after:0x%08X scope:covered_pixels\n",stats.hash_source,stats.hash_before,stats.hash_after);
                    const auto probe=[&](const char* name,const d3d8_quad::Probe& p){fprintf(log_,"startup_d3d8_draw_probe=%s xy:%u,%u texel:%u,%u source:0x%08X before:0x%08X after:0x%08X\n",name,p.x,p.y,p.texel_x,p.texel_y,p.source,p.before,p.after);};
                    if(stats.covered){probe("first",stats.first);probe("last",stats.last);}
                    if(stats.changed)th075::capture_first_exe_quad(target->bytes.data(),target->bytes.size(),target->width,target->height,target->pitch,view.x,view.y,view.width,view.height,log_);
                    return serviced();
                }
                fprintf(log_,"startup_d3d8_draw_rejected=reason:%s output_write:none\n",d3d8_quad::reject_name(stats.reject));
            }else fprintf(log_,"startup_d3d8_draw_rejected=state_or_resource_profile output_write:none\n");
        }
        fprintf(log_,"startup_d3d8_draw_boundary=executed:no\n");
        return unsupported();
    }
    d2rt::Cpu& cpu_;FILE* log_;uint32_t& root_refs_;
    ExePresenter presenter_;uint32_t present_calls_=0;
    struct StateBlock {
        std::array<uint32_t,256> render{};std::array<uint32_t,32> stage{};
        std::array<bool,256> render_mask{};std::array<bool,32> stage_mask{};
        std::array<uint32_t,6> viewport{};bool viewport_mask=false;
        uint32_t texture=0;bool texture_mask=false;
        uint32_t fvf=0;bool fvf_mask=false,alive=false;
    };
    std::array<StateBlock,64> state_blocks_{};StateBlock recording_{};
    unsigned state_block_count_=0;bool recording_on_=false;
    std::array<Resource,capacity> resources_{};
    std::array<uint32_t,256> render_{};std::array<uint32_t,32> stage_{};
    std::array<bool,32> stage_valid_{};std::array<bool,256> render_valid_{};
    uint32_t bound_texture_=0;
    uint32_t draw_calls_=0,draw_pixels_=0;
    std::array<uint32_t,6> viewport_={0,0,640,480,0,0x3F800000};
    uint32_t refs_=0,used_=0,count_=0,back_=0,depth_=0,target_=0,focus_=0,locked_=0,fvf_=0;
    bool scene_=false;
};
