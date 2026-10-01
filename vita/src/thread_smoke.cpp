#include "thread_smoke.h"
#include "runtime/bridge.h"
#include "runtime/guest_thread_ctx.h"
#include <algorithm>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

namespace {
constexpr uint32_t kPeb=0x00731000,kProcessData=0x00732000,kCode=0x00713000;
constexpr uint32_t kSlots=kDiagnosticTeb+0xE10,kStartupBuffer=0x00743000;
const char* const kCommand="\"C:\\TH075\\TH075.exe\"";
void emit32(std::vector<uint8_t>& code,uint32_t v) {
    for(unsigned i=0;i<4;++i) code.push_back(uint8_t(v>>(8*i)));
}
bool write_word(d2rt::Cpu& c,uint32_t address,uint32_t value) {
    return c.write(address,&value,4);
}
bool execute(d2rt::Bridge& br,const std::vector<uint8_t>& code,uint32_t& ret,
             bool& unsupported,FILE* log,const char* tag) {
    auto& c=*br.cpu();c.discard_code(kCode,0x1000);
    if(!c.write(kCode,code.data(),uint32_t(code.size()))) return false;
    std::vector<uint8_t> copy(code.size());
    if(!c.read(kCode,copy.data(),uint32_t(copy.size()))||copy!=code) return false;
    c.set_reg(d2rt::R_EFLAGS,0x202);
    fprintf(log,"thread_probe_begin=%s\n",tag);
    const char* fault=nullptr;
    bool ok=br.call_va(kCode,{},ret,&fault)&&!unsupported&&
        c.reg(d2rt::R_EIP)==br.sentinel()&&c.reg(d2rt::R_ESP)==0x009FF000;
    fprintf(log,"thread_probe=%s eax=0x%08X esp=0x%08X result=%s\n",
        tag,ret,c.reg(d2rt::R_ESP),ok?"passed":"failed");
    if(fault) fprintf(log,"thread_fault=%s\n",fault);
    return ok;
}
bool call(d2rt::Bridge& br,const d2rt::PeImage& image,const char* name,
          const std::vector<uint32_t>& args,uint32_t& ret,bool& unsupported,FILE* log) {
    uint32_t iat=0,target=0;
    for(const auto& imp:image.imports()) if(imp.dll=="KERNEL32.dll"&&imp.name==name) iat=imp.iat_va;
    uint32_t expected=br.shim_trap_existing("KERNEL32.dll",name);
    if(!iat||!expected||!br.cpu()->read(iat,&target,4)||target!=expected) {
        fprintf(log,"thread_error=iat_mismatch name=%s\n",name);return false;
    }
    std::vector<uint8_t> code;
    for(auto it=args.rbegin();it!=args.rend();++it) {code.push_back(0x68);emit32(code,*it);}
    code.push_back(0xFF);code.push_back(0x15);emit32(code,iat);code.push_back(0xC3);
    return execute(br,code,ret,unsupported,log,name);
}
bool read_fs(d2rt::Bridge& br,uint32_t offset,uint32_t expected,bool& unsupported,FILE* log) {
    fprintf(log,"thread_fs_offset=0x%08X expected=0x%08X\n",offset,expected);
    std::vector<uint8_t> code={0x64,0xA1}; // mov eax,fs:[offset]
    emit32(code,offset);code.push_back(0xC3);uint32_t ret=0;
    return execute(br,code,ret,unsupported,log,"read_fs")&&ret==expected;
}
bool writable_buffer(uint32_t p,uint32_t n) {
    return (p>=0x00740000&&uint64_t(p)+n<=0x00744000)||
           (p>=0x00C00000&&uint64_t(p)+n<=0x00D00000);
}
}

ThreadSmoke::~ThreadSmoke() {
    if(initialized_) {cpu_.set_fs_base(0);wx86_set_main_tib(0);wx86_set_scheduler(nullptr);}
}

