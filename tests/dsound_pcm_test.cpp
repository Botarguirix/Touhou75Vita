// Exercise the shipping bridge with synthetic PCM and COM frames. Port setup
// is simulated; no game binary, Vita SDK, mixer or playback is exercised.
#include "../vita/src/dsound_bootstrap.h"
#include <cassert>
#include <cstring>
#include <initializer_list>

static unsigned opened=0,closed=0;
int sceAudioOutOpenPort(int type,int len,int hz,int mode){
    assert(type==0 && len==1024 && hz==48000 && mode==1);++opened;return 7;
}
int sceAudioOutGetConfig(int port,int type){
    assert(port==7);return type==0?1024:type==1?48000:1;
}
int sceAudioOutReleasePort(int port){assert(port==7);++closed;return 0;}

class HostCpu final:public d2rt::Cpu {
public:
    std::vector<uint8_t> bytes=std::vector<uint8_t>(0x02000000);
    std::array<uint32_t,d2rt::R_COUNT> registers{};
    uint32_t format_begin=0,format_end=0;unsigned format_reads=0;
    bool map(uint32_t va,uint32_t size,const void* init,int) override {
        if(!hostptr(va,size))return false;
        if(init)std::memcpy(bytes.data()+va,init,size);
        return true;
    }
    bool protect(uint32_t va,uint32_t size,int) override{return hostptr(va,size)!=nullptr;}
    bool read(uint32_t va,void* out,uint32_t size) override {
        if(format_begin && va>=format_begin && va<format_end){
            ++format_reads;if(uint64_t(va)+size>format_end)return false;
        }
        if(!hostptr(va,size))return false;
        std::memcpy(out,bytes.data()+va,size);return true;
    }
    bool write(uint32_t va,const void* in,uint32_t size) override {
        if(!hostptr(va,size))return false;
        std::memcpy(bytes.data()+va,in,size);return true;
    }
    void* hostptr(uint32_t va,uint32_t size=1) override {
        return uint64_t(va)+size<=bytes.size()?bytes.data()+va:nullptr;
    }
    uint32_t reg(int r) override{return registers[r];}
    void set_reg(int r,uint32_t v) override{registers[r]=v;}
    void set_trap(uint32_t,uint32_t,d2rt::TrapFn) override{assert(false);}
    bool run(uint32_t,const char**) override{assert(false);return false;}
    uint32_t fault_addr() const override{assert(false);return 0;}
};

