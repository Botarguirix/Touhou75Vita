#include "batch_checks.h"
#include "startup_services.h"
#include "runtime/cpu.h"
#include "runtime/pe_image.h"
#include "runtime/guest_thread.h"
#include "runtime/guest_thread_ctx.h"
#include <array>
#include <vector>
#include <initializer_list>
#include <cstring>

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
    constexpr unsigned total = 20;
    fprintf(log,"batch_total=%u\n",total);
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
    fprintf(log,"batch_test_begin=05\n");
    uint32_t closed=0;
    bool closed_ok=setup && call("CreateEventA",{0,0,0,0},closed) && closed;
    if(closed) {
        const bool removed=call("CloseHandle",{closed},value)&&value==1;
        closed_ok=closed_ok&&removed;
        if(removed) closed_ok=closed_ok && call("SetEvent",{closed},value) && value==0 &&
            wx86_get_lasterr(cpu)==6 && call("WaitForSingleObject",{closed,0},value) &&
            value==0xFFFFFFFFu && wx86_get_lasterr(cpu)==6;
    }
    report(5,"closed_event_rejected_with_error6",closed_ok,"closed_handle_return_or_error");

    fprintf(log,"batch_test_begin=06\n");
    uint32_t retained=0;
    std::array<uint8_t,17> retained_bytes{};
    bool inplace=setup && call("HeapAlloc",{0x00AB0000,0,17},retained) && retained &&
        cpu.write(retained,pattern.data(),pattern.size());
    wx86_set_lasterr(cpu,0x4321);
    inplace=inplace && call("HeapReAlloc",{0x00AB0000,0x10,retained,65},value) && !value &&
        wx86_get_lasterr(cpu)==0x4321 && call("HeapSize",{0x00AB0000,0,retained},value) && value==17 &&
        cpu.read(retained,retained_bytes.data(),retained_bytes.size()) && retained_bytes==pattern;
    if(retained) { const bool freed=call("HeapFree",{0x00AB0000,0,retained},value)&&value==1; inplace=inplace&&freed; }
    report(6,"heap_inplace_failure_retains_size_data_error",inplace,"original_allocation_changed");

    fprintf(log,"batch_test_begin=07\n");
    uint32_t slot=0xFFFFFFFFu;
    bool tls=setup && call("TlsAlloc",{},slot) && slot!=0xFFFFFFFFu &&
        call("TlsGetValue",{slot},value) && value==0 && wx86_get_lasterr(cpu)==0 &&
        call("TlsSetValue",{slot,0x13572468},value) && value==1 &&
        call("TlsGetValue",{slot},value) && value==0x13572468 && wx86_get_lasterr(cpu)==0;
    if(slot!=0xFFFFFFFFu) {
        const bool freed=call("TlsFree",{slot},value)&&value==1;
        tls=tls&&freed;
        if(freed) tls=tls && call("TlsGetValue",{slot},value) && !value && wx86_get_lasterr(cpu)==87;
    }
    report(7,"tls_zero_set_get_free_invalid_index",tls,"tls_value_lifecycle_or_error");

    fprintf(log,"batch_test_begin=08\n");
    const std::array<uint8_t,4> encoded{{0x82,0xA0,0x41,0}}; // CP932: hiragana A, ASCII A, NUL.
    const std::array<uint16_t,3> expected_wide{{0x3042,0x0041,0}};
    std::array<uint16_t,3> wide{};
    std::array<uint8_t,4> roundtrip{};
    uint32_t used_default=1;
    bool cp932=setup && cpu.write(scratch+128,encoded.data(),encoded.size()) &&
        call("MultiByteToWideChar",{932,8,scratch+128,0xFFFFFFFFu,0,0},value) && value==3 &&
        call("MultiByteToWideChar",{932,8,scratch+128,0xFFFFFFFFu,scratch+160,3},value) && value==3 &&
        cpu.read(scratch+160,wide.data(),sizeof(wide)) && wide==expected_wide &&
        cpu.write(scratch+224,&used_default,4) &&
        call("WideCharToMultiByte",{932,0,scratch+160,0xFFFFFFFFu,scratch+192,4,0,scratch+224},value) && value==4 &&
        cpu.read(scratch+192,roundtrip.data(),roundtrip.size()) && roundtrip==encoded &&
        cpu.read(scratch+224,&used_default,4) && !used_default;
    report(8,"cp932_japanese_ascii_nul_roundtrip",cp932,"conversion_size_data_or_default_flag");

    fprintf(log,"batch_test_begin=09\n");
    uint32_t guest_error=0;
    const bool errors=setup && call("SetLastError",{0x2468},value) &&
        call("GetLastError",{},value) && value==0x2468 &&
        cpu.read(wx86_cur_tib()+0x34,&guest_error,4) && guest_error==0x2468 &&
        call("GetCurrentThreadId",{},value) && value==8 &&
        call("GetLastError",{},value) && value==0x2468;
    report(9,"last_error_teb_roundtrip_preserved_by_thread_id",errors,"teb_error_or_thread_id");

    fprintf(log,"batch_test_begin=10\n");
    std::array<uint8_t,32> filename{}; filename.fill(0xCC);
    const char path[]="C:\\TH075\\TH075.exe";
    bool path_ok=setup && cpu.write(scratch+256,filename.data(),filename.size()) &&
        call("GetModuleFileNameA",{0,scratch+256,4},value) && value==4 &&
        cpu.read(scratch+256,filename.data(),filename.size()) &&
        !std::memcmp(filename.data(),path,4);
    if(path_ok) for(unsigned i=4;i<filename.size();++i) path_ok=path_ok&&filename[i]==0xCC;
    path_ok=path_ok && call("GetModuleFileNameA",{0,scratch+256,32},value) && value==sizeof(path)-1 &&
        cpu.read(scratch+256,filename.data(),filename.size()) &&
        !std::memcmp(filename.data(),path,sizeof(path));
    report(10,"module_path_truncation_guard_and_nul",path_ok,"module_path_length_guard_or_nul");
    fprintf(log,"batch_test_begin=11\n");
    uint32_t original_priority=0;
    bool priorities=setup && call("GetThreadPriority",{0x00AB4000},original_priority) && original_priority!=0x7FFFFFFF;
    const bool priority_known=priorities;
    for(int32_t level : {-15,-2,-1,0,1,2,15}) {
        uint32_t got=0;
        const bool ok=priorities && call("SetThreadPriority",{0x00AB4000,uint32_t(level)},value) && value==1 &&
            call("GetThreadPriority",{0x00AB4000},got) && got==uint32_t(level);
        priorities=priorities&&ok;
    }
    if(priority_known) { const bool restored=call("SetThreadPriority",{0x00AB4000,original_priority},value)&&value==1; priorities=priorities&&restored; }
    report(11,"worker_supported_priority_roundtrip",priorities,"priority_value_or_restore");
    fprintf(log,"batch_test_begin=12\n");
    uint32_t unchanged=0;
    const bool invalid_priority=setup && priority_known &&
        call("SetThreadPriority",{0x00AB4000,13},value) && !value && wx86_get_lasterr(cpu)==87 &&
        call("GetThreadPriority",{0x00AB4000},unchanged) && unchanged==original_priority;
    report(12,"invalid_priority_preserves_worker_state",invalid_priority,"priority_error_or_state_changed");
    fprintf(log,"batch_test_begin=13\n");
    const bool foreign_priority=setup && call("GetThreadPriority",{0x00AB4990},value) &&
        value==0x7FFFFFFF && wx86_get_lasterr(cpu)==6 &&
        call("SetThreadPriority",{0x00AB4990,0},value) && !value && wx86_get_lasterr(cpu)==6;
    report(13,"foreign_thread_priority_handle_rejected",foreign_priority,"invalid_thread_handle_error");

    fprintf(log,"batch_test_begin=14\n");
    const uint32_t main_tib=wx86_cur_tib();
    uint32_t worker_error=0;
    bool isolation=setup && cpu.read(0x00760034,&worker_error,4);
    wx86_set_lasterr(cpu,0xAA55);
    cpu.set_fs_base(0x00760000);wx86_set_main_tib(0x00760000);
    isolation=isolation && call("SetLastError",{0x55AA},value) &&
        call("GetLastError",{},value) && value==0x55AA &&
        call("GetCurrentThreadId",{},value) && value==12;
    cpu.set_fs_base(saved.fs_base);wx86_set_main_tib(main_tib);
    isolation=isolation && call("GetLastError",{},value) && value==0xAA55;
    const bool error_restored=cpu.write(0x00760034,&worker_error,4);
    report(14,"worker_main_teb_last_error_isolation",isolation&&error_restored,"teb_binding_or_isolation");

    fprintf(log,"batch_test_begin=15\n");
    uint32_t shared_slot=0xFFFFFFFFu,worker_slot=0;
    bool tls_isolation=setup && call("TlsAlloc",{},shared_slot) && shared_slot<64 &&
        cpu.read(0x00760E10+4*shared_slot,&worker_slot,4) &&
        call("TlsSetValue",{shared_slot,0xAAAAAAAA},value) && value==1;
    cpu.set_fs_base(0x00760000);wx86_set_main_tib(0x00760000);
    tls_isolation=tls_isolation && call("TlsSetValue",{shared_slot,0xBBBBBBBB},value) && value==1 &&
        call("TlsGetValue",{shared_slot},value) && value==0xBBBBBBBB;
    cpu.set_fs_base(saved.fs_base);wx86_set_main_tib(main_tib);
    tls_isolation=tls_isolation && call("TlsGetValue",{shared_slot},value) && value==0xAAAAAAAA;
    if(shared_slot<64) {
        const bool slot_restored=cpu.write(0x00760E10+4*shared_slot,&worker_slot,4);
        const bool freed=call("TlsFree",{shared_slot},value)&&value==1;
        tls_isolation=tls_isolation&&slot_restored&&freed;
    }
    report(15,"tls_slot_values_isolated_between_tebs",tls_isolation,"per_thread_tls_or_cleanup");

    fprintf(log,"batch_test_begin=16\n");
    const uint8_t malformed[2]={0x82,0};
    std::array<uint8_t,8> guard{};guard.fill(0xCD);
    std::array<uint8_t,8> guard_copy{};
    const bool strict=setup && cpu.write(scratch+320,malformed,2) && cpu.write(scratch+352,guard.data(),8) &&
        call("MultiByteToWideChar",{932,8,scratch+320,0xFFFFFFFFu,scratch+352,4},value) &&
        !value && wx86_get_lasterr(cpu)==1113 && cpu.read(scratch+352,guard_copy.data(),8) && guard_copy==guard;
    report(16,"cp932_invalid_lead_strict_error_no_write",strict,"strict_error_or_output_modified");
    fprintf(log,"batch_test_begin=17\n");
    const bool short_conversion=setup && cpu.write(scratch+128,encoded.data(),encoded.size()) && cpu.write(scratch+352,guard.data(),8) &&
        call("MultiByteToWideChar",{932,8,scratch+128,0xFFFFFFFFu,scratch+352,2},value) &&
        !value && wx86_get_lasterr(cpu)==122 && cpu.read(scratch+352,guard_copy.data(),8) && guard_copy==guard;
    report(17,"cp932_small_output_error122_no_write",short_conversion,"capacity_error_or_output_modified");

    fprintf(log,"batch_test_begin=18\n");
    uint32_t zero_block=0;
    bool zero_heap=setup && call("HeapAlloc",{0x00AB0000,0,0},zero_block) && zero_block &&
        call("HeapSize",{0x00AB0000,0,zero_block},value) && !value;
    if(zero_block) {const bool freed=call("HeapFree",{0x00AB0000,0,zero_block},value)&&value==1;zero_heap=zero_heap&&freed;}
    report(18,"zero_size_heap_valid_pointer_and_size",zero_heap,"zero_size_allocation_contract");
    fprintf(log,"batch_test_begin=19\n");
    uint32_t aligned=0;
    std::array<uint8_t,15> grown{};
    bool grow=setup && call("HeapAlloc",{0x00AB0000,0,9},aligned) && aligned &&
        cpu.write(aligned,pattern.data(),9) &&
        call("HeapReAlloc",{0x00AB0000,8,aligned,15},value) && value==aligned &&
        cpu.read(aligned,grown.data(),grown.size());
    if(grow)for(unsigned i=0;i<grown.size();++i)grow=grow&&(grown[i]==(i<9?pattern[i]:0));
    if(aligned){const bool freed=call("HeapFree",{0x00AB0000,0,aligned},value)&&value==1;grow=grow&&freed;}
    report(19,"heap_inplace_growth_preserve_and_zero_tail",grow,"aligned_growth_pointer_or_data");
    fprintf(log,"batch_test_begin=20\n");
    const bool zero_path=setup && cpu.write(scratch+352,guard.data(),8) &&
        call("GetModuleFileNameA",{0,scratch+352,0},value) && !value && wx86_get_lasterr(cpu)==0 &&
        cpu.read(scratch+352,guard_copy.data(),8) && guard_copy==guard;
    report(20,"module_path_zero_capacity_preserves_guard",zero_path,"zero_capacity_return_error_or_write");
    fprintf(log,"batch_passed=%u\nbatch_failed=%u\nbatch_result=%s\n",passed,total-passed,passed==total?"passed":"failed");
    return passed==total;
}