bool ThreadSmoke::initialize() {
    std::vector<uint8_t> zero(0x3000,0);
    if(!cpu_.fs_base_is_direct()||!cpu_.map(kDiagnosticTeb,0x3000,zero.data(),d2rt::P_RW)||
       !cpu_.map(kCode,0x1000,nullptr,d2rt::P_RWX)) {
        fprintf(log_,"thread_context_result=failed\n");return false;
    }
    auto put=[&](uint32_t a,uint32_t v){return cpu_.write(a,&v,4);};
    // NT_TIB: empty exception chain, stack boundaries, and self pointer.
    bool ok=put(kDiagnosticTeb,0xFFFFFFFF)&&put(kDiagnosticTeb+4,0x00A00000)&&
        put(kDiagnosticTeb+8,0x00800000)&&put(kDiagnosticTeb+0x18,kDiagnosticTeb)&&
        put(kDiagnosticTeb+0x20,4)&&put(kDiagnosticTeb+0x24,8)&&
        put(kDiagnosticTeb+0x30,kPeb)&&put(kPeb+8,0x00400000)&&
        put(kPeb+0x10,kProcessData+0x100)&&put(kPeb+0x18,0x7501);
    // Minimal x86 process-parameter view: strings and empty environment.
    std::string path="C:\\TH075\\TH075.exe";
    auto unicode=[&](uint32_t descriptor,uint32_t address,const std::string& text) {
        std::vector<uint16_t> wide;for(unsigned char ch:text) wide.push_back(ch);wide.push_back(0);
        uint16_t sizes[2]={uint16_t(text.size()*2),uint16_t((text.size()+1)*2)};
        return cpu_.write(address,wide.data(),uint32_t(wide.size()*2))&&
            cpu_.write(descriptor,sizes,4)&&put(descriptor+4,address);
    };
    ok=ok&&cpu_.write(kProcessData,kCommand,uint32_t(strlen(kCommand)+1))&&
        put(kProcessData+0x100,0x80)&&put(kProcessData+0x104,0x80)&&
        unicode(kProcessData+0x138,kProcessData+0x200,path)&&
        unicode(kProcessData+0x140,kProcessData+0x300,kCommand)&&
        put(kProcessData+0x148,kProcessData+0x400);
    if(!ok) {fprintf(log_,"thread_context_result=failed\n");return false;}
    wx86_set_scheduler(nullptr);wx86_set_main_tib(kDiagnosticTeb);
    cpu_.set_fs_base(kDiagnosticTeb);initialized_=true;
    fprintf(log_,"thread_context_result=initialized\n");
    fprintf(log_,"thread_teb_va=0x%08X\n",kDiagnosticTeb);
    fprintf(log_,"process_peb_va=0x%08X\n",kPeb);
    fprintf(log_,"thread_context_scope=one_diagnostic_thread\n");
    fprintf(log_,"static_tls_callbacks=not_attempted\n");
    return true;
}

void ThreadSmoke::install(d2rt::Bridge& br) {
    auto add=[&](const char* name,uint32_t argc,std::function<uint32_t(d2rt::Cpu&)> fn) {
        d2rt::Shim shim;shim.argc=argc;shim.fn=[this,name,fn](d2rt::Cpu& c) {
            uint32_t ret=fn(c);fprintf(log_,"thread_shim=%s return=0x%08X\n",name,ret);return ret;
        };br.register_shim("KERNEL32.dll",name,shim);
    };
    add("TlsAlloc",0,[this](d2rt::Cpu& c) {
        for(uint32_t i=0;i<allocated_.size();++i) if(!allocated_[i]) {
            if(!write_word(c,kSlots+4*i,0)) return 0xFFFFFFFFu;
            allocated_[i]=true;return i;
        }
        wx86_set_lasterr(c,259);return 0xFFFFFFFFu;
    });
    add("TlsSetValue",2,[this](d2rt::Cpu& c) {
        uint32_t i=c.arg(0);
        if(i>=allocated_.size()||!allocated_[i]) {wx86_set_lasterr(c,87);return 0u;}
        return write_word(c,kSlots+4*i,c.arg(1))?1u:0u;
    });
    add("TlsGetValue",1,[this](d2rt::Cpu& c) {
        uint32_t i=c.arg(0);
        if(i>=allocated_.size()) {wx86_set_lasterr(c,87);return 0u;}
        wx86_set_lasterr(c,0);return c.read_u32(kSlots+4*i);
    });
    add("TlsFree",1,[this](d2rt::Cpu& c) {
        uint32_t i=c.arg(0);
        if(i>=allocated_.size()||!allocated_[i]) {wx86_set_lasterr(c,87);return 0u;}
        if(!write_word(c,kSlots+4*i,0)) return 0u;
        allocated_[i]=false;return 1u;
    });
    add("GetCommandLineA",0,[](d2rt::Cpu&){return kProcessData;});
    add("GetModuleHandleA",1,[](d2rt::Cpu& c) {
        if(!c.arg(0)) return 0x00400000u;
        wx86_set_lasterr(c,126);return 0u; // Named modules are not implemented.
    });
    add("GetStartupInfoA",1,[](d2rt::Cpu& c) {
        uint32_t p=c.arg(0);
        if(!writable_buffer(p,68)) {wx86_set_lasterr(c,87);return 0u;}
        uint32_t info[17]={};info[0]=sizeof(info);
        if(!c.write(p,info,sizeof(info))) wx86_set_lasterr(c,87);
        return 0u; // This API has a void return type.
    });
}

