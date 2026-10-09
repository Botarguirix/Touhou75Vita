#include "worker_probe.h"
#include "startup_services.h"
#include "runtime/cpu.h"
#include "runtime/pe_image.h"
#include "runtime/guest_thread.h"
#include "runtime/guest_thread_ctx.h"
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <array>

bool run_worker_probe(d2rt::Cpu& cpu, const d2rt::PeImage& image, uint32_t frame,
                      StartupServices& services, StartupWorker& state, FILE* log) {
    const bool audio=state.id==13;
    const uint32_t stack=audio?0x00A20000:0x00A00000, top=stack+0x20000, teb=audio?0x00770000:0x00760000;
    constexpr uint32_t sentinel = 0x00BFFFC0, traps = 0x00B00000;
    uint32_t args[6] = {};
    fprintf(log, "worker_probe_scope=original_worker_to_first_blocking_wait\n");
    if (frame < 0x00800000 || uint64_t(frame) + 28 > 0x00A00000 ||
        !cpu.read(frame + 4, args, sizeof(args))) return false;
    for (unsigned i = 0; i < 6; ++i) fprintf(log, "worker_create_arg%u=0x%08X\n", i, args[i]);
    if (args[0] || args[1] || args[2] != (audio?0x00407EF0u:0x00423B40u) || args[3] || args[4] || args[5] != (audio?0x0067139Cu:0x0068BE3Cu)) {
        fprintf(log, "worker_probe_result=unsupported_create_frame\n"); return false;
    }
    d2rt::X86Context main{};
    cpu.save_context(main);
    const uint32_t main_tib = wx86_cur_tib();
    main.fs_base=main_tib; // Cpu::save_context leaves this to the scheduler.
    struct Restore {
        d2rt::Cpu& cpu; const d2rt::X86Context& context; uint32_t tib;
        ~Restore() { cpu.load_context(context); wx86_set_main_tib(tib); }
    } restore{cpu, main, main_tib};
    if (!cpu.fs_base_is_direct() || !cpu.map(stack, top-stack, nullptr, d2rt::P_RW) ||
        !cpu.map(teb, 0x1000, nullptr, d2rt::P_RW)) {
        fprintf(log, "worker_probe_result=context_map_failed\n"); return false;
    }
    const std::array<std::array<uint32_t,2>,8> fields = {{{0,0xFFFFFFFF}, {4,top}, {8,stack},
        {0x18,teb}, {0x20,4}, {0x24,state.id}, {0x30,0x00731000}, {0x34,0}}};
    for (const auto& field : fields) {
        uint32_t copy = 0;
        if (!cpu.write(teb+field[0], &field[1], 4) ||
            !cpu.read(teb+field[0], &copy, 4) || copy != field[1]) return false;
    }
    const uint32_t esp = top-0x1000-8, call[2] = {sentinel,args[3]};
    if (!cpu.write(esp, call, sizeof(call))) return false;
    d2rt::X86Context worker{};
    worker.eip = args[2]; worker.gpr[d2rt::R_ESP] = esp; worker.fs_base = teb;
    cpu.load_context(worker); wx86_set_main_tib(teb);
    bool hit = false, valid = false;unsigned imports_serviced=0;
    cpu.set_trap(traps, 0x00C00000, [&](d2rt::Cpu& c, uint32_t trap) {
        if (trap == sentinel) { fprintf(log, "worker_probe_stop=worker_returned\n"); return false; }
        if (trap < traps || (trap-traps)%16 || (trap-traps)/16 >= image.imports().size()) return false;
        const auto& imp = image.imports()[(trap-traps)/16];
        const uint32_t sp = c.reg(d2rt::R_ESP);
        uint32_t ret = 0;
        hit = true;
        valid = sp >= stack && uint64_t(sp)+4 <= top && c.read(sp,&ret,4) &&
            ret >= image.load_base() && uint64_t(ret) < uint64_t(image.load_base())+image.image_size();
        fprintf(log, "worker_probe_import=%s!%s\n", imp.dll.c_str(), imp.name.c_str());
        fprintf(log, "worker_probe_return_va=0x%08X\n", ret);
        fprintf(log, "worker_probe_esp=0x%08X\n", sp);
        fprintf(log, "worker_probe_frame=%s\n", valid ? "valid" : "invalid");
        fprintf(log, "worker_probe_import_executed=no\n");
        uint32_t wait_args[2] = {};
        uint32_t tls_slot = 1;
        if (valid && imp.dll == "KERNEL32.dll" && imp.name == "WaitForSingleObject" &&
            ret == (audio?0x004080FAu:0x00423B8Cu) && uint64_t(sp)+12 <= top && c.read(sp+4,wait_args,8) &&
            wait_args[1] && services.event_unsignaled(wait_args[0]) &&
            c.read(teb+0xE10,&tls_slot,4) && !tls_slot) {
            if(wait_args[1]!=(audio?80u:16u))return false;
            state.event = wait_args[0]; state.timeout_ms = wait_args[1];
            state.wait_started_us = sceKernelGetProcessTimeWide();
            state.blocked = true;
            c.save_context(state.context); // Preserve the unexecuted wait frame.
            state.context.fs_base=teb;
            fprintf(log, "worker_wait_handle=0x%08X\n", state.event);
            fprintf(log, "worker_wait_timeout_ms=%u\n", state.timeout_ms);
            fprintf(log, "worker_wait_state=blocked_saved_context\n");
            fprintf(log, "worker_tls_slot0=zero\n");
            return false;
        }
        if(audio && valid && ++imports_serviced<=16 && services.call(imp)==StartupServiceResult::Serviced){
            fprintf(log,"worker_probe_import_executed=serviced id:%u\n",state.id);return true;
        }
        return false;
    });
    cpu.take_limit_hit(); cpu.set_run_limit(4096);
    const uint64_t start = sceKernelGetProcessTimeWide();
    const char* fault = nullptr;
    const bool stopped = cpu.run(args[2], &fault);
    const bool limit = cpu.take_limit_hit();
    fprintf(log, "worker_probe_elapsed_us=%llu\n", (unsigned long long)(sceKernelGetProcessTimeWide()-start));
    fprintf(log, "worker_probe_limit_hit=%s\n", limit ? "yes" : "no");
    if (!stopped) fprintf(log, "worker_probe_fault=%s\n", fault ? fault : "unknown");
    const bool passed = stopped && hit && valid && !limit && state.blocked;
    fprintf(log, "worker_probe_result=%s\n", passed ? "blocked_context_saved" : "failed");
    fprintf(log, "worker_scheduler=bounded_initial_handoff\n");
    return passed;
}

