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
uint64_t sceKernelGetProcessTimeWide(){static uint64_t time=0;return ++time;}

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
    Fixture(bool half=false) {
        assert(log);
        th075::exe_draw_ready=false;th075::exe_present_frames=0;
        th075::exe_draw_width=th075::exe_draw_height=0;th075::exe_draw_preview.fill(0);
        if(half)storage.enable_half_resolution();
        const std::array<uint32_t,13> p={640,480,22,1,0,1,0x00AB7000,1,1,80,1,0,0};
        assert(cpu.write(params,p.data(),sizeof(p)));
        const uint32_t w[]={0x00401000,0x00AB9000,0,2,0x00AB7000,0x20,params,out};
        uint32_t hr=~0u;
        assert(storage.create(w,hr)==StartupServiceResult::Serviced && hr==0);
        assert(cpu.read_u32(out)==D3D8Storage::device && root_refs==2);
        initial_available=dev(4,{});
        assert(initial_available==D3D8Storage::budget-640*480*6-(half?320*240*4:0));
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
        const auto result=storage.call(D3D8Storage::device_trap+slot*16);
        if(result!=StartupServiceResult::Serviced) {
            fprintf(stderr,"device slot %u failed, result %u\n",slot,unsigned(result));
            std::fflush(log);const long length=std::ftell(log);std::fseek(log,std::max(0L,length-4096),SEEK_SET);
            char line[512];while(std::fgets(line,sizeof(line),log))std::fputs(line,stderr);
        }
        assert(result==StartupServiceResult::Serviced);
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
        Fixture f(true);
        assert(f.dev(16,{0,0,Fixture::out})==0);const auto back=f.cpu.read_u32(Fixture::out);
        assert(f.dev(36,{0,0,1,0x12345678,0,0})==0);
        assert(f.call(D3D8Storage::surface_trap,9,{back,Fixture::out,0,0})==0);
        assert(f.cpu.read_u32(Fixture::out)==2560 && f.cpu.read_u32(Fixture::out+4)==D3D8Storage::staging);
        for(unsigned i=0;i<640*480;++i)assert(f.cpu.read_u32(D3D8Storage::staging+i*4)==0xFF345678);
        assert(f.call(D3D8Storage::surface_trap,10,{back})==0);
        const auto texture=f.texture(8,8,1),surface=f.surface(texture);
        assert(f.dev(4,{})==f.initial_available-8*8*4-4*4*4);
        assert(f.call(D3D8Storage::surface_trap,8,{surface,Fixture::out})==0);
        assert(f.cpu.read_u32(Fixture::out+16)==8*8*4 && f.cpu.read_u32(Fixture::out+24)==8);
        assert(f.release(surface,true)==1 && f.release(texture)==0 && f.dev(4,{})==f.initial_available);
        puts("PASS reduced backbuffer Clear/LockRect retain guest pitch/descriptors and release companion budget");
    }
    {
        Fixture f(true);const auto texture=f.texture(4,4,1);
        assert(f.call(D3D8Storage::texture_trap,16,{texture,0,Fixture::out,0,0})==0);
        for(unsigned y=0;y<4;++y)for(unsigned x=0;x<4;++x)
            f.cpu.write_u32(D3D8Storage::staging+(y*4+x)*4,0xFF000000u|((x&2)?0x00440000:0x00110000)|((y&2)?0x00005500:0x00002200)|0x33);
        assert(f.call(D3D8Storage::texture_trap,17,{texture,0})==0);
        const unsigned states[][2]={{7,0},{22,1},{137,0},{27,0}};
        for(const auto& state:states)assert(f.dev(50,{state[0],state[1]})==0);
        assert(f.dev(63,{0,3,1})==0 && f.dev(63,{0,4,4})==0 && f.dev(63,{0,6,1})==0 && f.dev(76,{0x144})==0 && f.dev(61,{0,texture})==0);
        d3d8_quad::Vertex v[4]={{-.5f,-.5f,.5f,1,0xFFFFFFFF,0,0},{639.5f,-.5f,.5f,1,0xFFFFFFFF,1,0},
            {-.5f,479.5f,.5f,1,0xFFFFFFFF,0,1},{639.5f,479.5f,.5f,1,0xFFFFFFFF,1,1}};
        assert(f.cpu.write(Fixture::params,v,sizeof(v)));
        th075::compact_diagnostics=true;th075::suppressed_diagnostics=0;
        assert(f.dev(34,{})==0 && f.dev(72,{5,2,Fixture::params,28})==0 && f.dev(35,{})==0);
        assert(th075::suppressed_diagnostics>0);th075::compact_diagnostics=false;
        assert(f.dev(16,{0,0,Fixture::out})==0);const auto back=f.cpu.read_u32(Fixture::out);
        assert(f.call(D3D8Storage::surface_trap,9,{back,Fixture::out,0,0})==0);
        assert(f.cpu.read_u32(D3D8Storage::staging)==0xFF112233 && f.cpu.read_u32(D3D8Storage::staging+639*4)==0xFF442233);
        assert(f.cpu.read_u32(D3D8Storage::staging+(479*640)*4)==0xFF115533 && f.cpu.read_u32(D3D8Storage::staging+(479*640+639)*4)==0xFF445533);
        assert(f.call(D3D8Storage::surface_trap,10,{back})==0);
        std::rewind(f.log);std::string trace;char chunk[2048];while(std::fgets(chunk,sizeof(chunk),f.log))trace+=chunk;
        assert(trace.find("covered:76800")!=std::string::npos && trace.find("startup_d3d8_boundary_method=")==std::string::npos);
        puts("PASS shipping reduced DrawPrimitiveUP samples render-target companions and returns expanded guest readback");
    }
    {
        Fixture f(true);f.storage.set_raster_executor(d3d8_quad::execute_serial_fast,nullptr);
        const auto texture=f.texture(2,2);
        assert(f.call(D3D8Storage::texture_trap,16,{texture,0,Fixture::out,0,0})==0);
        for(unsigned i=0;i<4;++i)f.cpu.write_u32(D3D8Storage::staging+i*4,0x00112233+i*0x010203);
        assert(f.call(D3D8Storage::texture_trap,17,{texture,0})==0);
        const unsigned states[][2]={{7,0},{22,1},{137,0},{27,1},{15,1},{24,1},{25,7}};
        for(const auto& state:states)assert(f.dev(50,{state[0],state[1]})==0);
        assert(f.dev(63,{0,3,1})==0 && f.dev(63,{0,4,4})==0 && f.dev(63,{0,6,1})==0 &&
            f.dev(76,{0x144})==0 && f.dev(61,{0,texture})==0);
        const d3d8_quad::Vertex v[4]={{-.5f,-.5f,.5f,1,0x80FFFFFF,0,0},{639.5f,-.5f,.5f,1,0x80FFFFFF,1,0},
            {-.5f,479.5f,.5f,1,0x80FFFFFF,0,1},{639.5f,479.5f,.5f,1,0x80FFFFFF,1,1}};
        assert(f.cpu.write(Fixture::params,v,sizeof(v)) && f.dev(36,{0,0,1,0xFFABCDEF,0,0})==0);
        for(unsigned filter:{1u,2u}) {
            assert(f.dev(63,{0,16,filter})==0 && f.dev(63,{0,17,filter})==0);
            assert(f.dev(34,{})==0 && f.dev(72,{5,2,Fixture::params,28})==0 && f.dev(35,{})==0);
        }
        assert(f.dev(16,{0,0,Fixture::out})==0);const auto back=f.cpu.read_u32(Fixture::out);
        assert(f.call(D3D8Storage::surface_trap,9,{back,Fixture::out,0,0})==0);
        for(unsigned i=0;i<640*480;++i)assert(f.cpu.read_u32(D3D8Storage::staging+i*4)==0xFFABCDEF);
        assert(f.call(D3D8Storage::surface_trap,10,{back})==0);f.storage.report_usage();
        std::rewind(f.log);std::string trace;char chunk[2048];while(std::fgets(chunk,sizeof(chunk),f.log))trace+=chunk;
        assert(trace.find("alpha_shortcuts:76800")!=std::string::npos &&
            trace.find("startup_d3d8_alpha_summary=skipped:153600 covered:153600")!=std::string::npos);
        assert(trace.find("startup_d3d8_draw_probe=")==std::string::npos);
        puts("PASS shipping fast executor COM draws discard transparent POINT/LINEAR samples and retain full guest readback");
    }
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
    {
        Fixture f;const auto h=f.texture(2,2),s=f.surface(h);
        assert(f.call(D3D8Storage::texture_trap,16,{h,0,Fixture::out,0,0})==0);
        const uint32_t colors[]={0xFFFF0000,0xFF00FF00,0xFF0000FF,0xFFFFFFFF};
        assert(f.cpu.write(D3D8Storage::staging,colors,sizeof(colors)));
        assert(f.call(D3D8Storage::texture_trap,17,{h,0})==0);
        assert(f.dev(61,{0,h})==0 && f.dev(76,{0x144})==0 && f.dev(34,{})==0);
        assert(f.dev(50,{7,0})==0 && f.dev(50,{137,0})==0 && f.dev(50,{22,1})==0);
        assert(f.dev(50,{15,0})==0 && f.dev(50,{27,0})==0);
        for(const auto stage:std::initializer_list<std::array<uint32_t,2>>{{1,4},{2,2},{3,0},{4,4},{5,2},{6,1},{13,1},{14,1},{16,2},{17,2},{18,2}})
            assert(f.dev(63,{0,stage[0],stage[1]})==0);
        d3d8_quad::Vertex q[4]={{-.5f,-.5f,.5f,1,0xFFFFFFFF,.5f,.5f},{639.5f,-.5f,.5f,1,0xFFFFFFFF,.5f,.5f},
            {-.5f,479.5f,.5f,1,0xFFFFFFFF,.5f,.5f},{639.5f,479.5f,.5f,1,0xFFFFFFFF,.5f,.5f}};
        assert(f.cpu.write(Fixture::out+0x200,q,sizeof(q)));
        assert(f.call(D3D8Storage::device_trap,72,{D3D8Storage::device,5,2,Fixture::out+0x200,28})==0);
        assert(th075::exe_draw_ready && th075::exe_draw_preview[(180-180)/2*260+10]==0xFF808080);
        q[0].x=-.504439f;q[1].y=-.498f;q[2].x=-.505992f;q[3].y=479.502f;
        assert(f.cpu.write(Fixture::out+0x200,q,sizeof(q)));
        assert(f.call(D3D8Storage::device_trap,72,{D3D8Storage::device,5,2,Fixture::out+0x200,28})==0);
        std::fflush(f.log);std::rewind(f.log);std::string trace;char record[512];
        while(std::fgets(record,sizeof(record),f.log))trace+=record;
        assert(trace.find("startup_d3d8_draw_geometry=convex_triangle_strip")!=std::string::npos);
        assert(trace.find("startup_d3d8_vertex_bits=0")!=std::string::npos);
        std::fseek(f.log,0,SEEK_END);
        puts("PASS shipping skewed triangle strip COM ABI and raw vertex-bit diagnostics");
        // A zero-scale selection animation is a successful DrawPrimitiveUP
        // with no fragments. The COM ABI must advance; malformed geometry
        // still stops and must record its original raw words in compact mode.
        for(auto& v:q){v.x=160;v.y=186;}
        assert(f.cpu.write(Fixture::out+0x200,q,sizeof(q)));
        assert(f.call(D3D8Storage::device_trap,72,{D3D8Storage::device,5,2,Fixture::out+0x200,28})==0);
        q[3].rhw=.5f;assert(f.cpu.write(Fixture::out+0x200,q,sizeof(q)));
        th075::compact_diagnostics=true;
        f.call(D3D8Storage::device_trap,72,{D3D8Storage::device,5,2,Fixture::out+0x200,28},StartupServiceResult::Unsupported);
        th075::compact_diagnostics=false;
        std::fflush(f.log);std::rewind(f.log);trace.clear();
        while(std::fgets(record,sizeof(record),f.log))trace+=record;
        assert(trace.find("startup_d3d8_degenerate_noop=")!=std::string::npos);
        assert(trace.find("rhw:3F000000")!=std::string::npos);
        std::fseek(f.log,0,SEEK_END);
        puts("PASS zero-area draw advances COM ABI and compact failure retains raw vertex words");
        q[3].rhw=1;assert(f.cpu.write(Fixture::out+0x200,q,sizeof(q)));
        // The actual COM method must reject unknown/mixed filters without ABI changes.
        assert(f.dev(63,{0,16,3})==0);
        f.call(D3D8Storage::device_trap,72,{D3D8Storage::device,5,2,Fixture::out+0x200,28},StartupServiceResult::Unsupported);
        assert(f.dev(63,{0,16,1})==0);
        f.call(D3D8Storage::device_trap,72,{D3D8Storage::device,5,2,Fixture::out+0x200,28},StartupServiceResult::Unsupported);
        assert(f.dev(63,{0,16,2})==0 && f.dev(63,{0,18,3})==0);
        f.call(D3D8Storage::device_trap,72,{D3D8Storage::device,5,2,Fixture::out+0x200,28},StartupServiceResult::Unsupported);
        assert(f.dev(35,{})==0 && f.dev(61,{0,0})==0 && f.release(s,true)==1 && f.release(h)==0);
        puts("PASS shipping DrawPrimitiveUP LINEAR single-level profile COM ABI and rejections");
    }
    if(argc>=2) {
        // Optional local trace contains only method/argument words. It stays
        // outside Git. Replay ownership/state calls, never draw or run the EXE.
        Fixture f;std::ifstream input(argv[1]);assert(input);
        std::string line;unsigned count=0;uint32_t final_result=~0u,last_create_result=~0u;
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
            if(trap==D3D8Storage::device_trap+20*16){last_create_result=final_result;assert(last_create_result==0);}
        }
        // A completed trace may end with Release (a refcount), not an HRESULT.
        assert(count>1000 && logo_destroyed && last_create_result==0);
        printf("PASS local physical ownership replay: %u calls; logo freed; final CreateTexture succeeds\n",count);
        if(argc==3) {
            // Project only allocation sizes of unexecuted remaining frames,
            // supplied from a local resource-header analysis outside Git.
            std::ifstream projection(argv[2]);assert(projection);
            uint32_t width=0,height=0,expected_used=0;unsigned projected=0;
            while(projection>>width>>height>>expected_used) {
                f.texture(width,height);++projected;
                assert(D3D8Storage::budget-f.dev(4,{})==expected_used);
            }
            assert(projected>0);
            printf("PASS resource-size allocation projection: %u additional images; no EXE/draw/upload executed\n",projected);
        }
    }
    {
        Fixture f(true);
        // Reproduce the exact byte count at the physical iteration93 denial,
        // without needing to execute the copyrighted EXE or upload its pixels.
        constexpr uint32_t physical_used=66386760;
        uint32_t remaining=physical_used-(D3D8Storage::budget-f.dev(4,{}));
        std::vector<uint32_t> retained;
        while(remaining) {
            assert(remaining%4==0);
            const auto pixels=remaining/4;
            unsigned chunk=1;
            while(chunk<=pixels/2 && chunk<1024u*1024)chunk*=2;
            const auto width=std::min(chunk,1024u);
            const auto height=chunk/width;
            retained.push_back(f.texture(width,height));remaining-=width*height*4;
        }
        assert(D3D8Storage::budget-f.dev(4,{})==physical_used);
        const auto last_character=f.texture(512,1024);
        assert(D3D8Storage::budget-f.dev(4,{})==68483912);
        assert(f.release(last_character)==0);
        for(const auto h:retained)assert(f.release(h)==0);
        assert(f.dev(4,{})==f.initial_available);
        puts("PASS physical 93 byte-count regression admits last 2 MiB character texture and reclaims storage");
    }
    {
        Fixture f(true);std::vector<uint32_t> retained;
        assert(D3D8Storage::budget==96u*1024*1024);
        while(f.dev(4,{})>=4u*1024*1024)retained.push_back(f.texture(1024,1024));
        const auto available=f.dev(4,{});
        f.texture(1024,1024,0,0x8876017C);
        assert(f.dev(4,{})==available);
        for(const auto h:retained)assert(f.release(h)==0);
        assert(f.dev(4,{})==f.initial_available);
        puts("PASS 96 MiB cap still rejects exhaustion with real HRESULT and zero COM output");
    }
}
