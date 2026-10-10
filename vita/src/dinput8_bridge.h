#pragma once
#include "startup_services.h"
#include "runtime/cpu.h"
#include <psp2/ctrl.h>
#include <array>
#include <cstring>
#include <limits>

// New TH075-specific implementation. See REFERENCE_REPENTOGXM.md for prior
// art reviewed; no Isaac source, addresses or translated game code is copied.
class DirectInput8Bridge {
public:
    static constexpr uint32_t root=0x00AD0000, root_table=root+0x100,
        keyboard=root+0x200, keyboard_table=root+0x300,
        root_trap=0x00BF8000, keyboard_trap=0x00BF7000;
    DirectInput8Bridge(d2rt::Cpu& cpu,FILE* log):cpu_(cpu),log_(log) {}
    static bool range(uint32_t t,uint32_t b,unsigned n){return t>=b && t<b+n*16 && (t-b)%16==0;}
    bool owns(uint32_t t) const {return range(t,root_trap,11)||range(t,keyboard_trap,32);}
    const char* interface_name(uint32_t t) const {return range(t,root_trap,11)?"IDirectInput8A":"IDirectInputDevice8A";}
    const char* method_name(uint32_t t) const {
        if(range(t,root_trap,11)) {
            static const char* n[]={"QueryInterface","AddRef","Release","CreateDevice","EnumDevices","GetDeviceStatus","RunControlPanel","Initialize","FindDevice","EnumDevicesBySemantics","ConfigureDevices"};return n[(t-root_trap)/16];
        }
        if(!range(t,keyboard_trap,32))return "Unknown";
        static const char* n[]={"QueryInterface","AddRef","Release","GetCapabilities","EnumObjects","GetProperty","SetProperty","Acquire","Unacquire","GetDeviceState","GetDeviceData","SetDataFormat","SetEventNotification","SetCooperativeLevel","GetObjectInfo","GetDeviceInfo","RunControlPanel","Initialize","CreateEffect","EnumEffects","GetEffectInfo","GetForceFeedbackState","SendForceFeedbackCommand","EnumCreatedEffectObjects","Escape","Poll","SendDeviceData","EnumEffectsInFile","WriteEffectToFile","BuildActionMap","SetActionMap","GetImageInfo"};
        return n[(t-keyboard_trap)/16];
    }
    unsigned serviced_calls() const {return calls_;}
    StartupServiceResult create() {
        uint32_t w[6]{};const uint32_t esp=cpu_.reg(d2rt::R_ESP);
        if(!frame(esp,sizeof(w)) || !cpu_.read(esp,w,sizeof(w)))return failure();
        fprintf(log_,"startup_dinput_create=instance:0x%08X version:0x%08X iid:0x%08X output:0x%08X aggregate:0x%08X\n",w[1],w[2],w[3],w[4],w[5]);
        Guid iid{};if(!read_guid(w[3],iid) || !output(w[4],0u))return failure();
        if(w[1]!=0x00400000 || w[2]!=0x800 || w[5] || iid!=input_iid || root_refs_)return unsupported();
        // Native nonblocking pad capture is the sole input producer.
        const int rc=sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
        fprintf(log_,"startup_dinput_sampling_rc=0x%08X\n",unsigned(rc));
        if(rc<0)return failure();
        if(!cpu_.map(root,0x1000,nullptr,d2rt::P_RW) || !table(root_table,root_trap,11) ||
            !table(keyboard_table,keyboard_trap,32) || !output(root,root_table))return failure();
        root_refs_=1;
        if(!sample())return failure();
        if(!output(w[4],root))return failure();
        fprintf(log_,"startup_dinput_root=owned object=0x%08X vtable_readback=passed\n",root);
        fprintf(log_,"startup_dinput_scope=native_pad_snapshot_keyboard_adapter joystick_callbacks_pending\n");
        fprintf(log_,"startup_dinput_bindings=dpad_left_stick:arrows cross:Z circle:X square:C triangle:A start:Return select:Escape L:LeftShift R:Space\n");
        return finish(0,sizeof(w),w[0],esp);
    }
    StartupServiceResult call(uint32_t t) {
        if(!owns(t))return unsupported();
        const bool is_root=range(t,root_trap,11);
        const unsigned slot=(t-(is_root?root_trap:keyboard_trap))/16;
        // Counts include `this`; unimplemented methods preserve their frame.
        const unsigned root_args[]={3,1,1,4,5,2,3,3,4,6,5};
        unsigned argc=is_root?root_args[slot]:0;
        if(!is_root) {switch(slot){case 0:argc=3;break;case 1:case 2:case 7:case 8:case 25:argc=1;break;
            case 9:case 13:argc=3;break;case 11:argc=2;break;default:break;}}
        fprintf(log_,"startup_dinput_method=%s::%s slot=%u\n",interface_name(t),method_name(t),slot);
        if(!argc)return unsupported();
        uint32_t w[7]{};const uint32_t esp=cpu_.reg(d2rt::R_ESP),cleanup=(argc+1)*4;
        if(!frame(esp,cleanup) || !cpu_.read(esp,w,cleanup))return failure();
        const uint32_t obj=is_root?root:keyboard,vt=is_root?root_table:keyboard_table;
        uint32_t actual=0;uint32_t& refs=is_root?root_refs_:keyboard_refs_;
        if(!refs || w[1]!=obj || !cpu_.read(obj,&actual,4) || actual!=vt)return failure();
        for(unsigned i=1;i<=argc;++i)fprintf(log_,"startup_dinput_arg%u=0x%08X\n",i-1,w[i]);
        uint32_t hr=0;
        if(slot==0) {
            Guid iid{};if(!read_guid(w[2],iid))return failure();
            const bool match=iid==unknown_iid || iid==(is_root?input_iid:device_iid);
            if(match && refs==std::numeric_limits<uint32_t>::max())return failure();
            if(!output(w[3],match?obj:0u))return failure();
            if(match)++refs;else hr=0x80004002;
        } else if(slot==1) {
            if(refs==std::numeric_limits<uint32_t>::max())return failure();
            hr=++refs;
        } else if(slot==2) {
            hr=--refs;
            if(!refs) {
                if(!output(obj,0u))return failure();
                if(!is_root){acquired_=false;configured_=false;cooperative_=false;snapshot_pending_=false;if(!root_refs_)return failure();--root_refs_;}
            }
        } else if(is_root) {
            if(slot==3) {
                Guid guid{};if(!read_guid(w[2],guid) || !output(w[3],0u))return failure();
                if(guid!=keyboard_guid || w[4] || keyboard_refs_)return unsupported();
                if(!output(keyboard,keyboard_table) || !output(w[3],keyboard))return failure();
                keyboard_refs_=1;++root_refs_;
                fprintf(log_,"startup_dinput_keyboard=owned object=0x%08X\n",keyboard);
            } else if(slot==5) {
                Guid guid{};if(!read_guid(w[2],guid))return failure();
                if(guid!=keyboard_guid)return unsupported();
                if(!sample())return failure();
            } else if(slot==4) {
                // This backend exposes a keyboard, not a DirectInput joystick.
                // A GAMECTRL-only query therefore has no matching devices.
                // Other filters still require real guest callback delivery.
                if(w[2]!=4 || w[5]!=1 || w[3]<0x00401000 || w[3]>=0x00690000)return unsupported();
                fprintf(log_,"startup_dinput_enumeration=gamectrl attached_only matching_devices:0 backend:keyboard_native_pad no_callback\n");
            } else return unsupported();
        } else {
            if(slot==11) {
                if(acquired_)return unsupported();
                std::array<uint32_t,6> df{};
                if(!w[2] || !cpu_.read(w[2],df.data(),sizeof(df)))return failure();
                fprintf(log_,"startup_dinput_format=size:%u object_size:%u flags:%u bytes:%u objects:%u\n",df[0],df[1],df[2],df[3],df[4]);
                if(df[0]!=24 || df[1]!=16 || df[2]!=2 || df[3]!=256 || df[4]!=256)return unsupported();
                std::array<uint32_t,4*256> objects{};
                if(!df[5] || !cpu_.read(df[5],objects.data(),sizeof(objects)))return failure();
                for(unsigned i=0;i<256;++i) {
                    Guid key{};const auto* a=objects.data()+4*i;
                    if(!read_guid(a[0],key))return failure();
                    if(key!=key_guid || a[1]!=i || a[2]!=(0x8000000Cu|(i<<8)) || a[3])return unsupported();
                }
                configured_=true;fprintf(log_,"startup_dinput_keyboard_format=all_256_objects_validated\n");
            } else if(slot==13) {
                if(w[2]!=0x00AB7000 || w[3]!=0x16 || acquired_)return unsupported();
                cooperative_=true;fprintf(log_,"startup_dinput_cooperative=owned_window_nonexclusive_background_nowinkey\n");
            } else if(slot==7) {
                if(!configured_ || !cooperative_)hr=0x80070057;
                else {if(!sample())return failure();acquired_=true;snapshot_pending_=true;}
            } else if(slot==8) {acquired_=false;snapshot_pending_=false;}
            else if(slot==9) {
                if(w[2]!=256)return unsupported();
                if(!acquired_)hr=0x8007000C; // DIERR_NOTACQUIRED
                else {
                    if((!snapshot_pending_ && !sample()) || !output(w[3],keys_))return failure();
                    snapshot_pending_=false;
                    fprintf(log_,"startup_dinput_keyboard_state=bytes:256 snapshot:%u native_buttons:0x%08X analog:%u,%u\n",samples_,pad_.buttons,pad_.lx,pad_.ly);
                    for(unsigned key=0;key<keys_.size();++key)if(keys_[key]!=delivered_[key])
                        fprintf(log_,"startup_dinput_key=scan:0x%02X down:%u snapshot:%u\n",key,keys_[key]?1u:0u,samples_);
                    delivered_=keys_;
                }
            } else if(slot==25) {
                if(!acquired_)hr=0x8007000C;
                else {if(!sample())return failure();snapshot_pending_=true;}
            } else return unsupported();
        }
        return finish(hr,cleanup,w[0],esp);
    }
private:
    using Guid=std::array<uint32_t,4>;
    inline static constexpr Guid input_iid={0xBF798030,0x4DA2483A,0x645D99AA,0x009736ED};
    inline static constexpr Guid device_iid={0x54D41080,0x4833DC15,0x8F741BA4,0x7981A373};
    inline static constexpr Guid unknown_iid={0,0,0x000000C0,0x46000000};
    inline static constexpr Guid keyboard_guid={0x6F1D2B61,0x11CFD5A0,0x4544C7BF,0x00005453};
    inline static constexpr Guid key_guid={0x55728220,0x11CFD33C,0x4544C7BF,0x00005453};
    static bool frame(uint32_t esp,uint32_t bytes){return esp>=0x00800000 && uint64_t(esp)+bytes<=0x00A00000;}
    bool read_guid(uint32_t p,Guid& guid){return p>=0x10000 && cpu_.read(p,guid.data(),sizeof(guid));}
    template<class T> bool output(uint32_t p,const T& v){return p>=0x10000 && uint64_t(p)+sizeof(v)<=0x02000000 && cpu_.write(p,&v,sizeof(v));}
    bool table(uint32_t p,uint32_t traps,unsigned count) {
        std::array<uint32_t,32> a{},b{};for(unsigned i=0;i<count;++i)a[i]=traps+i*16;
        return cpu_.write(p,a.data(),count*4) && cpu_.read(p,b.data(),count*4) && a==b;
    }
    bool sample() {
        SceCtrlData next{};const int rc=sceCtrlPeekBufferPositive(0,&next,1);
        if(rc!=1) {keys_.fill(0);pad_={};fprintf(log_,"startup_dinput_sample_error=rc:0x%08X neutralized:yes\n",unsigned(rc));return false;}
        pad_=next;++samples_;keys_.fill(0);
        auto bind=[&](bool held,unsigned key){if(held)keys_[key]=0x80;};
        bind((pad_.buttons&SCE_CTRL_UP)||pad_.ly<64,0xC8);
        bind((pad_.buttons&SCE_CTRL_DOWN)||pad_.ly>192,0xD0);
        bind((pad_.buttons&SCE_CTRL_LEFT)||pad_.lx<64,0xCB);
        bind((pad_.buttons&SCE_CTRL_RIGHT)||pad_.lx>192,0xCD);
        bind(pad_.buttons&SCE_CTRL_CROSS,0x2C);bind(pad_.buttons&SCE_CTRL_CIRCLE,0x2D);
        bind(pad_.buttons&SCE_CTRL_SQUARE,0x2E);bind(pad_.buttons&SCE_CTRL_TRIANGLE,0x1E);
        bind(pad_.buttons&SCE_CTRL_START,0x1C);bind(pad_.buttons&SCE_CTRL_SELECT,0x01);
        bind(pad_.buttons&SCE_CTRL_LTRIGGER,0x2A);bind(pad_.buttons&SCE_CTRL_RTRIGGER,0x39);
        if(samples_==1)fprintf(log_,"startup_dinput_native_sample=passed buttons:0x%08X analog:%u,%u\n",pad_.buttons,pad_.lx,pad_.ly);
        return true;
    }
    StartupServiceResult finish(uint32_t hr,uint32_t cleanup,uint32_t ret,uint32_t esp) {
        const int regs[]={d2rt::R_EBX,d2rt::R_EBP,d2rt::R_ESI,d2rt::R_EDI};std::array<uint32_t,4> before{};
        for(unsigned i=0;i<4;++i)before[i]=cpu_.reg(regs[i]);
        cpu_.trap_epilogue(hr,cleanup,ret);
        bool good=cpu_.reg(d2rt::R_EAX)==hr && cpu_.reg(d2rt::R_ESP)==esp+cleanup && cpu_.reg(d2rt::R_EIP)==ret;
        for(unsigned i=0;i<4;++i)good=good && before[i]==cpu_.reg(regs[i]);
        fprintf(log_,"startup_dinput_abi=%s cleanup=%u return=0x%08X hresult=0x%08X\n",good?"passed":"failed",cleanup,ret,hr);
        if(good)++calls_;
        return good?StartupServiceResult::Serviced:failure();
    }
    static StartupServiceResult failure(){return StartupServiceResult::ContractFailure;}
    StartupServiceResult unsupported(){fprintf(log_,"startup_dinput_method_executed=no\n");return StartupServiceResult::Unsupported;}
    d2rt::Cpu& cpu_;FILE* log_;uint32_t root_refs_=0,keyboard_refs_=0,samples_=0;unsigned calls_=0;
    bool configured_=false,cooperative_=false,acquired_=false,snapshot_pending_=false;
    SceCtrlData pad_{};std::array<uint8_t,256> keys_{},delivered_{};
};
