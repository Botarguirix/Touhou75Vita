// Exercise the shipping D3D8Storage class with owned host memory and real COM
// stack layouts. No EXE or proprietary resource is executed or distributed.
#include "../vita/src/d3d8_storage.h"
#include <cassert>
#include <initializer_list>
#include <fstream>
#include <sstream>
#include <string>

// Native display/allocator paths must never be reached by these storage tests.
int sceDisplaySetFrameBuf(const SceDisplayFrameBuf*,int){assert(false);return -1;}
int sceDisplayGetFrameBuf(SceDisplayFrameBuf*,int){assert(false);return -1;}
int sceDisplayWaitVblankStart(){assert(false);return -1;}
SceUID sceKernelAllocMemBlock(const char*,int,uint32_t,void*){assert(false);return -1;}
int sceKernelGetMemBlockBase(SceUID,void**){assert(false);return -1;}
int sceKernelFreeMemBlock(SceUID){assert(false);return -1;}
uint64_t sceKernelGetProcessTimeWide(){assert(false);return 0;}

class HostCpu final:public d2rt::Cpu {
public:
    std::vector<uint8_t> bytes=std::vector<uint8_t>(0x02000000);
    std::array<uint32_t,d2rt::R_COUNT> registers{};
    bool map(uint32_t va,uint32_t size,const void* init,int) override {
        if(!hostptr(va,size))return false;
        if(init)std::memcpy(bytes.data()+va,init,size);
        return true;
    }
    bool protect(uint32_t va,uint32_t size,int) override {return hostptr(va,size)!=nullptr;}
    bool read(uint32_t va,void* out,uint32_t size) override {
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
    static constexpr uint32_t stack=0x009F0000,out=0x00900000,params=out+0x100;
    HostCpu cpu;FILE* log=std::tmpfile();uint32_t root_refs=1;
    D3D8Storage storage{cpu,log,root_refs};uint32_t initial_available=0;
    Fixture() {
        assert(log);
        const std::array<uint32_t,13> p={640,480,22,1,0,1,0x00AB7000,1,1,80,1,0,0};
        assert(cpu.write(params,p.data(),sizeof(p)));
        const uint32_t w[]={0x00401000,0x00AB9000,0,2,0x00AB7000,0x20,params,out};
        uint32_t hr=~0u;
        assert(storage.create(w,hr)==StartupServiceResult::Serviced && hr==0);
        assert(cpu.read_u32(out)==D3D8Storage::device && root_refs==2);
        initial_available=dev(4,{});
        assert(initial_available==D3D8Storage::budget-640*480*6);
    }
    ~Fixture(){std::fclose(log);}
    uint32_t call(uint32_t base,unsigned slot,std::initializer_list<uint32_t> args,
        StartupServiceResult expected=StartupServiceResult::Serviced) {
        std::vector<uint32_t> w{0x00401000};w.insert(w.end(),args.begin(),args.end());
        assert(cpu.write(stack,w.data(),uint32_t(w.size()*4)));
        cpu.set_reg(d2rt::R_ESP,stack);cpu.set_reg(d2rt::R_EIP,base+slot*16);
        const int kept[]={d2rt::R_EBX,d2rt::R_EBP,d2rt::R_ESI,d2rt::R_EDI};
        for(int r:kept)cpu.set_reg(r,0x11220000u+uint32_t(r));
        assert(storage.call(base+slot*16)==expected);
        for(int r:kept)assert(cpu.reg(r)==0x11220000u+uint32_t(r));
        assert(cpu.reg(d2rt::R_ESP)==(expected==StartupServiceResult::Serviced?stack+w.size()*4:stack));
        assert(cpu.reg(d2rt::R_EIP)==(expected==StartupServiceResult::Serviced?0x00401000:base+slot*16));
        return cpu.reg(d2rt::R_EAX);
    }
    uint32_t dev(unsigned slot,std::initializer_list<uint32_t> args) {
        std::vector<uint32_t> w{D3D8Storage::device};w.insert(w.end(),args.begin(),args.end());
        // initializer_list forwarding is deliberately kept at call sites below.
        const uint32_t ret=0x00401000;
        assert(cpu.write(stack,&ret,4) && cpu.write(stack+4,w.data(),uint32_t(w.size()*4)));
        cpu.set_reg(d2rt::R_ESP,stack);cpu.set_reg(d2rt::R_EIP,D3D8Storage::device_trap+slot*16);
        assert(storage.call(D3D8Storage::device_trap+slot*16)==StartupServiceResult::Serviced);
        assert(cpu.reg(d2rt::R_ESP)==stack+(w.size()+1)*4 && cpu.reg(d2rt::R_EIP)==ret);
        return cpu.reg(d2rt::R_EAX);
    }
    uint32_t texture(uint32_t width=1024,uint32_t height=512,uint32_t usage=0,
        uint32_t hr=0) {
        assert(dev(20,{width,height,1,usage,21,usage?0u:1u,out})==hr);
        const auto h=cpu.read_u32(out);assert(hr?h==0:h!=0);return h;
    }
    uint32_t surface(uint32_t h) {
        assert(call(D3D8Storage::texture_trap,15,{h,0,out})==0);return cpu.read_u32(out);
    }
    uint32_t release(uint32_t h,bool surface=false) {
        return call(surface?D3D8Storage::surface_trap:D3D8Storage::texture_trap,2,{h});
    }
    void dead(uint32_t h) {
        assert(cpu.read_u32(h)==0);
        call(D3D8Storage::texture_trap,1,{h},StartupServiceResult::ContractFailure);
    }
};

int main(int argc,char** argv) {
    {
        Fixture f;const auto h=f.texture(),s=f.surface(h);
        assert(f.surface(h)==s);
        assert(f.release(s,true)==2 && f.release(s,true)==1 && f.release(h)==0);
        assert(f.dev(4,{})==f.initial_available);f.dead(h);
        assert(f.cpu.read_u32(s)==0);
        f.call(D3D8Storage::surface_trap,8,{s,Fixture::out},StartupServiceResult::ContractFailure);
        puts("PASS forwarded level-surface references, destruction, budget and stale aliases");
    }
    {
        Fixture f;const auto h=f.texture(),s=f.surface(h);
        assert(f.release(h)==1);
        assert(f.call(D3D8Storage::surface_trap,8,{s,Fixture::out})==0);
        assert(f.cpu.read_u32(Fixture::out+24)==1024);
        assert(f.call(D3D8Storage::surface_trap,1,{s})==2);
        assert(f.release(s,true)==1 && f.release(s,true)==0);
        assert(f.dev(4,{})==f.initial_available);f.dead(h);
        puts("PASS surface keeps parent storage alive after caller releases texture");
    }
    {
        Fixture f;const auto h=f.texture();
        assert(f.dev(61,{0,h})==0 && f.dev(52,{})==0 && f.dev(61,{0,h})==0);
        assert(f.dev(53,{Fixture::out})==0);const auto token=f.cpu.read_u32(Fixture::out);
        assert(f.release(h)==2 && f.dev(61,{0,0})==0);
        assert(f.cpu.read_u32(h)==D3D8Storage::texture_table);
        assert(f.dev(54,{token})==0 && f.dev(60,{0,Fixture::out})==0);
        assert(f.cpu.read_u32(Fixture::out)==h && f.release(h)==2);
        assert(f.dev(56,{token})==0 && f.dev(61,{0,0})==0);
        assert(f.dev(4,{})==f.initial_available);f.dead(h);
        puts("PASS texture binding, state-block transfer/apply/delete retain and free correctly");
    }
    {
        Fixture f;const auto h=f.texture(512,512,1),s=f.surface(h);
        assert(f.dev(16,{0,0,Fixture::out})==0);const auto back=f.cpu.read_u32(Fixture::out);
        assert(f.dev(31,{s,0})==0 && f.release(s,true)==2 && f.release(h)==1);
        assert(f.dev(32,{Fixture::out})==0 && f.cpu.read_u32(Fixture::out)==s);
        assert(f.release(s,true)==1 && f.dev(31,{back,0})==0);
        assert(f.call(D3D8Storage::surface_trap,2,{back})==2);
        assert(f.dev(4,{})==f.initial_available);f.dead(h);
        puts("PASS render-target surface retains parent until replaced; implicit backbuffer survives");
    }
    {
        Fixture f;const auto h=f.texture(32,32);
        f.call(D3D8Storage::texture_trap,15,{h,0,4},StartupServiceResult::ContractFailure);
        assert(f.call(D3D8Storage::texture_trap,1,{h})==2 && f.release(h)==1);
        assert(f.call(D3D8Storage::texture_trap,16,{h,0,Fixture::out,0,0})==0);
        f.call(D3D8Storage::texture_trap,2,{h},StartupServiceResult::ContractFailure);
        assert(f.cpu.read_u32(h)==D3D8Storage::texture_table);
        const uint32_t pixel=0x55667788;f.cpu.write_u32(D3D8Storage::staging,pixel);
        assert(f.call(D3D8Storage::texture_trap,17,{h,0})==0);
        const auto s=f.surface(h);assert(f.release(s,true)==1 && f.release(h)==0);
        assert(f.dev(4,{})==f.initial_available);f.dead(h);
        puts("PASS invalid outputs do not add refs and locked last reference cannot be destroyed");
    }
    {
        Fixture f;const auto logo=f.texture(),surface=f.surface(logo);
        assert(f.release(surface,true)==1); // The observed temporary surface is released.
        std::vector<uint32_t> retained;
        while(f.dev(4,{})>=4u*1024*1024)retained.push_back(f.texture(1024,1024));
        while(f.dev(4,{})>=1024u*1024)retained.push_back(f.texture(512,512));
        const auto before=f.dev(4,{});assert(before<1024u*1024);
        f.texture(512,512,0,0x8876017C);
        assert(f.release(logo)==0 && f.dev(4,{})==before+2u*1024*1024);
        const auto next=f.texture(512,512);
        assert(f.dev(4,{})==before+1024u*1024 && f.release(next)==0);
        for(auto h:retained)assert(f.release(h)==0);
        assert(f.dev(4,{})==f.initial_available);
        puts("PASS observed 2 MiB logo release makes room for formerly rejected 1 MiB allocation");
    }
    if(argc==2) {
        // Optional local trace contains only method/argument words. It stays
        // outside Git. Replay ownership/state calls, never draw or run the EXE.
        Fixture f;std::ifstream input(argv[1]);assert(input);
        std::string line;unsigned count=0;uint32_t final_result=~0u;
        bool logo_destroyed=false;
        while(std::getline(input,line)) {
            std::istringstream record(line);record>>std::hex;
            uint32_t trap=0,word=0;assert(record>>trap);
            std::vector<uint32_t> w{0x00401000};while(record>>word)w.push_back(word);
            assert(w.size()>=2 && w.size()<=10 && f.storage.owns(trap));
            assert(f.cpu.write(Fixture::stack,w.data(),uint32_t(w.size()*4)));
            f.cpu.set_reg(d2rt::R_ESP,Fixture::stack);f.cpu.set_reg(d2rt::R_EIP,trap);
            assert(f.storage.call(trap)==StartupServiceResult::Serviced);
            assert(f.cpu.reg(d2rt::R_ESP)==Fixture::stack+w.size()*4 && f.cpu.reg(d2rt::R_EIP)==w[0]);
            final_result=f.cpu.reg(d2rt::R_EAX);++count;
            if(trap==D3D8Storage::texture_trap+2*16 && w[1]==0x00ABC938) {
                assert(final_result==0 && f.cpu.read_u32(w[1])==0);logo_destroyed=true;
            }
            if(trap==D3D8Storage::device_trap+20*16)assert(final_result==0);
        }
        assert(count>1000 && logo_destroyed && final_result==0);
        printf("PASS local physical-77 ownership replay: %u calls; logo freed; final CreateTexture succeeds\n",count);
    }
}
