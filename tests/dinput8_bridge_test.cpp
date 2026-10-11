// Shipping keyboard COM bridge exercised with synthetic native pad snapshots.
#include "../vita/src/dinput8_bridge.h"
#include <cassert>
#include <initializer_list>
#include <string>
#include <vector>

namespace pad_stub {SceCtrlData pad{};unsigned peeks=0;int result=1;}
int sceCtrlSetSamplingMode(SceCtrlPadInputMode mode){assert(mode==SCE_CTRL_MODE_ANALOG_WIDE);return 0;}
int sceCtrlPeekBufferPositive(int port,SceCtrlData* pad,int count){
    assert(port==0 && count==1);++pad_stub::peeks;
    if(pad_stub::result==1)*pad=pad_stub::pad;
    return pad_stub::result;
}
class HostCpu final:public d2rt::Cpu {
public:
    std::vector<uint8_t> bytes=std::vector<uint8_t>(0x02000000);
    std::array<uint32_t,d2rt::R_COUNT> registers{};
    bool map(uint32_t a,uint32_t n,const void* p,int)override{if(!hostptr(a,n))return false;if(p)std::memcpy(hostptr(a,n),p,n);return true;}
    bool protect(uint32_t a,uint32_t n,int)override{return hostptr(a,n)!=nullptr;}
    bool read(uint32_t a,void* p,uint32_t n)override{if(!hostptr(a,n))return false;std::memcpy(p,hostptr(a,n),n);return true;}
    bool write(uint32_t a,const void* p,uint32_t n)override{if(!hostptr(a,n))return false;std::memcpy(hostptr(a,n),p,n);return true;}
    void* hostptr(uint32_t a,uint32_t n=1)override{return uint64_t(a)+n<=bytes.size()?bytes.data()+a:nullptr;}
    uint32_t reg(int r)override{return registers[r];}
    void set_reg(int r,uint32_t v)override{registers[r]=v;}
    void set_trap(uint32_t,uint32_t,d2rt::TrapFn)override{assert(false);}
    bool run(uint32_t,const char**)override{assert(false);return false;}
    uint32_t fault_addr()const override{assert(false);return 0;}
};
struct Fixture {
    static constexpr uint32_t out=0x00900000,stack=0x009F0000,ret=0x0040710E;
    HostCpu cpu;FILE* log=std::tmpfile();DirectInput8Bridge input{cpu,log};
    Fixture(bool acquire=true){
        assert(log);pad_stub::pad={};pad_stub::result=1;
        const uint32_t iid[]={0xBF798030,0x4DA2483A,0x645D99AA,0x009736ED},
            guid[]={0x6F1D2B61,0x11CFD5A0,0x4544C7BF,0x00005453},
            key[]={0x55728220,0x11CFD33C,0x4544C7BF,0x00005453};
        assert(cpu.write(out+0x100,iid,16) && cpu.write(out+0x120,guid,16) && cpu.write(out+0x140,key,16));
        const uint32_t frame[]={ret,0x00400000,0x800,out+0x100,out,0};
        assert(cpu.write(stack,frame,sizeof(frame)));cpu.set_reg(d2rt::R_ESP,stack);
        assert(input.create()==StartupServiceResult::Serviced && cpu.read_u32(out)==DirectInput8Bridge::root);
        assert(call(true,3,{out+0x120,out,0})==0 && cpu.read_u32(out)==DirectInput8Bridge::keyboard);
        const uint32_t df[]={24,16,2,256,256,out+0x2000};
        assert(cpu.write(out+0x180,df,sizeof(df)));
        for(unsigned i=0;i<256;++i){const uint32_t obj[]={out+0x140,i,0x8000000Cu|(i<<8),0};assert(cpu.write(out+0x2000+i*16,obj,sizeof(obj)));}
        assert(call(false,11,{out+0x180})==0 && call(false,13,{0x00AB7000,0x16})==0);
        if(acquire)assert(call(false,7,{})==0);
    }
    ~Fixture(){assert(call(false,2,{})==0 && call(true,2,{})==0);std::fclose(log);}
    uint32_t call(bool root,unsigned slot,std::initializer_list<uint32_t> args,
        StartupServiceResult expected=StartupServiceResult::Serviced){
        std::vector<uint32_t> words{ret,root?DirectInput8Bridge::root:DirectInput8Bridge::keyboard};
        words.insert(words.end(),args.begin(),args.end());assert(cpu.write(stack,words.data(),uint32_t(words.size()*4)));
        const auto trap=(root?DirectInput8Bridge::root_trap:DirectInput8Bridge::keyboard_trap)+slot*16;
        cpu.set_reg(d2rt::R_ESP,stack);cpu.set_reg(d2rt::R_EIP,trap);
        for(int r:{d2rt::R_EBX,d2rt::R_EBP,d2rt::R_ESI,d2rt::R_EDI})cpu.set_reg(r,0x12340000u+r);
        assert(input.call(trap)==expected);
        assert(cpu.reg(d2rt::R_ESP)==(expected==StartupServiceResult::Serviced?stack+words.size()*4:stack));
        assert(cpu.reg(d2rt::R_EIP)==(expected==StartupServiceResult::Serviced?ret:trap));
        for(int r:{d2rt::R_EBX,d2rt::R_EBP,d2rt::R_ESI,d2rt::R_EDI})assert(cpu.reg(r)==0x12340000u+unsigned(r));
        return cpu.reg(d2rt::R_EAX);
    }
    std::array<uint8_t,256> snapshot(){
        assert(call(false,25,{})==0 && call(false,9,{256,out+0x400})==0);
        std::array<uint8_t,256> keys{};assert(cpu.read(out+0x400,keys.data(),keys.size()));return keys;
    }
    std::string trace(){std::fflush(log);std::rewind(log);std::string s;char line[512];while(std::fgets(line,sizeof(line),log))s+=line;std::fseek(log,0,SEEK_END);return s;}
};
int main(){
    {
        Fixture f;
        for(const auto& binding:std::initializer_list<std::array<unsigned,2>>{
            {SCE_CTRL_UP,0xC8},{SCE_CTRL_DOWN,0xD0},{SCE_CTRL_LEFT,0xCB},{SCE_CTRL_RIGHT,0xCD},
            {SCE_CTRL_CROSS,0x2C},{SCE_CTRL_CIRCLE,0x2D},{SCE_CTRL_SQUARE,0x2E},{SCE_CTRL_TRIANGLE,0x1E},
            {SCE_CTRL_START,0x1C},{SCE_CTRL_SELECT,0x01},{SCE_CTRL_LTRIGGER,0x2A},{SCE_CTRL_RTRIGGER,0x39}}){
            pad_stub::pad.buttons=binding[0];const auto keys=f.snapshot();
            for(unsigned i=0;i<keys.size();++i)assert(keys[i]==(i==binding[1]?0x80:0));
        }
        puts("PASS twelve digital Vita bindings reach exactly the shipping DirectInput keyboard bytes");
    }
    {
        Fixture f;pad_stub::pad.lx=64;pad_stub::pad.ly=192;
        for(auto key:f.snapshot())assert(key==0);
        pad_stub::pad.lx=63;pad_stub::pad.ly=193;auto keys=f.snapshot();
        assert(keys[0xCB]==0x80 && keys[0xD0]==0x80);
        pad_stub::pad.lx=193;pad_stub::pad.ly=63;keys=f.snapshot();
        assert(keys[0xCD]==0x80 && keys[0xC8]==0x80 && !keys[0xCB] && !keys[0xD0]);
        pad_stub::pad={};pad_stub::pad.buttons=0x10010000;
        for(auto key:f.snapshot())assert(key==0);
        puts("PASS left-stick thresholds, diagonals, release and unrelated button bits");
    }
    {
        Fixture f;pad_stub::pad.buttons=SCE_CTRL_CROSS;assert(f.call(false,25,{})==0);
        const auto peeks=pad_stub::peeks;pad_stub::pad.buttons=0;
        assert(f.call(false,9,{256,Fixture::out+0x400})==0 && pad_stub::peeks==peeks);
        assert(f.cpu.bytes[Fixture::out+0x400+0x2C]==0x80);
        assert(f.call(false,9,{256,Fixture::out+0x400})==0 && pad_stub::peeks==peeks+1);
        assert(f.cpu.bytes[Fixture::out+0x400+0x2C]==0);
        puts("PASS Poll snapshot is delivered once before a fresh native sample and key release");
    }
    {
        Fixture f(false);f.cpu.bytes[Fixture::out+0x2000+8]^=1;
        f.call(false,11,{Fixture::out+0x180},StartupServiceResult::Unsupported);
        f.cpu.bytes[Fixture::out+0x2000+8]^=1;
        assert(f.call(false,11,{Fixture::out+0x180})==0 && f.call(false,7,{})==0);
        f.call(false,11,{Fixture::out+0x180},StartupServiceResult::Unsupported);
        puts("PASS malformed key-object formats and format changes while acquired remain boundaries");
    }
    {
        Fixture f(false);std::memset(f.cpu.hostptr(Fixture::out+0x400,256),0xA5,256);
        assert(f.call(false,9,{256,Fixture::out+0x400})==0x8007000C);
        f.call(false,9,{255,Fixture::out+0x400},StartupServiceResult::Unsupported);
        for(unsigned i=0;i<256;++i)assert(f.cpu.bytes[Fixture::out+0x400+i]==0xA5);
        assert(f.call(false,7,{})==0 && f.call(false,8,{})==0 && f.call(false,25,{})==0x8007000C);
        puts("PASS unacquired keyboard and unsupported state size retain guest output and COM ABI");
    }
    {
        Fixture f;std::memset(f.cpu.hostptr(Fixture::out+0x400,256),0xA5,256);
        for(int rc:{0,-123}){pad_stub::result=rc;f.call(false,25,{},StartupServiceResult::ContractFailure);}
        for(unsigned i=0;i<256;++i)assert(f.cpu.bytes[Fixture::out+0x400+i]==0xA5);
        pad_stub::result=1;puts("PASS native sampling failure refuses the call without consuming its frame or output");
    }
    {
        Fixture f;
        assert(f.call(true,4,{4,0x00401000,0,1})==0);
        f.call(true,4,{1,0x00401000,0,1},StartupServiceResult::Unsupported);
        assert(f.call(false,1,{})==2 && f.call(false,2,{})==1);
        puts("PASS keyboard-only enumeration boundaries and live device references remain intact");
    }
    {
        Fixture f;pad_stub::pad.buttons=SCE_CTRL_CROSS;f.snapshot();pad_stub::pad.buttons=0;f.snapshot();
        const auto trace=f.trace();assert(trace.find("startup_dinput_bindings=")!=std::string::npos);
        assert(trace.find("scan:0x2C down:1")!=std::string::npos && trace.find("scan:0x2C down:0")!=std::string::npos);
        puts("PASS delivered key edges and configured mapping logged; menu and combat responses require Vita");
    }
    {
        Fixture f;
        for(uint32_t trap:{0u,DirectInput8Bridge::root_trap+11*16,DirectInput8Bridge::keyboard_trap+32*16,DirectInput8Bridge::root_trap+1}){
            f.cpu.set_reg(d2rt::R_ESP,Fixture::stack);f.cpu.set_reg(d2rt::R_EIP,trap);
            assert(!f.input.owns(trap) && std::strcmp(f.input.method_name(trap),"Unknown")==0);
            assert(f.input.call(trap)==StartupServiceResult::Unsupported);
            assert(f.cpu.reg(d2rt::R_ESP)==Fixture::stack && f.cpu.reg(d2rt::R_EIP)==trap);
        }
        puts("PASS unowned and unaligned traps cannot index method tables or consume a frame");
    }
}