struct Fixture {
    static constexpr uint32_t stack=0x009F0000,out=0x00900000,desc=out+0x100,
        format=out+0x200,guid=out+0x300,ret=0x00406A9B;
    HostCpu cpu;FILE* log=std::tmpfile();DirectSoundBootstrap sound{cpu,log};
    Fixture(){
        assert(log);
        const uint32_t clsid[]={0x3901CC3F,0x4FA484B5,0x81AA35BA,0x9BA0B872};
        const uint32_t iid[]={0xC50A7E93,0x4834F395,0xA97FF69E,0x6609E59D};
        assert(cpu.write(guid,clsid,16) && cpu.write(guid+16,iid,16));
        const uint32_t w[]={ret,guid,0,3,guid+16,out};
        assert(cpu.write(stack,w,sizeof(w)));cpu.set_reg(d2rt::R_ESP,stack);
        assert(sound.create(true)==StartupServiceResult::Serviced);
        assert(call(DirectSoundBootstrap::trap_base,10,{DirectSoundBootstrap::object,0})==0);
        assert(call(DirectSoundBootstrap::trap_base,6,{DirectSoundBootstrap::object,0xAB7000,2})==0);
    }
    ~Fixture(){assert(call(DirectSoundBootstrap::trap_base,2,{DirectSoundBootstrap::object})==0);std::fclose(log);}
    uint32_t call(uint32_t base,unsigned slot,std::initializer_list<uint32_t> args,
        StartupServiceResult expected=StartupServiceResult::Serviced){
        std::vector<uint32_t> w{ret};w.insert(w.end(),args.begin(),args.end());
        assert(cpu.write(stack,w.data(),uint32_t(w.size()*4)));
        const uint32_t trap=base+slot*16;cpu.set_reg(d2rt::R_ESP,stack);cpu.set_reg(d2rt::R_EIP,trap);
        const int kept[]={d2rt::R_EBX,d2rt::R_EBP,d2rt::R_ESI,d2rt::R_EDI};
        for(int r:kept)cpu.set_reg(r,0x11220000u+uint32_t(r));
        const auto result=sound.call(trap);
        if(result!=expected)std::fprintf(stderr,"Contract mismatch: %s expected:%d actual:%d\n",sound.method_name(trap),int(expected),int(result));
        assert(result==expected);
        for(int r:kept)assert(cpu.reg(r)==0x11220000u+uint32_t(r));
        assert(cpu.reg(d2rt::R_ESP)==(expected==StartupServiceResult::Serviced?stack+w.size()*4:stack));
        assert(cpu.reg(d2rt::R_EIP)==(expected==StartupServiceResult::Serviced?ret:trap));
        return cpu.reg(d2rt::R_EAX);
    }
    void pcm(uint32_t p,unsigned channels=2,unsigned bits=16,unsigned hz=44100){
        const uint32_t align=channels*bits/8;
        const uint8_t bytes[]={1,0,uint8_t(channels),0,uint8_t(hz),uint8_t(hz>>8),0,0,
            uint8_t(hz*align),uint8_t(hz*align>>8),uint8_t(hz*align>>16),0,
            uint8_t(align),0,uint8_t(bits),0};
        assert(cpu.write(p,bytes,16));
    }
    uint32_t allocate(uint32_t size,uint32_t p=format,StartupServiceResult expected=StartupServiceResult::Serviced){
        const uint32_t d[]={36,0x58080,size,0,p,0,0,0,0};
        const uint32_t sentinel=0x12345678;
        assert(cpu.write(desc,d,sizeof(d)) && cpu.write(out,&sentinel,4));
        call(DirectSoundBootstrap::trap_base,3,{DirectSoundBootstrap::object,desc,out,0},expected);
        if(expected!=StartupServiceResult::Serviced){assert(cpu.read_u32(out)==0x12345678);return 0;}
        return cpu.read_u32(out);
    }
    uint32_t buffer(uint32_t b,unsigned slot,std::initializer_list<uint32_t> args={},StartupServiceResult expected=StartupServiceResult::Serviced){
        std::vector<uint32_t> words{b};words.insert(words.end(),args.begin(),args.end());
        // Keep call sites concise while passing the bridge's actual stack ABI.
        switch(words.size()){
        case 1:return call(DirectSoundBootstrap::secondary_trap,slot,{words[0]},expected);
        case 2:return call(DirectSoundBootstrap::secondary_trap,slot,{words[0],words[1]},expected);
        case 3:return call(DirectSoundBootstrap::secondary_trap,slot,{words[0],words[1],words[2]},expected);
        case 4:return call(DirectSoundBootstrap::secondary_trap,slot,{words[0],words[1],words[2],words[3]},expected);
        case 5:return call(DirectSoundBootstrap::secondary_trap,slot,{words[0],words[1],words[2],words[3],words[4]},expected);
        case 8:return call(DirectSoundBootstrap::secondary_trap,slot,{words[0],words[1],words[2],words[3],words[4],words[5],words[6],words[7]},expected);
        default:assert(false);return ~0u;
        }
    }
    void release(uint32_t b){assert(buffer(b,2)==0);}
};

