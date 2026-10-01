#include "service_smoke.h"
#include "runtime/bridge.h"
#include <algorithm>
#include <cstring>
#include <functional>
#include <string>

namespace {
constexpr uint32_t kHeap=0x00C00000,kHeapSize=0x00100000,kHeapHandle=0x7501;
constexpr uint32_t kCode=0x00712000,kScratch=0x00740000,kScratchSize=0x4000;
constexpr uint32_t kBuffer=kScratch+0x1000,kReadCount=kScratch+0x2000;
constexpr uint32_t kFileHandle=0x7502;
bool inside(uint32_t a,uint32_t n,uint32_t base,uint32_t size) {
    return a>=base && uint64_t(a)+n<=uint64_t(base)+size;
}
bool buffer_ok(uint32_t a,uint32_t n) {
    return inside(a,n,kScratch,kScratchSize)||inside(a,n,kHeap,kHeapSize);
}
void emit32(std::vector<uint8_t>& code,uint32_t value) {
    for(unsigned i=0;i<4;++i) code.push_back(uint8_t(value>>(8*i)));
}
uint32_t iat_for(const d2rt::PeImage& image,const char* name) {
    for(const auto& imp:image.imports()) if(imp.dll=="KERNEL32.dll"&&imp.name==name) return imp.iat_va;
    return 0;
}
bool execute(d2rt::Bridge& bridge,const std::vector<uint8_t>& code,
             uint32_t& result,bool& unsupported,FILE* log,const char* name) {
    auto& cpu=*bridge.cpu();
    cpu.discard_code(kCode,0x1000);
    if(!cpu.write(kCode,code.data(),uint32_t(code.size()))) return false;
    std::vector<uint8_t> copy(code.size());
    if(!cpu.read(kCode,copy.data(),uint32_t(copy.size()))||copy!=code) return false;
    cpu.set_reg(d2rt::R_EFLAGS,0x202);
    fprintf(log,"service_probe_begin=%s\n",name);
    const char* fault=nullptr;
    bool ok=bridge.call_va(kCode,{},result,&fault);
    ok=ok&&!unsupported&&cpu.reg(d2rt::R_EIP)==bridge.sentinel()&&
       cpu.reg(d2rt::R_ESP)==0x009FF000;
    fprintf(log,"service_probe=%s eax=0x%08X esp=0x%08X result=%s\n",
            name,result,cpu.reg(d2rt::R_ESP),ok?"passed":"failed");
    if(fault) fprintf(log,"service_fault=%s\n",fault);
    return ok;
}
bool invoke(d2rt::Bridge& bridge,const d2rt::PeImage& image,const char* name,
            const std::vector<uint32_t>& args,uint32_t& result,bool& unsupported,FILE* log) {
    uint32_t iat=iat_for(image,name);
    uint32_t expected=bridge.shim_trap_existing("KERNEL32.dll",name),target=0;
    if(!iat||!expected||!bridge.cpu()->read(iat,&target,4)||target!=expected) {
        fprintf(log,"service_error=invalid_iat name=%s\n",name);return false;
    }
    std::vector<uint8_t> code;
    for(auto it=args.rbegin();it!=args.rend();++it) { code.push_back(0x68);emit32(code,*it); }
    code.push_back(0xFF);code.push_back(0x15);emit32(code,iat);code.push_back(0xC3);
    return execute(bridge,code,result,unsupported,log,name);
}
}

ServiceSmoke::ServiceSmoke(d2rt::Cpu& cpu,FILE* log,uint32_t& last_error)
    :cpu_(cpu),log_(log),last_error_(last_error) {
    heap_.init(kHeap,kHeapSize,16,"Touhou diagnostic heap");
}
ServiceSmoke::~ServiceSmoke() { if(file_) fclose(file_); }