bool ThreadSmoke::run(d2rt::Bridge& br,const d2rt::PeImage& image,bool& unsupported) {
    bool ok=read_fs(br,0x18,kDiagnosticTeb,unsupported,log_)&&
        read_fs(br,4,0x00A00000,unsupported,log_)&&read_fs(br,8,0x00800000,unsupported,log_)&&
        read_fs(br,0x30,kPeb,unsupported,log_)&&read_fs(br,0x34,6,unsupported,log_);
    uint32_t ret=0;
    // Check both directions: IAT setter -> FS read, FS write -> IAT getter.
    if(ok) ok=call(br,image,"SetLastError",{0x1075},ret,unsupported,log_)&&
        read_fs(br,0x34,0x1075,unsupported,log_);
    if(ok) {
        std::vector<uint8_t> code={0x64,0xC7,0x05};emit32(code,0x34);emit32(code,0x1076);code.push_back(0xC3);
        ok=execute(br,code,ret,unsupported,log_,"write_fs")&&
            call(br,image,"GetLastError",{},ret,unsupported,log_)&&ret==0x1076;
    }
    fprintf(log_,"teb_fs_smoke_result=%s\n",ok?"passed":"failed");
    if(!ok) return false;
    uint32_t first=0,second=0;
    ok=call(br,image,"TlsAlloc",{},first,unsupported,log_)&&first<64&&
       call(br,image,"TlsAlloc",{},second,unsupported,log_)&&second<64&&first!=second;
    if(ok) ok=call(br,image,"TlsGetValue",{first},ret,unsupported,log_)&&ret==0&&wx86_get_lasterr(cpu_)==0;
    if(ok) ok=call(br,image,"TlsSetValue",{first,0x1075},ret,unsupported,log_)&&ret==1&&
        read_fs(br,0xE10+4*first,0x1075,unsupported,log_);
    if(ok) ok=call(br,image,"TlsGetValue",{second},ret,unsupported,log_)&&ret==0&&
        call(br,image,"TlsSetValue",{second,0x2075},ret,unsupported,log_)&&ret==1&&
        call(br,image,"TlsGetValue",{first},ret,unsupported,log_)&&ret==0x1075&&
        call(br,image,"TlsGetValue",{second},ret,unsupported,log_)&&ret==0x2075;
    if(ok) ok=call(br,image,"TlsGetValue",{64},ret,unsupported,log_)&&ret==0&&wx86_get_lasterr(cpu_)==87;
    if(ok) ok=call(br,image,"TlsFree",{first},ret,unsupported,log_)&&ret==1&&
        call(br,image,"TlsFree",{first},ret,unsupported,log_)&&ret==0&&wx86_get_lasterr(cpu_)==87;
    if(ok) ok=call(br,image,"TlsAlloc",{},ret,unsupported,log_)&&ret==first&&
        call(br,image,"TlsGetValue",{first},ret,unsupported,log_)&&ret==0;
    if(ok) ok=call(br,image,"TlsFree",{first},ret,unsupported,log_)&&ret==1&&
        call(br,image,"TlsFree",{second},ret,unsupported,log_)&&ret==1;
    const auto live=std::count(allocated_.begin(),allocated_.end(),true);
    ok=ok&&live==0;fprintf(log_,"tls_live_slots=%u\n",unsigned(live));
    fprintf(log_,"tls_smoke_result=%s\n",ok?"passed":"failed");if(!ok) return false;

    // Traverse FS -> PEB -> ImageBaseAddress using guest instructions.
    std::vector<uint8_t> peb_code={0x64,0xA1};emit32(peb_code,0x30);
    peb_code.push_back(0x8B);peb_code.push_back(0x40);peb_code.push_back(0x08);peb_code.push_back(0xC3);
    ok=execute(br,peb_code,ret,unsupported,log_,"peb_image_base")&&ret==0x00400000&&
        call(br,image,"GetModuleHandleA",{0},ret,unsupported,log_)&&ret==0x00400000&&
        cpu_.read_u32(kPeb+0x10)==kProcessData+0x100;
    if(ok) ok=call(br,image,"GetCommandLineA",{},ret,unsupported,log_)&&ret==kProcessData;
    char command[64]={};
    if(ok) ok=cpu_.read(ret,command,uint32_t(strlen(kCommand)+1))&&!strcmp(command,kCommand);
    if(ok) ok=call(br,image,"GetStartupInfoA",{kStartupBuffer},ret,unsupported,log_)&&
        cpu_.read_u32(kStartupBuffer)==68;
    uint32_t startup[17]={};
    if(ok) ok=cpu_.read(kStartupBuffer,startup,sizeof(startup))&&
        std::all_of(startup+1,startup+17,[](uint32_t v){return v==0;});
    fprintf(log_,"process_smoke_result=%s\n",ok?"passed":"failed");
    return ok;
}