int main(){
    {
        Fixture f;f.pcm(Fixture::format);
        const uint8_t tail[]={'d','a','t','a'};assert(f.cpu.write(Fixture::format+16,tail,4));
        const auto b=f.allocate(1048576);assert(b==DirectSoundBootstrap::secondary);
        assert(f.buffer(b,3,{Fixture::out+16})==0x80070057); // caps size must be 20
        f.release(b);
        puts("PASS physical-79 stereo PCM with data-chunk bytes after its 16-byte format");
    }
    {
        Fixture f;const uint32_t p=0x01FFFFF0;f.pcm(p);
        f.cpu.format_begin=p;f.cpu.format_end=p+16;
        const auto b=f.allocate(1048576,p);assert(f.cpu.format_reads==1);f.release(b);
        puts("PASS 16-byte PCM at guest memory boundary accepted without cbSize read");
    }
    {
        Fixture f;
        for(unsigned offset:{0u,2u,4u,8u,12u,14u}){
            f.pcm(Fixture::format);f.cpu.bytes[Fixture::format+offset]=0;
            f.allocate(1048576,Fixture::format,StartupServiceResult::Unsupported);
        }
        f.pcm(Fixture::format);f.cpu.bytes[Fixture::format]=3;
        f.allocate(1048576,Fixture::format,StartupServiceResult::Unsupported);
        f.pcm(Fixture::format);f.allocate(1048577,Fixture::format,StartupServiceResult::Unsupported);
        f.allocate(DirectSoundBootstrap::pcm_capacity+4,Fixture::format,StartupServiceResult::Unsupported);
        const auto b=f.allocate(1048576);assert(b==DirectSoundBootstrap::secondary);f.release(b);
        puts("PASS unsupported formats, inconsistent rate/alignment and oversized buffers remain rejected");
    }
    {
        Fixture f;
        for(unsigned bits:{8u,16u}){
            f.pcm(Fixture::format,1,bits);const auto b=f.allocate(16);
            assert(f.buffer(b,11,{0,16,Fixture::out+16,Fixture::out+20,Fixture::out+24,Fixture::out+28,2})==0);
            const auto p=f.cpu.read_u32(Fixture::out+16);
            for(unsigned i=0;i<16;++i)assert(f.cpu.bytes[p+i]==(bits==8?128:0));
            assert(f.buffer(b,2,{},StartupServiceResult::Unsupported)==0);
            assert(f.buffer(b,19,{p,16,0,0})==0);f.release(b);
        }
        puts("PASS unsigned 8-bit and signed 16-bit silence, locking and last-reference release");
    }
    {
        Fixture f;f.pcm(Fixture::format);const auto b=f.allocate(1048576);
        const uint32_t iid8[]={0x6825A449,0x4D827524,0xE3500F92,0x1EABB36A};
        assert(f.cpu.write(Fixture::guid,iid8,16));assert(f.buffer(b,0,{Fixture::guid,Fixture::out})==0);
        assert(f.cpu.read_u32(Fixture::out)==b);assert(f.buffer(b,2)==1);
        assert(f.buffer(b,11,{0,524288,Fixture::out+16,Fixture::out+20,Fixture::out+24,Fixture::out+28,0})==0);
        const auto p=f.cpu.read_u32(Fixture::out+16);
        for(unsigned i=0;i<524288;++i)f.cpu.bytes[p+i]=uint8_t(i*7u);
        assert(f.buffer(b,19,{p,524288,0,0})==0);
        assert(f.buffer(b,11,{1048572,8,Fixture::out+16,Fixture::out+20,Fixture::out+24,Fixture::out+28,0})==0);
        assert(f.cpu.read_u32(Fixture::out+20)==4 && f.cpu.read_u32(Fixture::out+28)==4);
        const auto first=f.cpu.read_u32(Fixture::out+16),second=f.cpu.read_u32(Fixture::out+24);
        for(unsigned i=0;i<4;++i)assert(f.cpu.bytes[second+i]==uint8_t(i*7u));
        assert(f.buffer(b,19,{first,4,second,4})==0);f.release(b);
        puts("PASS Buffer8 identity, half-buffer synthetic upload and wrapped PCM lock preserve samples");
    }
    {
        Fixture f;f.pcm(Fixture::format);const auto b=f.allocate(1048576);
        assert(f.buffer(b,5,{0,0,Fixture::out})==0 && f.cpu.read_u32(Fixture::out)==18);
        assert(f.buffer(b,5,{Fixture::out,16,0})==0x80070057);
        assert(f.buffer(b,5,{Fixture::out,18,Fixture::out+24})==0);
        assert(f.cpu.read_u32(Fixture::out+24)==18 && f.cpu.bytes[Fixture::out+16]==0 && f.cpu.bytes[Fixture::out+17]==0);
        assert(f.buffer(b,9,{Fixture::out})==0 && f.cpu.read_u32(Fixture::out)==0);
        f.buffer(b,12,{},StartupServiceResult::Unsupported);
        f.release(b);
        puts("PASS canonical GetFormat, stopped status and explicit unsupported playback boundary");
    }
    assert(opened==closed);puts("PASS simulated port references balanced; no native audio submitted");
}