void ServiceSmoke::install(d2rt::Bridge& bridge) {
    auto add=[&](const char* name,uint32_t argc,std::function<uint32_t(d2rt::Cpu&)> fn) {
        d2rt::Shim shim;shim.argc=argc;
        shim.fn=[this,fn,name](d2rt::Cpu& c) {
            ++calls_;uint32_t value=fn(c);
            fprintf(log_,"service_shim=%s return=0x%08X\n",name,value);return value;
        };
        bridge.register_shim("KERNEL32.dll",name,shim);
    };
    add("GetProcessHeap",0,[](d2rt::Cpu&){return kHeapHandle;});
    add("HeapAlloc",3,[this](d2rt::Cpu& c) {
        if(c.arg(0)!=kHeapHandle||(c.arg(1)&~8u)) {last_error_=87;return 0u;}
        uint32_t n=c.arg(2),a=heap_.alloc(n);
        if(!a) {last_error_=8;return 0u;}
        if(c.arg(1)&8) {
            std::vector<uint8_t> zero(n,0);
            if(!cpu_.write(a,zero.data(),n)) {heap_.free(a);last_error_=487;return 0u;}
        }
        return a;
    });
    add("HeapFree",3,[this](d2rt::Cpu& c) {
        if(c.arg(0)!=kHeapHandle||c.arg(1)||!heap_.free(c.arg(2))) {last_error_=87;return 0u;}
        return 1u;
    });
    add("CreateFileA",7,[this](d2rt::Cpu& c) {
        char name[32]={};bool ended=false;
        for(unsigned i=0;i<sizeof(name);++i) {
            if(!buffer_ok(c.arg(0)+i,1)||!c.read(c.arg(0)+i,&name[i],1)) {last_error_=87;return 0xFFFFFFFFu;}
            if(!name[i]) {ended=true;break;}
        }
        // Only the exercised read-only contract is supported in this iteration.
        if(!ended||strcmp(name,"TH075.exe")||c.arg(1)!=0x80000000u||
           c.arg(2)>7||c.arg(3)||c.arg(4)!=3||c.arg(5)!=0x80||c.arg(6)||file_) {
            last_error_=87;return 0xFFFFFFFFu;
        }
        file_=fopen("ux0:data/TH075Vita/TH075.exe","rb");
        if(!file_) {last_error_=2;return 0xFFFFFFFFu;}
        return kFileHandle;
    });
    add("GetFileSize",2,[this](d2rt::Cpu& c) {
        if(c.arg(0)!=kFileHandle||!file_) {last_error_=6;return 0xFFFFFFFFu;}
        if(c.arg(1)&&!buffer_ok(c.arg(1),4)) {last_error_=87;return 0xFFFFFFFFu;}
        long old=ftell(file_);
        if(old<0||fseek(file_,0,SEEK_END)) {last_error_=30;return 0xFFFFFFFFu;}
        long size=ftell(file_);
        if(fseek(file_,old,SEEK_SET)||size<0) {last_error_=30;return 0xFFFFFFFFu;}
        if(c.arg(1)) {uint32_t high=0;if(!c.write(c.arg(1),&high,4)) {last_error_=87;return 0xFFFFFFFFu;}}
        return uint32_t(size);
    });
    add("ReadFile",5,[this](d2rt::Cpu& c) {
        if(c.arg(0)!=kFileHandle||!file_) {last_error_=6;return 0u;}
        uint32_t n=c.arg(2),count=0;
        if(n>4096||c.arg(4)||!buffer_ok(c.arg(1),n)||!buffer_ok(c.arg(3),4)) {last_error_=87;return 0u;}
        if(!c.write(c.arg(3),&count,4)) {last_error_=87;return 0u;}
        std::vector<uint8_t> bytes(n);
        count=uint32_t(fread(bytes.data(),1,n,file_));
        if(ferror(file_)) {last_error_=30;return 0u;}
        if(!c.write(c.arg(1),bytes.data(),count)||!c.write(c.arg(3),&count,4)) {last_error_=87;return 0u;}
        return 1u;
    });
    add("CloseHandle",1,[this](d2rt::Cpu& c) {
        if(c.arg(0)!=kFileHandle||!file_) {last_error_=6;return 0u;}
        int rc=fclose(file_);file_=nullptr;
        if(rc) {last_error_=6;return 0u;}return 1u;
    });
}

