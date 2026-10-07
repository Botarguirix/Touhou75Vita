#include "batch_checks.h"
#include "startup_services.h"
#include "runtime/cpu.h"
#include "runtime/pe_image.h"
#include "runtime/guest_thread.h"
#include "runtime/guest_thread_ctx.h"
#include <array>
#include <vector>
#include <initializer_list>

bool run_batch_checks(d2rt::Cpu& cpu, StartupServices& services, FILE* log) {
    constexpr uint32_t frame = 0x00800100, scratch = 0x00A30000, ret = 0x0064232C;
    d2rt::X86Context saved{};
    cpu.save_context(saved);
    const uint32_t old_error = wx86_get_lasterr(cpu);
    std::array<uint8_t,36> old_frame{};
    const bool setup = cpu.read(frame,old_frame.data(),old_frame.size()) &&
        cpu.map(scratch,0x1000,nullptr,d2rt::P_RW) && services.heap_ready();
    struct Restore {
        d2rt::Cpu& cpu; const d2rt::X86Context& state;
        const std::array<uint8_t,36>& bytes; uint32_t error; bool valid;
        ~Restore() {
            if(valid) cpu.write(0x00800100,bytes.data(),bytes.size());
            cpu.load_context(state); wx86_set_lasterr(cpu,error);
        }
    } restore{cpu,saved,old_frame,old_error,setup};
    fprintf(log,"batch_scope=independent_synthetic_service_checks_on_vita\n");
    fprintf(log,"batch_total=4\n");
    unsigned passed = 0;
    const char* step = "setup";
    auto call = [&](const char* name, std::initializer_list<uint32_t> args, uint32_t& result) {
        step = name;
        std::vector<uint32_t> words{ret}; words.insert(words.end(),args.begin(),args.end());
        if(!cpu.write(frame,words.data(),uint32_t(words.size()*4))) return false;
        cpu.set_reg(d2rt::R_ESP,frame);
        const d2rt::ImportRef imp{"KERNEL32.dll",name,0,0,0};
        const auto status = services.call(imp);
        result = cpu.reg(d2rt::R_EAX);
        fprintf(log,"batch_api=%s result=0x%08X serviced=%s\n",name,result,
            status==StartupServiceResult::Serviced ? "yes" : "no");
        return status==StartupServiceResult::Serviced;
    };
    auto report = [&](unsigned id,const char* name,bool ok,const char* error) {
        fprintf(log,"batch_test_%02u_name=%s\n",id,name);
        fprintf(log,"batch_test_%02u_result=%s\n",id,ok ? "passed" : "failed");
        fprintf(log,"batch_test_%02u_error=%s\n",id,ok ? "none" : error);
        passed += ok;
        fflush(log);
    };
    // Always attempt every independent case. Cleanup does not hide its failed step.
    for(unsigned id=1;id<=2;++id) {
        fprintf(log,"batch_test_begin=%02u\n",id);
        uint32_t handle=0,value=0;
        bool ok=setup && call("CreateEventA",{0,id==2 ? 1u : 0u,id==2 ? 1u : 0u,0},handle) && handle;
        if(id==1) {
            ok=ok && call("WaitForSingleObject",{handle,0},value) && value==0x102 &&
                call("SetEvent",{handle},value) && value==1 &&
                call("WaitForSingleObject",{handle,0},value) && value==0 &&
                call("WaitForSingleObject",{handle,0},value) && value==0x102;
        } else {
            ok=ok && call("WaitForSingleObject",{handle,0},value) && value==0 &&
                call("WaitForSingleObject",{handle,0},value) && value==0 &&
                call("ResetEvent",{handle},value) && value==1 &&
                call("WaitForSingleObject",{handle,0},value) && value==0x102;
        }
        const char* error=step;
        if(handle) { const bool closed=call("CloseHandle",{handle},value) && value==1; if(ok&&!closed) error="CloseHandle"; ok=ok&&closed; }
        report(id,id==1 ? "event_auto_reset_consumes_signal" : "event_manual_reset_retains_signal",ok,error);
    }
    fprintf(log,"batch_test_begin=03\n");
    uint32_t pointer=0,moved=0,value=0;
    std::array<uint8_t,17> pattern{}; for(unsigned i=0;i<pattern.size();++i) pattern[i]=uint8_t(i*7+3);
    std::array<uint8_t,65> bytes{};
    bool heap=setup && call("HeapAlloc",{0x00AB0000,0,17},pointer) && pointer &&
        cpu.write(pointer,pattern.data(),pattern.size());
    heap=heap && call("HeapReAlloc",{0x00AB0000,8,pointer,65},moved) && moved && moved!=pointer;
    // Track the returned allocation even if a later content assertion fails.
    const uint32_t live=moved ? moved : pointer;
    heap=heap && cpu.read(live,bytes.data(),bytes.size());
    if(heap) for(unsigned i=0;i<bytes.size();++i) heap=heap&&(bytes[i]==(i<pattern.size()?pattern[i]:0));
    heap=heap && call("HeapSize",{0x00AB0000,0,live},value) && value==65 &&
        call("HeapReAlloc",{0x00AB0000,0,live,9},value) && value==live &&
        call("HeapSize",{0x00AB0000,0,live},value) && value==9;
    const char* heap_error=heap ? "none" : "resize_size_or_content";
    if(live) { const bool freed=call("HeapFree",{0x00AB0000,0,live},value)&&value==1; if(heap&&!freed) heap_error="HeapFree"; heap=heap&&freed; }
    report(3,"heap_move_preserve_zero_grow_and_shrink",heap,heap_error);
    fprintf(log,"batch_test_begin=04\n");
    uint64_t frequency=0,first=0,second=0,filetime=0;
    wx86_set_lasterr(cpu,0x1234);
    const bool clocks=setup && call("GetSystemTimeAsFileTime",{scratch},value) &&
        cpu.read(scratch,&filetime,8) && filetime &&
        call("QueryPerformanceFrequency",{scratch+16},value) && value==1 &&
        cpu.read(scratch+16,&frequency,8) && frequency==1000000 &&
        call("QueryPerformanceCounter",{scratch+32},value) && value==1 &&
        cpu.read(scratch+32,&first,8) &&
        call("QueryPerformanceCounter",{scratch+48},value) && value==1 &&
        cpu.read(scratch+48,&second,8) && second>=first && wx86_get_lasterr(cpu)==0x1234;
    report(4,"utc_qpc_frequency_monotonic_and_last_error",clocks,"clock_output_or_last_error");
    fprintf(log,"batch_passed=%u\nbatch_failed=%u\nbatch_result=%s\n",passed,4-passed,passed==4?"passed":"failed");
    return passed==4;
}