bool wake_worker_slice(d2rt::Cpu& cpu,const d2rt::PeImage& image,StartupServices& services,
                       StartupWorker& worker,FILE* log) {
    const uint32_t stack=worker.id==13?0x00A20000:0x00A00000,top=stack+0x20000;
    if(!worker.blocked || worker.timeout_ms!=(worker.id==13?80u:16u) || !services.event_unsignaled(worker.event)) {
        fprintf(log,"worker_wake_result=unsupported_wait_state\n"); return false;
    }
    uint64_t now=sceKernelGetProcessTimeWide();
    if(now<worker.wait_started_us)return false;
    if(!startup_wait::timeout_elapsed(worker.wait_started_us,worker.timeout_ms,now)) {
        const uint64_t remaining=uint64_t(worker.timeout_ms)*1000-(now-worker.wait_started_us);
        const int rc=sceKernelDelayThread(uint32_t(remaining));
        if(rc<0)return false;
    }
    now=sceKernelGetProcessTimeWide();
    if(!startup_wait::timeout_elapsed(worker.wait_started_us,worker.timeout_ms,now))return false;
    d2rt::X86Context main{}; cpu.save_context(main);
    const uint32_t main_tib=wx86_cur_tib();
    main.fs_base=main_tib;
    struct Restore {
        d2rt::Cpu& cpu; const d2rt::X86Context& ctx; uint32_t tib;
        ~Restore(){
            cpu.set_trap(0,0,[](d2rt::Cpu&,uint32_t){return false;});
            cpu.load_context(ctx);wx86_set_main_tib(tib);
        }
    } restore{cpu,main,main_tib};
    cpu.load_context(worker.context); wx86_set_main_tib(worker.context.fs_base);
    uint32_t wait[3]{};
    const uint32_t esp=cpu.reg(d2rt::R_ESP);
    if(esp<stack || uint64_t(esp)+12>top || !cpu.read(esp,wait,12) ||
       wait[0]!=(worker.id==13 ? 0x004080FAu:0x00423B8Cu) ||
       wait[1]!=worker.event || wait[2]!=worker.timeout_ms ||
       worker.context.fs_base!=(worker.id==13 ? 0x00770000u:0x00760000u))return false;
    const uint32_t ret=wait[0];
    cpu.trap_epilogue(0x102,12,ret); // Actual elapsed timeout, not an invented signal.
    worker.blocked=false;
    fprintf(log,"worker_wake_elapsed_us=%llu\n",(unsigned long long)(now-worker.wait_started_us));
    fprintf(log,"worker_wake_reason=real_timeout ms:%u id:%u\nworker_wait_result=0x00000102\n",worker.timeout_ms,worker.id);
    fprintf(log,"worker_dispatch_priority=%d\n",worker.priority);
    bool boundary=false,failed=false;
    unsigned calls=0;
    cpu.set_trap(0x00B00000,0x00C00000,[&](d2rt::Cpu& c,uint32_t trap){
        if(trap<0x00B00000 || (trap-0x00B00000)%16 || (trap-0x00B00000)/16>=image.imports().size()) {failed=true;return false;}
        const auto& imp=image.imports()[(trap-0x00B00000)/16];
        const uint32_t sp=c.reg(d2rt::R_ESP);
        uint32_t return_va=0;
        if(sp<stack || uint64_t(sp)+12>top || !c.read(sp,&return_va,4) ||
           return_va<image.load_base() || uint64_t(return_va)>=uint64_t(image.load_base())+image.image_size()) {failed=true;return false;}
        fprintf(log,"worker_resumed_import=%s!%s\n",imp.dll.c_str(),imp.name.c_str());
        fprintf(log,"worker_resumed_return_va=0x%08X\n",return_va);
        if(imp.dll=="KERNEL32.dll" && imp.name=="WaitForSingleObject") {
            uint32_t args[2]={}; if(!c.read(sp+4,args,8)){failed=true;return false;}
            if(args[1] && services.event_unsignaled(args[0])) {
                if(return_va!=(worker.id==13 ? 0x004080FAu:0x00423B8Cu) ||
                   args[0]!=worker.event || args[1]!=(worker.id==13 ? 80u:16u)) {
                    failed=true;return false;
                }
                worker.event=args[0];worker.timeout_ms=args[1];worker.wait_started_us=sceKernelGetProcessTimeWide();
                worker.blocked=true;c.save_context(worker.context);boundary=true;
                worker.context.fs_base=wx86_cur_tib();
                fprintf(log,"worker_wait_state=reblocked_saved_context\n");return false;
            }
        }
        if(++calls>32){failed=true;return false;}
        const auto result=services.call(imp);
        if(result==StartupServiceResult::Serviced)return true;
        if(result==StartupServiceResult::ContractFailure){failed=true;return false;}
        worker.unsupported_boundary=true;c.save_context(worker.context);boundary=true;
        worker.context.fs_base=wx86_cur_tib();
        fprintf(log,"worker_stop_import=%s!%s\nstartup_stop_import=%s!%s\n",
            imp.dll.c_str(),imp.name.c_str(),imp.dll.c_str(),imp.name.c_str());
        fprintf(log,"startup_stop_worker_eip=0x%08X esp:0x%08X id:%u\n",c.reg(d2rt::R_EIP),sp,worker.id);
        fprintf(log,"startup_stop_thread=worker\n");return false;
    });
    cpu.take_limit_hit();cpu.set_run_limit(4096);
    const char* fault=nullptr;
    const bool stopped=cpu.run(ret,&fault);
    const bool limit=cpu.take_limit_hit();
    if(fault)fprintf(log,"worker_wake_fault=%s\n",fault);
    const bool passed=stopped&&!limit&&!failed&&boundary;
    fprintf(log,"worker_wake_result=%s\n",passed?"next_boundary_reached":"failed");
    return passed;
}