bool ServiceSmoke::run(d2rt::Bridge& bridge,const d2rt::PeImage& image,
                       const std::vector<uint8_t>& exe,bool& unsupported) {
    if(!cpu_.map(kCode,0x1000,nullptr,d2rt::P_RWX)||
       !cpu_.map(kScratch,kScratchSize,nullptr,d2rt::P_RW)||
       !cpu_.map(kHeap,kHeapSize,nullptr,d2rt::P_RW)) {
        fprintf(log_,"service_error=map_failed\n");return false;
    }
    fprintf(log_,"service_contract=diagnostic_heap_and_readonly_exe_only\n");
    uint32_t result=0,heap_handle=0,pointer=0;
    bool ok=invoke(bridge,image,"GetProcessHeap",{},heap_handle,unsupported,log_)&&heap_handle==kHeapHandle;
    if(ok) ok=invoke(bridge,image,"HeapAlloc",{heap_handle,8,256},pointer,unsupported,log_)&&
        inside(pointer,256,kHeap,kHeapSize)&&heap_.size_of(pointer)==256;
    std::vector<uint8_t> bytes(256,0xFF);
    if(ok) ok=cpu_.read(pointer,bytes.data(),256)&&
        std::all_of(bytes.begin(),bytes.end(),[](uint8_t b){return b==0;});
    fprintf(log_,"heap_zero_result=%s\n",ok?"passed":"failed");
    if(ok) {
        std::vector<uint8_t> code={0xB8};emit32(code,pointer); // mov eax,pointer
        code.push_back(0xC7);code.push_back(0x00);emit32(code,0x75); // mov [eax],0x75
        code.push_back(0x8B);code.push_back(0x00);code.push_back(0xC3); // mov eax,[eax];ret
        ok=execute(bridge,code,result,unsupported,log_,"heap_guest_write")&&result==0x75&&cpu_.read_u32(pointer)==0x75;
    }
    if(ok) ok=invoke(bridge,image,"HeapFree",{heap_handle,0,pointer},result,unsupported,log_)&&result==1&&heap_.used_bytes()==0;
    if(ok) ok=invoke(bridge,image,"HeapFree",{heap_handle,0,pointer},result,unsupported,log_)&&result==0&&last_error_==87;
    if(ok) ok=invoke(bridge,image,"HeapAlloc",{heap_handle,0,kHeapSize+1},result,unsupported,log_)&&result==0&&last_error_==8;
    fprintf(log_,"heap_live_bytes=%u\n",heap_.used_bytes());
    fprintf(log_,"heap_smoke_result=%s\n",ok?"passed":"failed");
    if(!ok) return false;

    const char name[]="TH075.exe";
    uint32_t handle=0;
    ok=cpu_.write(kScratch,name,sizeof(name))&&
        invoke(bridge,image,"CreateFileA",{kScratch,0x80000000,1,0,3,0x80,0},handle,unsupported,log_)&&handle==kFileHandle;
    if(ok) ok=invoke(bridge,image,"GetFileSize",{handle,0},result,unsupported,log_)&&result==exe.size();
    if(ok) ok=invoke(bridge,image,"ReadFile",{handle,kBuffer,64,kReadCount,0},result,unsupported,log_)&&result==1&&
        cpu_.read_u32(kReadCount)==64;
    bytes.resize(64);
    if(ok) ok=exe.size()>=64&&cpu_.read(kBuffer,bytes.data(),64)&&std::equal(bytes.begin(),bytes.end(),exe.begin());
    fprintf(log_,"file_header_compare=%s\n",ok?"passed":"failed");
    if(ok) ok=invoke(bridge,image,"CloseHandle",{handle},result,unsupported,log_)&&result==1&&!file_;
    if(ok) ok=invoke(bridge,image,"CloseHandle",{handle},result,unsupported,log_)&&result==0&&last_error_==6;
    fprintf(log_,"service_calls=%u\n",calls_);
    fprintf(log_,"file_smoke_result=%s\n",ok?"passed":"failed");
    return ok;
}
