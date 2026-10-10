// Exercise the shipping bridge with synthetic PCM and COM frames. Port setup
// is simulated; no game binary or Vita hardware is exercised.
#include "../vita/src/dsound_bootstrap.h"
#include <cassert>
#include <cstring>
#include <initializer_list>

#include "support/dsound_native_stub.h"
struct LogFile {FILE* file=std::tmpfile();~LogFile(){if(file)std::fclose(file);}};

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
    HostCpu cpu;LogFile log;DirectSoundBootstrap sound{cpu,log.file};
    Fixture(){
        assert(log.file);
        const uint32_t clsid[]={0x3901CC3F,0x4FA484B5,0x81AA35BA,0x9BA0B872};
        const uint32_t iid[]={0xC50A7E93,0x4834F395,0xA97FF69E,0x6609E59D};
        assert(cpu.write(guid,clsid,16) && cpu.write(guid+16,iid,16));
        const uint32_t w[]={ret,guid,0,3,guid+16,out};
        assert(cpu.write(stack,w,sizeof(w)));cpu.set_reg(d2rt::R_ESP,stack);
        assert(sound.create(true)==StartupServiceResult::Serviced);
        assert(call(DirectSoundBootstrap::trap_base,10,{DirectSoundBootstrap::object,0})==0);
        assert(call(DirectSoundBootstrap::trap_base,6,{DirectSoundBootstrap::object,0xAB7000,2})==0);
    }
    ~Fixture(){if(sound.healthy())assert(call(DirectSoundBootstrap::trap_base,2,{DirectSoundBootstrap::object})==0);}
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
        f.buffer(b,12,{1,0,1},StartupServiceResult::Unsupported);
        f.release(b);
        puts("PASS canonical GetFormat, stopped status and unsupported reserved Play arguments");
    }
    {
        Fixture f;f.pcm(Fixture::format);constexpr unsigned size=1048576,chunk=131072;
        const auto b=f.allocate(size);
        // The original refill uses ENTIREBUFFER but writes only 128 KiB at
        // its requested offset. The returned full lock wraps at the ring end.
        for(unsigned segment=0;segment<8;++segment){
            const unsigned offset=segment*chunk;
            const unsigned requested=segment==0?0:chunk; // ignored for flag 2
            assert(f.buffer(b,11,{offset,requested,Fixture::out+16,Fixture::out+20,Fixture::out+24,Fixture::out+28,2})==0);
            const auto p=f.cpu.read_u32(Fixture::out+16),n=f.cpu.read_u32(Fixture::out+20),
                p2=f.cpu.read_u32(Fixture::out+24),n2=f.cpu.read_u32(Fixture::out+28);
            assert(p==DirectSoundBootstrap::pcm_staging+offset && n==size-offset);
            assert(p2==(offset?DirectSoundBootstrap::pcm_staging:0) && n2==offset);
            for(unsigned i=0;i<size;++i)
                assert(f.cpu.bytes[DirectSoundBootstrap::pcm_staging+i]==(i<offset?uint8_t(i/chunk+1):0));
            std::memset(f.cpu.hostptr(p,chunk),int(segment+1),chunk);
            assert(f.buffer(b,19,{p,n,p2,n2})==0);
        }
        assert(f.buffer(b,11,{0,1,Fixture::out+16,Fixture::out+20,Fixture::out+24,Fixture::out+28,2})==0);
        for(unsigned i=0;i<size;++i)assert(f.cpu.bytes[DirectSoundBootstrap::pcm_staging+i]==i/chunk+1);
        assert(f.buffer(b,19,{DirectSoundBootstrap::pcm_staging,size,0,0})==0);f.release(b);
        puts("PASS ENTIREBUFFER preserves all eight original-sized refill offsets, split spans and untouched PCM");
    }
    {
        Fixture f;f.pcm(Fixture::format);const auto b=f.allocate(4096);
        for(const auto& args:std::initializer_list<std::array<uint32_t,3>>{{1,16,2},{4096,16,2},{0xFFFFFFFCu,16,2},{0,16,1},{0,16,3},{0,0,0},{0,4097,0}}){
            std::memset(f.cpu.hostptr(Fixture::out+16,16),0xA5,16);
            std::memset(f.cpu.hostptr(DirectSoundBootstrap::pcm_staging,4096),0x5A,4096);
            f.buffer(b,11,{args[0],args[1],Fixture::out+16,Fixture::out+20,Fixture::out+24,Fixture::out+28,args[2]},StartupServiceResult::Unsupported);
            for(unsigned i=0;i<16;++i)assert(f.cpu.bytes[Fixture::out+16+i]==0xA5);
            for(unsigned i=0;i<4096;++i)assert(f.cpu.bytes[DirectSoundBootstrap::pcm_staging+i]==0x5A);
            assert(f.sound.refill_dispatch_safe());
        }
        f.release(b);puts("PASS invalid lock offsets, alignment, sizes and flags stop before staging or pointer writes");
    }
    {
        native_stub::reset();Fixture f;f.pcm(Fixture::format,2,16,48000);
        constexpr unsigned size=8192,chunk=1024;const auto b=f.allocate(size);
        for(unsigned segment=0;segment<8;++segment){
            const unsigned offset=segment*chunk;
            assert(f.buffer(b,11,{offset,chunk,Fixture::out+16,Fixture::out+20,Fixture::out+24,Fixture::out+28,2})==0);
            const auto p=f.cpu.read_u32(Fixture::out+16),n=f.cpu.read_u32(Fixture::out+20),
                p2=f.cpu.read_u32(Fixture::out+24),n2=f.cpu.read_u32(Fixture::out+28);
            for(unsigned i=0;i<chunk;i+=4){
                const uint16_t left=uint16_t(1000+segment),right=uint16_t(-int(1000+segment));
                f.cpu.bytes[p+i]=uint8_t(left);f.cpu.bytes[p+i+1]=uint8_t(left>>8);
                f.cpu.bytes[p+i+2]=uint8_t(right);f.cpu.bytes[p+i+3]=uint8_t(right>>8);
            }
            assert(f.buffer(b,19,{p,n,p2,n2})==0);
        }
        assert(f.buffer(b,13,{size-chunk})==0 && f.buffer(b,12,{0,0,1})==0);
        native_stub::wait_block();assert(f.buffer(b,18)==0);
        native_stub::finish();f.sound.shutdown_playback();
        for(unsigned frame=0;frame<1024;++frame){
            const unsigned segment=((size-chunk+frame*4)%size)/chunk;
            assert(native_stub::captured[0][frame*2]==int(1000+segment));
            assert(native_stub::captured[0][frame*2+1]==-int(1000+segment));
        }
        f.release(b);puts("PASS shipping native output captures distinct offset refills across the loop seam without silent samples");
    }
    {
        native_stub::reset();Fixture f;f.pcm(Fixture::format);const auto b=f.allocate(1048576);
        assert(f.buffer(b,11,{0,524288,Fixture::out+16,Fixture::out+20,Fixture::out+24,Fixture::out+28,2})==0);
        const auto p=f.cpu.read_u32(Fixture::out+16);assert(f.cpu.read_u32(Fixture::out+20)==1048576);
        for(unsigned i=0;i<524288;i+=4){f.cpu.bytes[p+i]=0x10;f.cpu.bytes[p+i+1]=0x27;f.cpu.bytes[p+i+2]=0xF0;f.cpu.bytes[p+i+3]=0xD8;}
        assert(f.buffer(b,19,{p,1048576,0,0})==0);
        assert(f.buffer(b,12,{0,0,1})==0);native_stub::wait_block();
        assert(f.buffer(b,9,{Fixture::out})==0 && f.cpu.read_u32(Fixture::out)==5);
        assert(f.buffer(b,4,{Fixture::out,Fixture::out+4})==0 && f.cpu.read_u32(Fixture::out)==0);
        assert(f.buffer(b,12,{0,0,1})==0); // repeated Play retains position
        assert(f.buffer(b,13,{4})==0); // seek while an old epoch is pending
        assert(f.buffer(b,18)==0);
        assert(f.buffer(b,9,{Fixture::out})==0 && f.cpu.read_u32(Fixture::out)==0);
        native_stub::finish();f.sound.shutdown_playback();
        assert(native_stub::captured[0][0]==10000 && native_stub::captured[0][1]==-10000);
        assert(f.buffer(b,4,{Fixture::out,0})==0 && f.cpu.read_u32(Fixture::out)==4);
        f.release(b);puts("PASS physical-80 Play frame, captured guest samples, status, repeated Play, seek, Stop and join");
    }
    {
        native_stub::reset();Fixture f;f.pcm(Fixture::format);const auto b=f.allocate(1048576);
        assert(f.buffer(b,12,{0,0,1})==0);native_stub::wait_block();
        assert(f.buffer(b,12,{0,0,1})==0);
        f.buffer(b,12,{0,0,0},StartupServiceResult::Unsupported);
        native_stub::finish();f.sound.shutdown_playback();
        assert(f.buffer(b,4,{Fixture::out,0})==0 && f.cpu.read_u32(Fixture::out)>0);
        f.release(b);puts("PASS repeated Play retains accepted block progress; active loop-mode switch remains explicit boundary");
    }
    {
        native_stub::reset();Fixture f;f.pcm(Fixture::format);const auto b=f.allocate(4096);
        assert(f.buffer(b,12,{0,0,1})==0);native_stub::wait_block();f.release(b);
        native_stub::finish();f.sound.shutdown_playback();
        puts("PASS last buffer release while output pending discards stale cursor commit and joins before port release");
    }
    for(unsigned kind=0;kind<3;++kind){
        native_stub::reset();Fixture f;f.pcm(Fixture::format);const auto b=f.allocate(4096);
        if(kind==0)native_stub::fail_volume=-101;
        if(kind==1)native_stub::fail_create=-102;
        if(kind==2)native_stub::fail_start=-103;
        f.buffer(b,12,{0,0,1},StartupServiceResult::ContractFailure);
        assert(!f.sound.playback_active());f.release(b);
    }
    puts("PASS native volume, thread creation and thread start failures do not report successful playback");
    {
        native_stub::reset();Fixture f;f.pcm(Fixture::format);const auto b=f.allocate(4096);
        native_stub::fail_output=-104;assert(f.buffer(b,12,{0,0,1})==0);
        native_stub::wait_block();native_stub::finish();f.sound.shutdown_playback();
        assert(!f.sound.healthy());f.buffer(b,9,{Fixture::out},StartupServiceResult::ContractFailure);
        puts("PASS asynchronous output failure propagates, drains and joins without accepting another service");
    }
    {
        native_stub::reset();Fixture f;f.pcm(Fixture::format);const auto b=f.allocate(4096);
        assert(f.buffer(b,12,{0,0,1})==0);native_stub::wait_block();native_stub::rest.store(-105);
        f.buffer(b,4,{Fixture::out,Fixture::out+4},StartupServiceResult::ContractFailure);
        assert(!f.sound.healthy());native_stub::finish();f.sound.shutdown_playback();
        puts("PASS failed native queue query records asynchronous failure and leaves the guest call unconsumed");
    }
    assert(native_stub::opened==native_stub::closed && native_stub::starts==native_stub::joins);
    puts("PASS simulated port and thread references balanced; Vita audibility remains unverified");
}
