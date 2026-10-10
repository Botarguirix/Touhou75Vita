#include "startup_probe.h"
#include "startup_resume_policy.h"
#include "startup_limits.h"
#include "guest_string_probe.h"
#include "startup_services.h"
#include "d3d8_bootstrap.h"
#include "dinput8_bridge.h"
#include "dsound_bootstrap.h"
#include "worker_probe.h"
#include "seh_chain.h"
#include "thread_smoke.h"
#include "runtime/cpu.h"
#include "runtime/guest_thread_ctx.h"
#include "runtime/pe_image.h"
#include "platform/vita_host.h"
#include <psp2/kernel/error.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <atomic>
#include <cstring>
#include <string>

namespace {
constexpr uint32_t kStack = 0x00800000, kStackEnd = 0x00A00000;
constexpr uint32_t kTrap = 0x00B00000, kTrapEnd = 0x00C00000;
constexpr uint32_t kSentinel = 0x00BFFFF0, kEntry = 0x0064232C;
// Allow the original logo's 181 updates and scene handoff, retaining a
// separately bounded native watchdog as the hard safety bound.
constexpr uint64_t kRunBudget = 65536, kTimeoutUs = startup_limits::watchdog_timeout_us;
// The VitaSDK example and the pinned WinVita native threads use this class.
// 0x10000040 used by r1 was rejected on hardware with ILLEGAL_PRIORITY.
constexpr int kWatchdogPriority = 0x10000100;
const char* const kWatchdogPath = "ux0:data/TH075Vita/iteration79-watchdog.log";

bool stack_range(uint32_t address, uint32_t size) {
    return address >= kStack && uint64_t(address) + size <= kStackEnd;
}

// Known Japanese EXE: 0041CAE0 builds 3600 floats at 006884C0.
// Its saved return address is 00602D0F. Observe only; never fill the table
// or replace the original x87 cosine routine with a native approximation.
bool cosine_progress(d2rt::Cpu& cpu, uint32_t& index) {
    uint32_t frame = cpu.reg(d2rt::R_EBP);
    for (unsigned depth = 0; depth < 64; ++depth) {
        uint32_t next = 0, ret = 0;
        if ((frame & 3) || frame < kStack + 4 || !stack_range(frame, 8) ||
            !cpu.read(frame, &next, 4) || !cpu.read(frame + 4, &ret, 4)) return false;
        if (ret == 0x00602D0F) {
            return cpu.read(frame - 4, &index, 4) && index < 3600;
        }
        if (next <= frame) return false;
        frame = next;
    }
    return false;
}
bool file_u32(const std::vector<uint8_t>& file, uint64_t offset, uint32_t& out) {
    if (offset > file.size() || file.size() - offset < 4) return false;
    const auto* p = file.data() + size_t(offset);
    out = uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
          (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
    return true;
}

bool original_code_address(const std::vector<uint8_t>& exe, uint32_t nt,
                           const d2rt::PeImage& image, uint32_t ip) {
    uint32_t header = 0, optional = 0;
    if (!file_u32(exe, uint64_t(nt) + 4, header) ||
        !file_u32(exe, uint64_t(nt) + 20, optional)) return false;
    const unsigned sections = header >> 16;
    if (!sections || sections > 96 || ip < image.load_base() ||
        uint64_t(ip) >= uint64_t(image.load_base()) + image.image_size()) return false;
    const uint64_t table = uint64_t(nt) + 24 + (optional & 0xFFFF);
    for (unsigned i = 0; i < sections; ++i) {
        const uint64_t section = table + i * 40;
        uint32_t size = 0, rva = 0, raw = 0, flags = 0;
        if (!file_u32(exe, section + 8, size) || !file_u32(exe, section + 12, rva) ||
            !file_u32(exe, section + 16, raw) || !file_u32(exe, section + 36, flags)) return false;
        const uint64_t begin = uint64_t(image.load_base()) + rva;
        const uint64_t end = begin + (size > raw ? size : raw);
        if ((flags & 0x20000000) && ip >= begin && ip < end) return true;
    }
    return false;
}

// Owns a separate, unbuffered file. It never acquires runtime/CPU locks or
// mutates guest state. A dynablock's internal loop can evade the block budget;
// in that case this native thread ends the process instead of unsafe unwinding.
class StartupWatchdog {
public:
    explicit StartupWatchdog(const uint32_t* ip) : ip_(ip) {}
    ~StartupWatchdog() { finish(); }
    bool start(FILE* log) {
        report_ = fopen(kWatchdogPath, "wb");
        if (!report_) {
            fprintf(log, "startup_watchdog_error=log_open_failed\n");
            return false;
        }
        setvbuf(report_, nullptr, _IONBF, 0);
        fprintf(report_, "watchdog_revision=iteration79\n");
        fprintf(report_, "watchdog_scope=original_entrypoint_only\n");
        fprintf(report_, "watchdog_timeout_us=%llu\n", (unsigned long long)kTimeoutUs);
        fprintf(report_, "watchdog_result=prepared\n");
        fprintf(log, "startup_watchdog_priority=0x%08X\n", (unsigned)kWatchdogPriority);
        fprintf(report_, "watchdog_priority=0x%08X\n", (unsigned)kWatchdogPriority);
        deadline_ = sceKernelGetProcessTimeWide() + kTimeoutUs;
        thread_ = sceKernelCreateThread("TH075 startup guard", entry,
            kWatchdogPriority, 0x10000, 0, SCE_KERNEL_CPU_MASK_USER_1, nullptr);
        if (thread_ < 0) {
            fprintf(log, "startup_watchdog_create_rc=0x%08X\n", (unsigned)thread_);
            if ((uint32_t)thread_ == (uint32_t)SCE_KERNEL_ERROR_ILLEGAL_PRIORITY)
                fprintf(log, "startup_watchdog_error=illegal_priority\n");
            fprintf(report_, "watchdog_result=create_failed\n");
            fclose(report_); report_ = nullptr;
            return false;
        }
        StartupWatchdog* self = this;
        const int rc = sceKernelStartThread(thread_, sizeof(self), &self);
        if (rc < 0) {
            fprintf(log, "startup_watchdog_start_rc=0x%08X\n", (unsigned)rc);
            fprintf(report_, "watchdog_result=start_failed\n");
            sceKernelDeleteThread(thread_); thread_ = -1;
            fclose(report_); report_ = nullptr;
            return false;
        }
        const uint64_t ready_deadline = sceKernelGetProcessTimeWide() + 250000;
        while (!ready_.load() && sceKernelGetProcessTimeWide() < ready_deadline)
            sceKernelDelayThread(1000);
        if (!ready_.load()) {
            fprintf(log, "startup_watchdog_error=thread_not_ready\n");
            finish();
            return false;
        }
        if (!placement_ok_) {
            fprintf(log, "startup_watchdog_error=affinity_failed\n");
            finish();
            return false;
        }
        fprintf(log, "startup_watchdog=armed\n");
        fprintf(log, "startup_watchdog_timeout_us=%llu\n", (unsigned long long)kTimeoutUs);
        return true;
    }
    void finish() {
        if (thread_ < 0) return;
        unsigned running = 0;
        if (!state_.compare_exchange_strong(running, 1) && running == 2)
            sceKernelExitProcess(124);
        // The guard sleeps for at most 10 ms. Do not free state until joined.
        int status = 0;
        SceUInt timeout = 1000000;
        const int rc = sceKernelWaitThreadEnd(thread_, &status, &timeout);
        if (rc < 0) {
            // The guard may still reference this object; process exit is the
            // only cleanup that doesn't introduce a use-after-free.
            sceKernelExitProcess(125);
        }
        sceKernelDeleteThread(thread_); thread_ = -1;
        fclose(report_); report_ = nullptr;
    }
private:
    static int entry(SceSize size, void* arg) {
        if (size != sizeof(StartupWatchdog*) || !arg) return 1;
        StartupWatchdog* self = nullptr;
        memcpy(&self, arg, sizeof(self));
        if (!self) return 1;
        // WinVita requires each native thread to pin itself after start.
        unsigned mask = 0;
        const int pin_rc = wx86_vita_pin_self(SCE_KERNEL_CPU_MASK_USER_1, &mask);
        fprintf(self->report_, "watchdog_pin_rc=0x%08X\n", (unsigned)pin_rc);
        fprintf(self->report_, "watchdog_affinity_readback=0x%08X\n", mask);
        fprintf(self->report_, "watchdog_actual_priority=0x%08X\n",
            (unsigned)sceKernelGetThreadCurrentPriority());
        self->placement_ok_ = pin_rc >= 0;
        self->ready_.store(true);
        if (!self->placement_ok_) {
            fprintf(self->report_, "watchdog_result=affinity_failed\n");
            return 1;
        }
        while (self->state_.load() == 0) {
            if (sceKernelGetProcessTimeWide() >= self->deadline_) {
                unsigned running = 0;
                if (self->state_.compare_exchange_strong(running, 2)) {
                    fprintf(self->report_, "watchdog_result=timeout\n");
                    // ip_ptr() explicitly exposes stable storage for watchdog
                    // sampling; no Cpu::reg() call on the watchdog's thread.
                    if (self->ip_) fprintf(self->report_, "watchdog_sampled_eip=0x%08X\n",
                        __atomic_load_n(self->ip_, __ATOMIC_RELAXED));
                    fflush(self->report_);
                    sceKernelExitProcess(124);
                    return 124;
                }
            }
            sceKernelDelayThread(10000);
        }
        fprintf(self->report_, "watchdog_result=disarmed\n");
        return 0;
    }
    const uint32_t* ip_;
    std::atomic<unsigned> state_{0}; // running, finished, timed out
    std::atomic<bool> ready_{false};
    bool placement_ok_ = false; // published by ready_'s release/acquire
    uint64_t deadline_ = 0;
    SceUID thread_ = -1;
    FILE* report_ = nullptr;
};

struct TrapCleanup {
    d2rt::Cpu& cpu;
    ~TrapCleanup() {
        cpu.set_run_limit(0);
        cpu.set_trap(0, 0, [](d2rt::Cpu&, uint32_t) { return false; });
    }
};

// Keep the guest on a different core from the equal-priority watchdog.
// The watchdog sleeps normally; it must still run if a guest block spins.
struct RunnerPlacement {
    FILE* log;
    int old_mask = -1;
    bool pinned = false;
    bool pin() {
        old_mask = sceKernelGetThreadCpuAffinityMask(sceKernelGetThreadId());
        if (old_mask < 0) {
            fprintf(log, "startup_runner_affinity_query_rc=0x%08X\n", (unsigned)old_mask);
            return false;
        }
        unsigned mask = 0;
        const int rc = wx86_vita_pin_self(SCE_KERNEL_CPU_MASK_USER_0, &mask);
        pinned = rc >= 0;
        fprintf(log, "startup_runner_pin_rc=0x%08X\n", (unsigned)rc);
        fprintf(log, "startup_runner_affinity_readback=0x%08X\n", mask);
        return pinned;
    }
    ~RunnerPlacement() {
        if (pinned) {
            unsigned mask = 0;
            const int rc = wx86_vita_pin_self(old_mask, &mask);
            fprintf(log, "startup_runner_affinity_restore_rc=0x%08X\n", (unsigned)rc);
        }
    }
};
}

bool run_startup_probe(d2rt::Cpu& cpu, const d2rt::PeImage& image,
                       const std::vector<uint8_t>& exe, FILE* log) {
    fprintf(log, "startup_scope=original_exe_crt_single_thread_sync\n");
    fprintf(log, "startup_import_policy=serve_startup_and_heap_contracts_stop_before_unknown_import\n");
    fprintf(log, "game_bootable=not_yet_established\n");
    // This checkpoint doesn't initialize PE static TLS or call its callbacks.
    // Refuse an executable needing that setup rather than skip it silently.
    uint32_t nt = 0, directories = 0, tls_rva = 0, tls_size = 0;
    if (!file_u32(exe, 0x3C, nt) || !file_u32(exe, uint64_t(nt) + 24 + 92, directories) ||
        (directories > 9 &&
         (!file_u32(exe, uint64_t(nt) + 24 + 96 + 9 * 8, tls_rva) ||
          !file_u32(exe, uint64_t(nt) + 24 + 100 + 9 * 8, tls_size)))) {
        fprintf(log, "startup_result=invalid_tls_directory\n"); return false;
    }
    fprintf(log, "startup_pe_tls_rva=0x%08X\n", tls_rva);
    fprintf(log, "startup_pe_tls_size=%u\n", tls_size);
    if (tls_rva || tls_size || image.entry_va() != kEntry ||
        image.load_base() != 0x00400000 || image.imports().empty() ||
        image.imports().size() >= (kSentinel - kTrap) / 16) {
        fprintf(log, "startup_result=unsupported_image_layout\n"); return false;
    }
    // The smoke bridge patched its own PE copy. Restore the original image
    // and invalidate all earlier translations before establishing this phase.
    cpu.discard_code(image.load_base(), image.image_size());
    cpu.discard_code(0x00713000, 0x1000);
    if (!cpu.map(image.load_base(), image.image_size(), image.image().data(), d2rt::P_RWX) ||
        !cpu.map(kStack, kStackEnd - kStack, nullptr, d2rt::P_RW)) {
        fprintf(log, "startup_result=map_failed\n"); return false;
    }
    ThreadSmoke thread(cpu, log);
    if (!thread.initialize()) {
        fprintf(log, "startup_result=context_failed\n"); return false;
    }
    StartupServices services(cpu, image, log);
    D3D8Bootstrap d3d8(cpu,log);
    DirectInput8Bridge input(cpu,log);
    DirectSoundBootstrap sound(cpu,log);
    TrapCleanup cleanup{cpu};
    if (image.imports().size() > (StartupServices::processor_feature_trap - kTrap) / 16) {
        fprintf(log, "startup_result=trap_space_exhausted\n"); return false;
    }
    for (size_t i = 0; i < image.imports().size(); ++i) {
        const uint32_t target = kTrap + uint32_t(i) * 16;
        const uint32_t iat = image.imports()[i].iat_va;
        if (iat < image.load_base() || uint64_t(iat) + 4 >
            uint64_t(image.load_base()) + image.image_size() ||
            !cpu.write(iat, &target, 4)) {
            fprintf(log, "startup_result=iat_write_failed\n"); return false;
        }
        uint32_t copy = 0;
        if (!cpu.read(iat, &copy, 4) || copy != target) {
            fprintf(log, "startup_result=iat_readback_failed\n"); return false;
        }
    }
    fprintf(log, "startup_iat_intercept_count=%u\n", (unsigned)image.imports().size());
    fprintf(log, "startup_guest_image=restored_from_verified_pe\n");
    fprintf(log, "startup_text_patch=none\n");
    bool import_hit = false, original_call_valid = false;
    bool expected_boundary = false, service_failed = false;
    bool game_entry_chain_verified = false;
    unsigned main_import_calls = 0;
    uint32_t worker_create_frame = 0;
    StartupWorker worker;
    StartupWorker audio_worker;audio_worker.handle=0x00AB4010;audio_worker.id=13;
    uint32_t audio_create_frame=0;
    bool priority_dispatch_requested = false;
    const d2rt::ImportRef dynamic_critical = {
        "KERNEL32.dll", "InitializeCriticalSectionAndSpinCount", 0, 0, 0};
    const d2rt::ImportRef dynamic_processor = {
        "KERNEL32.dll", "IsProcessorFeaturePresent", 0, 0, 0};
    unsigned boundaries_since_flush=0;
    fprintf(log,"startup_logging=all_calls_recorded flush_every:32_successful_callbacks flush_on:stop_or_slice\n");
    auto startup_trap = [&](d2rt::Cpu& c, uint32_t trap) {
        struct FlushBoundary {
            FILE* file;unsigned& count;bool continuing=false;
            ~FlushBoundary() { if(!continuing || ++count>=32){fflush(file);count=0;} }
        } flush_boundary{log,boundaries_since_flush};
        if(sound.owns(trap)) {
            ++main_import_calls;import_hit=true;
            const auto result=sound.call(trap);
            if(result==StartupServiceResult::Serviced){flush_boundary.continuing=true;return true;}
            service_failed=result==StartupServiceResult::ContractFailure;
            expected_boundary=!service_failed;
            fprintf(log,"startup_stop_import=%s::%s\n",sound.interface_name(trap),sound.method_name(trap));
            fprintf(log,"startup_stop_trap_va=0x%08X\n",trap);
            return false;
        }
        if(input.owns(trap)) {
            ++main_import_calls;import_hit=true;
            const auto result=input.call(trap);
            if(result==StartupServiceResult::Serviced){flush_boundary.continuing=true;return true;}
            service_failed=result==StartupServiceResult::ContractFailure;
            expected_boundary=!service_failed;
            fprintf(log,"startup_stop_import=%s::%s\n",input.interface_name(trap),input.method_name(trap));
            fprintf(log,"startup_stop_trap_va=0x%08X\n",trap);
            return false;
        }
        if(d3d8.owns_trap(trap)) {
            ++main_import_calls; import_hit=true;
            const auto result=d3d8.call(trap);
            if(result==StartupServiceResult::Serviced){flush_boundary.continuing=true;return true;}
            service_failed=result==StartupServiceResult::ContractFailure;
            expected_boundary=!service_failed;
            fprintf(log,"startup_stop_import=%s::%s\n",d3d8.interface_name(trap),d3d8.method_name(trap));
            fprintf(log,"startup_stop_trap_va=0x%08X\n",trap);
            return false;
        }
        if (trap == kSentinel) {
            fprintf(log, "startup_stop=entrypoint_returned\n"); return false;
        }
        const bool feature_export = trap == StartupServices::processor_feature_trap;
        const bool dynamic = trap == StartupServices::critical_init_trap || feature_export;
        if (!dynamic && (trap < kTrap || (trap - kTrap) % 16 ||
            (trap - kTrap) / 16 >= image.imports().size())) {
            fprintf(log, "startup_stop=unknown_trap\n"); return false;
        }
        const auto& imp = feature_export ? dynamic_processor :
            (dynamic ? dynamic_critical : image.imports()[(trap - kTrap) / 16]);
        if (dynamic) fprintf(log, "startup_dynamic_export_called=yes\n");
        const bool first = !import_hit;
        ++main_import_calls;
        import_hit = true;
        const std::string tag = imp.dll + "!" +
            (imp.name.empty() ? ("#" + std::to_string(imp.ordinal)) : imp.name);
        const uint32_t esp = c.reg(d2rt::R_ESP);
        uint32_t ret = 0, arg = 0, seh = 0, prev = 0, handler = 0;
        const bool frame_ok = stack_range(esp, 8) && c.read(esp, &ret, 4) &&
            c.read(esp + 4, &arg, 4);
        unsigned seh_depth = 0;
        const bool seh_ok = c.read(kDiagnosticTeb, &seh, 4) &&
            validate_seh_chain(seh, kStack, kStackEnd, image.load_base(),
                image.load_base() + image.image_size(),
                [&](uint32_t address, uint32_t& value) { return c.read(address, &value, 4); },
                seh_depth) && c.read(seh, &prev, 4) && c.read(seh + 4, &handler, 4);
        if (first) {
            fprintf(log, "startup_first_import=%s\n", tag.c_str());
            fprintf(log, "startup_first_iat_va=0x%08X\n", imp.iat_va);
            uint32_t size = 0;
            original_call_valid = imp.dll == "KERNEL32.dll" && imp.name == "GetVersionExA" &&
                imp.iat_va == 0x00657090 && ret == 0x00642352 && frame_ok && seh_ok &&
                prev == 0xFFFFFFFF && handler == 0x00645468 &&
                stack_range(arg, 148) && c.read(arg, &size, 4) && size == 148;
            fprintf(log, "startup_entry_checkpoint=%s\n", original_call_valid ? "passed" : "failed");
        }
        fprintf(log, "startup_import_call=%s\n", tag.c_str());
        fprintf(log, "startup_return_va=0x%08X\n", ret);
        auto log_frame=[&]() {
            fprintf(log,"startup_call_trap_va=0x%08X\nstartup_arg0_va=0x%08X\n",trap,arg);
            fprintf(log,"startup_call_frame=%s\nstartup_seh_registration=%s\n",frame_ok ? "valid":"invalid",seh_ok ? "passed":"failed");
            fprintf(log,"startup_seh_record_va=0x%08X\nstartup_seh_previous=0x%08X\nstartup_seh_handler=0x%08X\n",seh,prev,handler);
            fprintf(log,"startup_seh_chain_depth=%u\nstartup_exception_dispatch=not_implemented\n",seh_depth);
            const char* registers[]={"eax","ecx","edx","ebx","esp","ebp","esi","edi"};
            for(int r=d2rt::R_EAX;r<=d2rt::R_EDI;++r)
                fprintf(log,"startup_%s=0x%08X\n",registers[r],c.reg(r));
        };
        if(first)log_frame();
        if (!original_call_valid || !frame_ok || !seh_ok) {
            if(!first)log_frame();
            service_failed = true;
            fprintf(log, "startup_service_error=entry_frame_or_seh_mismatch\n");
            fprintf(log, "startup_stop_import=%s\n", tag.c_str());
            return false;
        }
        if(imp.dll=="USER32.dll" && imp.name=="MessageBoxA") {
            uint32_t args[4]{};
            if(stack_range(esp,20) && c.read(esp+4,args,sizeof(args))) {
                fprintf(log,"startup_messagebox=observed_only window:0x%08X text:0x%08X caption:0x%08X type:0x%08X\n",args[0],args[1],args[2],args[3]);
                for(unsigned i=1;i<=2;++i) {
                    const auto snapshot=guest_string_probe::observe(args[i],
                        [&](uint32_t address,uint8_t& b){return c.read(address,&b,1);});
                    fprintf(log,"startup_messagebox_%s=status:%s bytes:%u encoding:raw_CP932 hex:",
                        i==1?"text":"caption",guest_string_probe::name(snapshot.status),unsigned(snapshot.length));
                    for(size_t j=0;j<snapshot.length;++j)fprintf(log,"%02X",unsigned(snapshot.bytes[j]));
                    fputc('\n',log);
                }
            }else fprintf(log,"startup_messagebox=observation_rejected_invalid_stack\n");
            // No native dialog, button result or guest continuation is supplied.
        }
        // The original game entry calls 0x4239F0; its timer import returns to
        // 0x423A23. Validate both saved return addresses before recording entry.
        if (imp.dll == "WINMM.dll" && imp.name == "timeBeginPeriod" && ret == 0x00423A23) {
            const uint32_t timer_frame = c.reg(d2rt::R_EBP);
            uint32_t game_frame = 0, timer_caller = 0, game_caller = 0;
            const bool chain = stack_range(timer_frame, 8) &&
                c.read(timer_frame, &game_frame, 4) &&
                c.read(timer_frame + 4, &timer_caller, 4) &&
                game_frame > timer_frame && stack_range(game_frame, 8) &&
                c.read(game_frame + 4, &game_caller, 4) &&
                timer_caller == 0x00602A7C && game_caller == 0x006424B0;
            fprintf(log, "startup_game_entry_call_chain=%s\n", chain ? "verified" : "unverified");
            fprintf(log, "startup_game_timer_caller_va=0x%08X\n", timer_caller);
            fprintf(log, "startup_game_crt_caller_va=0x%08X\n", game_caller);
            if (chain) game_entry_chain_verified = true;
        }
        const StartupServiceResult service = imp.dll=="d3d8.dll" && imp.name=="Direct3DCreate8" ? d3d8.create():
            imp.dll=="DINPUT8.dll" && imp.name=="DirectInput8Create" ? input.create():
            imp.dll=="ole32.dll" && imp.name=="CoCreateInstance" ? sound.create(services.com_ready(wx86_cur_tib())):services.call(imp);
        if(service==StartupServiceResult::Deferred) {
            fprintf(log,"startup_scheduler_yield=dispatch_window_creation\n");
            return false;
        }
        if(service==StartupServiceResult::Unsupported && imp.dll=="USER32.dll" && imp.name=="CreateWindowExA") {
            uint32_t args[12]{};
            if(stack_range(esp,52) && c.read(esp+4,args,sizeof(args))) {
                fprintf(log,"startup_window_requested_width=%u\nstartup_window_requested_height=%u\n",args[6],args[7]);
                fprintf(log,"startup_window_requested_style=0x%08X\nstartup_window_requested_exstyle=0x%08X\n",args[3],args[0]);
                fprintf(log,"startup_window_creation_boundary=synchronous_guest_messages_and_renderer_required\n");
            }
        }
        if (service == StartupServiceResult::Unsupported && imp.dll == "KERNEL32.dll" &&
            imp.name == "CreateThread" && ret == 0x00423A58) worker_create_frame = esp;
        if(service==StartupServiceResult::Unsupported && imp.dll=="KERNEL32.dll" &&
            imp.name=="CreateThread" && ret==0x0040721F)audio_create_frame=esp;
        if (service == StartupServiceResult::Serviced) {
            if(imp.dll=="KERNEL32.dll" && imp.name=="SetThreadPriority" && ret==0x00423A6C &&
               c.reg(d2rt::R_EAX)==1 && worker.blocked && worker.priority>0) {
                priority_dispatch_requested=true;
                fprintf(log,"startup_scheduler_yield=dispatch_priority_worker\n");return false;
            }
            flush_boundary.continuing=true;return true;
        }
        if (service == StartupServiceResult::ContractFailure) {
            if(!first)log_frame();
            service_failed = true;
            fprintf(log, "startup_stop_import=%s\n", tag.c_str());
            return false;
        }
        // Preserve the unsupported API's complete call frame and registers.
        if(!first)log_frame();
        fprintf(log, "startup_stop_import=%s\n", tag.c_str());
        fprintf(log, "startup_stop_iat_va=0x%08X\n", imp.iat_va);
        fprintf(log, "startup_stop_trap_va=0x%08X\n", trap);
        fprintf(log, "startup_stop_api_executed=no\n");
        if (services.heap_ready()) {
            expected_boundary = true;
            fprintf(log, "startup_heap_boundary=passed\n");
        } else if (imp.dll == "KERNEL32.dll" && imp.name == "HeapCreate") {
            uint32_t initial = 0, maximum = 0;
            const bool heap_frame = stack_range(esp, 16) &&
                c.read(esp + 8, &initial, 4) && c.read(esp + 12, &maximum, 4);
            fprintf(log, "startup_heap_flags=0x%08X\n", arg);
            fprintf(log, "startup_heap_initial_bytes=%u\n", initial);
            fprintf(log, "startup_heap_maximum_bytes=%u\n", maximum);
            const bool globals = services.version_globals_match();
            expected_boundary = imp.iat_va == 0x00657160 && ret == 0x0064974C &&
                heap_frame && arg == 0 && initial == 0x1000 && maximum == 0 &&
                services.version_calls() == 1 && services.module_calls() == 1 && globals;
            fprintf(log, "startup_heap_call_frame=%s\n", expected_boundary ? "passed" : "failed");
        }
        return false;
    };
    cpu.set_trap(kTrap, kTrapEnd, startup_trap);
    for (int r = d2rt::R_EAX; r <= d2rt::R_EDI; ++r) cpu.set_reg(r, 0);
    const uint32_t initial_esp = kStackEnd - 0x1000 - 4;
    if (!cpu.write(initial_esp, &kSentinel, 4)) {
        fprintf(log, "startup_result=stack_frame_failed\n"); return false;
    }
    cpu.set_reg(d2rt::R_ESP, initial_esp);
    cpu.set_reg(d2rt::R_EFLAGS, 0x202);
    cpu.set_reg(d2rt::R_EIP, kEntry);
    cpu.take_limit_hit();
    cpu.set_run_limit(kRunBudget);
    fprintf(log, "startup_run_budget=%llu\n", (unsigned long long)kRunBudget);
    fprintf(log, "startup_budget_unit=approx_instructions_via_8_per_block_entry\n");
    fprintf(log, "startup_budget_block_entries=%llu\n", (unsigned long long)(kRunBudget / 8));
    RunnerPlacement placement{log};
    if (!placement.pin()) {
        fprintf(log, "startup_result=runner_affinity_failed\n"); return false;
    }
    StartupWatchdog watchdog(cpu.ip_ptr());
    if (!watchdog.start(log)) {
        fprintf(log, "startup_result=watchdog_unavailable\n"); return false;
    }
    fprintf(log, "startup_entry_va=0x%08X\n", kEntry);
    fprintf(log, "startup_initial_esp=0x%08X\n", initial_esp);
    fprintf(log, "game_entrypoint=attempted\n");
    fprintf(log, "game_code_executed=unconfirmed\n");
    fflush(log);
    const uint64_t start = sceKernelGetProcessTimeWide();
    const char* fault = nullptr;
    bool stopped = false, limit = false;
    unsigned cosine_slices = 0, resume_slices = 0, import_resume_slices = 0;
    unsigned frame_wait_resumes=0;
    fprintf(log,"startup_slice_cap=32 frame_wait_extra:4 maximum:64\nstartup_time_cap_us=%llu\n",(unsigned long long)startup_limits::time_cap_us);
    fprintf(log,"startup_frame_scope=present:%u waits:%u\n",startup_limits::present_frames,startup_limits::frame_wait_resumes);
    auto run_main = [&](uint32_t entry, uint64_t budget) {
        cpu.take_limit_hit(); cpu.set_run_limit(budget);
        stopped = cpu.run(entry, &fault);
        limit = cpu.take_limit_hit();
        fflush(log);boundaries_since_flush=0;
        uint32_t previous = 0;
        bool have_previous = false;
        unsigned unchanged = 0, repeated_state = 0;
        uint32_t previous_state = 0;
        bool have_state = false;
        while (stopped && limit && !service_failed) {
            uint32_t index = 0;
            const bool cosine = cosine_progress(cpu, index);
            const uint64_t elapsed = sceKernelGetProcessTimeWide() - start;
            const uint32_t ip = cpu.reg(d2rt::R_EIP), esp = cpu.reg(d2rt::R_ESP);
            uint8_t stack[128] = {};
            const bool code = original_code_address(exe, nt, image, ip);
            const bool known_trap = startup_known_resume_trap(ip,kTrap,image.imports().size(),
                StartupServices::critical_init_trap,StartupServices::processor_feature_trap,
                sound.owns(ip) || input.owns(ip) || d3d8.owns_trap(ip));
            const bool valid_stack = stack_range(esp,sizeof(stack)) && cpu.read(esp,stack,sizeof(stack));
            uint32_t pending_return=0;
            if(valid_stack)std::memcpy(&pending_return,stack,4);
            const bool valid_return = known_trap && valid_stack && cpu.mapped(pending_return) &&
                original_code_address(exe,nt,image,pending_return);
            if ((!code && !valid_return) || !cpu.mapped(ip) ||
                wx86_cur_tib() != kDiagnosticTeb || !valid_stack) {
                fprintf(log,"startup_resume_stop=invalid_code_stack_or_tib eip:0x%08X code:%s known_trap:%s return:0x%08X valid_return:%s stack:%s tib:0x%08X\n",
                    ip,code?"yes":"no",known_trap?"yes":"no",pending_return,
                    valid_return?"yes":"no",valid_stack?"yes":"no",wx86_cur_tib());break;
            }
            if(known_trap){
                fprintf(log,"startup_pending_trap=0x%08X return:0x%08X dispatch:normal_callback\n",ip,pending_return);
                if(ip>=kTrap && (ip-kTrap)/16<image.imports().size()) {
                    const auto& pending=image.imports()[(ip-kTrap)/16];
                    fprintf(log,"startup_pending_import=%s!%s\n",pending.dll.c_str(),pending.name.c_str());
                }
            }
            uint32_t hash = 2166136261u;
            auto hash_word = [&](uint32_t word) { hash = (hash ^ word) * 16777619u; };
            const char* registers[] = {"eax","ecx","edx","ebx","esp","ebp","esi","edi","eip","eflags"};
            for (int r = d2rt::R_EAX; r <= d2rt::R_EFLAGS; ++r) {
                const uint32_t value = cpu.reg(r);
                hash_word(value);
                fprintf(log, "startup_slice_%s=0x%08X\n", registers[r], value);
            }
            for (uint8_t byte : stack) hash = (hash ^ byte) * 16777619u;
            hash_word(main_import_calls); hash_word(services.heap_alloc_calls());
            repeated_state = have_state && hash == previous_state ? repeated_state + 1 : 0;
            previous_state = hash; have_state = true;
            uint32_t stack_top = 0; std::memcpy(&stack_top, stack, 4);
            fprintf(log, "startup_slice_progress=state:0x%08X imports:%u allocations:%u stack_top:0x%08X elapsed_us:%llu\n",
                hash, main_import_calls, services.heap_alloc_calls(), stack_top, (unsigned long long)elapsed);
            if (cosine) {
                uint32_t first = 0, last = 0;
                const bool samples = index > 0 && cpu.read(0x006884C0, &first, 4) &&
                    cpu.read(0x006884C0 + (index - 1) * 4, &last, 4);
                fprintf(log, "startup_cosine_progress=%u/3600 eip=0x%08X elapsed_us=%llu\n",
                    index, ip, (unsigned long long)elapsed);
                if (samples) fprintf(log, "startup_cosine_samples=first:0x%08X last_completed:0x%08X\n", first, last);
                if (have_previous && index < previous) {
                    fprintf(log, "startup_resume_stop=cosine_index_regressed\n"); break;
                }
                unchanged = have_previous && index == previous ? unchanged + 1 : 0;
                previous = index; have_previous = true;
            } else {
                have_previous = false; unchanged = 0;
            }
            const unsigned slice_cap=startup_wait::slice_cap(frame_wait_resumes);
            if (unchanged >= 2 || repeated_state >= 3 || resume_slices >= slice_cap || elapsed >= startup_limits::time_cap_us) {
                fprintf(log, "startup_resume_stop=%s\n", unchanged >= 2 ? "cosine_no_index_progress" :
                    repeated_state >= 3 ? "repeated_sampled_state" : resume_slices >= slice_cap ? "slice_cap" : "time_cap"); break;
            }
            ++resume_slices;
            if (cosine) ++cosine_slices;
            if (known_trap) ++import_resume_slices;
            fprintf(log, "startup_resume_slice=%u scope=%s budget=%llu\n",
                resume_slices, cosine ? "original_cosine" : known_trap ? "owned_import_trap" : "original_executable_section", (unsigned long long)kRunBudget);
            fflush(log);
            // run() recharges its block budget on the same emulator. All
            // registers, stack, flags and x87 state remain owned by it.
            cpu.set_run_limit(kRunBudget);
            stopped = cpu.run(cpu.reg(d2rt::R_EIP), &fault);
            limit = cpu.take_limit_hit();
            fflush(log);boundaries_since_flush=0;
        }
    };
    run_main(kEntry, kRunBudget);
    bool worker_ok = true;
    bool thread_created = false;
    if (worker_create_frame && stopped && !limit && !service_failed) {
        const uint32_t create_frame = worker_create_frame;
        d2rt::X86Context saved_main{}, restored_main{};
        cpu.save_context(saved_main);
        saved_main.fs_base=wx86_cur_tib();
        worker_ok = run_worker_probe(cpu, image, create_frame, services, worker, log);
        cpu.save_context(restored_main);
        restored_main.fs_base=wx86_cur_tib();
        const bool main_restored = startup_wait::same_context(saved_main,restored_main) &&
            wx86_cur_tib() == kDiagnosticTeb;
        fprintf(log, "startup_main_context_readback=%s\n", main_restored ? "passed" : "failed");
        worker_ok = worker_ok && main_restored;
        cpu.set_trap(kTrap, kTrapEnd, startup_trap);
        if (worker_ok) {
            uint32_t ret = 0, output = 0, copy = 0;
            worker_ok = cpu.reg(d2rt::R_ESP) == create_frame &&
                cpu.read(create_frame,&ret,4) && ret == 0x00423A58 &&
                cpu.read(create_frame+24,&output,4) && output == 0x0068BE3C &&
                cpu.write(output,&worker.id,4) && cpu.read(output,&copy,4) && copy == worker.id;
            if (worker_ok) {
                cpu.trap_epilogue(worker.handle,28,ret);
                worker_ok = cpu.reg(d2rt::R_EIP) == ret && cpu.reg(d2rt::R_ESP) == create_frame+28 &&
                    cpu.reg(d2rt::R_EAX) == worker.handle;
            }
            if (worker_ok) {
                thread_created = true;
                services.attach_worker(&worker);
                fprintf(log, "startup_serviced_import=KERNEL32.dll!CreateThread\n");
                fprintf(log, "startup_thread_handle=0x%08X\n", worker.handle);
                fprintf(log, "startup_thread_id=%u\n", worker.id);
                fprintf(log, "startup_thread_id_readback=passed\n");
                fprintf(log, "startup_thread_stack_bytes=28\n");
                fprintf(log, "startup_thread_handoff=worker_blocked_main_resumed\n");
                worker_create_frame = 0;
                expected_boundary = false;
                run_main(ret, 4096);
            }
        }
    }
    if(priority_dispatch_requested && stopped && !limit && worker_ok) {
        worker_ok=wake_worker_slice(cpu,image,services,worker,log);
        cpu.set_trap(kTrap,kTrapEnd,startup_trap);
        if(worker_ok && worker.unsupported_boundary) expected_boundary=true;
        else if(worker_ok) {
            priority_dispatch_requested=false;
            expected_boundary=false;
            run_main(cpu.reg(d2rt::R_EIP), 4096);
        }
    }
    for(unsigned window_dispatch=0;window_dispatch<4 && services.window_pending() && stopped && !limit && !service_failed && worker_ok;++window_dispatch) {
        const bool window_ok=services.finish_window_creation();
        cpu.set_trap(kTrap,kTrapEnd,startup_trap);
        if(!window_ok)service_failed=true;
        else {
            expected_boundary=false;
            run_main(cpu.reg(d2rt::R_EIP), kRunBudget);
        }
    }
    bool audio_created=false;
    if(audio_create_frame && stopped && !limit && !service_failed && worker_ok) {
        d2rt::X86Context saved{},restored{};cpu.save_context(saved);
        saved.fs_base=wx86_cur_tib();
        fprintf(log,"startup_audio_worker=initial_handoff entry:0x00407EF0\n");
        worker_ok=run_worker_probe(cpu,image,audio_create_frame,services,audio_worker,log);
        cpu.save_context(restored);
        restored.fs_base=wx86_cur_tib();
        worker_ok=worker_ok && startup_wait::same_context(saved,restored) && wx86_cur_tib()==kDiagnosticTeb;
        cpu.set_trap(kTrap,kTrapEnd,startup_trap);
        uint32_t ret=0,out=0,copy=0;
        if(worker_ok)worker_ok=cpu.read(audio_create_frame,&ret,4) && ret==0x0040721F &&
            cpu.read(audio_create_frame+24,&out,4) && out==0x0067139C &&
            cpu.write(out,&audio_worker.id,4) && cpu.read(out,&copy,4) && copy==audio_worker.id;
        if(worker_ok) {
            services.attach_audio_worker(&audio_worker);audio_created=true;
            cpu.trap_epilogue(audio_worker.handle,28,ret);
            fprintf(log,"startup_audio_worker=blocked_main_resumed handle:0x%08X id:%u\n",audio_worker.handle,audio_worker.id);
            expected_boundary=false;run_main(ret,kRunBudget);
            // The new thread runs at the same priority as the timer. Dispatch
            // its first elapsed wait before reporting the main boundary.
            if(stopped && !limit && !service_failed && audio_worker.priority>0) {
                worker_ok=wake_worker_slice(cpu,image,services,audio_worker,log);
                cpu.set_trap(kTrap,kTrapEnd,startup_trap);
            }
        } else service_failed=true;
    }
    // Hardware 74 reached this infinite frame-event wait after a genuine
    // Present. Pause main with its unexecuted call intact, run the original
    // timer worker after its real timeout, then retry the same event contract.
    for(unsigned tick=0;tick<startup_limits::frame_wait_resumes && stopped && !limit && !service_failed && worker_ok;++tick) {
        const uint32_t trap=cpu.reg(d2rt::R_EIP),esp=cpu.reg(d2rt::R_ESP);
        uint32_t wait[3]{};
        if(trap<kTrap || (trap-kTrap)%16 || (trap-kTrap)/16>=image.imports().size() ||
           !stack_range(esp,12) || !cpu.read(esp,wait,12) ||
           !startup_wait::observed_frame_wait(wx86_cur_tib(),esp,wait))break;
        const auto& imp=image.imports()[(trap-kTrap)/16];
        if(imp.dll!="KERNEL32.dll" || imp.name!="WaitForSingleObject" || imp.iat_va!=0x00657120 ||
           !services.event_auto_reset(wait[1]) || !services.event_unsignaled(wait[1]))break;
        uint32_t timer_event=0,period=0;
        if(!thread_created || !worker.blocked || worker.unsupported_boundary || worker.id!=12 ||
           worker.event!=0x00AB2000 || worker.timeout_ms!=16 ||
           !cpu.read(0x0068BE34,&timer_event,4) || timer_event!=worker.event ||
           !cpu.read(0x0066C23C,&period,4) || period!=worker.timeout_ms)break;
        if(sceKernelGetProcessTimeWide()-start>=startup_limits::time_cap_us) {
            fprintf(log,"startup_frame_wait_stop=time_cap\n");break;
        }
        d2rt::X86Context saved{},restored{};cpu.save_context(saved);
        saved.fs_base=wx86_cur_tib();
        const uint32_t error=wx86_get_lasterr(cpu),generation=services.event_generation(wait[1]);
        const uint64_t frame_started=sceKernelGetProcessTimeWide();
        uint32_t counter=0;uint16_t state=0;
        const uint32_t ebp=cpu.reg(d2rt::R_EBP);
        if(cpu.read(0x0066C240,&counter,4) && ebp>=kStack+0x3C && stack_range(ebp-0x3C,2) && cpu.read(ebp-0x3C,&state,2))
            fprintf(log,"startup_frame_phase=state_word:0x%04X transition_counter:0x%08X scope:observed_guest_globals\n",unsigned(state),counter);
        // Read the original main scene only at the exact observed wait. The
        // logo age is incremented by guest code after Present; never write it.
        uint32_t scene=0,vtable=0,methods[3]{};
        if(ebp>=kStack+0x184 && stack_range(ebp-0x184,4) &&
            cpu.read(ebp-0x184,&scene,4) && scene>=0x10000 &&
            uint64_t(scene)+16<=0x02000000 && cpu.read(scene,&vtable,4) &&
            vtable>=image.load_base() && uint64_t(vtable)+sizeof(methods)<=uint64_t(image.load_base())+image.image_size() &&
            cpu.read(vtable,methods,sizeof(methods))) {
            fprintf(log,"startup_frame_scene=object:0x%08X vtable:0x%08X update:0x%08X draw:0x%08X scope:observed_guest_object\n",scene,vtable,methods[1],methods[2]);
            uint16_t age=0;
            if(vtable==0x00657BE0 && cpu.read(scene+0xC,&age,2))
                fprintf(log,"startup_frame_logo_age=%u transition_after:180 scope:original_guest_counter\n",unsigned(age));
        }
        fprintf(log,"startup_frame_wait=blocked frame:%u handle:0x%08X timeout:infinite producer:timer_worker\n",frame_wait_resumes+1,wait[1]);
        worker_ok=wake_worker_slice(cpu,image,services,worker,log);
        cpu.set_trap(kTrap,kTrapEnd,startup_trap);
        cpu.save_context(restored);
        uint32_t after[3]{};
        restored.fs_base=wx86_cur_tib();
        const bool preserved=startup_wait::same_context(saved,restored) &&
            wx86_cur_tib()==kDiagnosticTeb && wx86_get_lasterr(cpu)==error &&
            cpu.read(esp,after,12) && !std::memcmp(wait,after,12);
        fprintf(log,"startup_frame_wait_context=%s scope:gpr_flags_fs_fpu_callframe_lasterror\n",preserved ? "preserved":"failed");
        if(!worker_ok || !preserved){worker_ok=false;service_failed=true;break;}
        if(worker.unsupported_boundary){expected_boundary=true;break;}
        if(!services.event_signaled(wait[1]) || services.event_generation(wait[1])==generation) {
            fprintf(log,"startup_frame_wait_stop=producer_did_not_signal\n");break;
        }
        fprintf(log,"startup_frame_wait_signal=observed generation:%u\n",services.event_generation(wait[1]));
        const auto result=services.call(imp);
        const bool resumed=result==StartupServiceResult::Serviced && cpu.reg(d2rt::R_EAX)==0 &&
            cpu.reg(d2rt::R_ESP)==esp+12 && cpu.reg(d2rt::R_EIP)==wait[0] &&
            wx86_get_lasterr(cpu)==error && !services.event_signaled(wait[1]);
        fprintf(log,"startup_frame_wait_resume=%s result:WAIT_OBJECT_0 auto_reset:consumed cleanup:12\n",resumed ? "passed":"failed");
        if(!resumed){service_failed=true;break;}
        ++frame_wait_resumes;expected_boundary=false;
        run_main(wait[0],kRunBudget);
        fprintf(log,"startup_frame_cycle_elapsed_us=%llu cycle:%u scope:timer_and_main\n",(unsigned long long)(sceKernelGetProcessTimeWide()-frame_started),frame_wait_resumes);
    }
    fprintf(log,"startup_frame_wait_resumes=%u\n",frame_wait_resumes);
    fprintf(log, "startup_thread_create_calls=%u\n", (thread_created?1u:0u)+(audio_created?1u:0u));
    fprintf(log, "startup_d3d8_serviced_calls=%u\n",d3d8.serviced_calls());
    d3d8.report_usage();
    fprintf(log, "startup_dinput_serviced_calls=%u\n",input.serviced_calls());
    fprintf(log, "startup_dsound_serviced_calls=%u\n",sound.serviced_calls());
    fprintf(log, "startup_cosine_resume_slices=%u\n", cosine_slices);
    fprintf(log,"startup_import_resume_slices=%u\n",import_resume_slices);
    fprintf(log, "startup_resume_slices=%u\nstartup_main_import_calls=%u\n", resume_slices, main_import_calls);
    if (limit || !stopped) fprintf(log, "startup_stop_import=%s EIP 0x%08X\n",
        limit ? "CPU BUDGET" : "CPU FAULT", cpu.reg(d2rt::R_EIP));
    watchdog.finish();
    fprintf(log, "startup_elapsed_us=%llu\n",
        (unsigned long long)(sceKernelGetProcessTimeWide() - start));
    fprintf(log, "startup_final_eip=0x%08X\n", cpu.reg(d2rt::R_EIP));
    fprintf(log, "startup_final_esp=0x%08X\n", cpu.reg(d2rt::R_ESP));
    fprintf(log, "startup_final_ebp=0x%08X\n", cpu.reg(d2rt::R_EBP));
    if (limit && cpu.reg(d2rt::R_EIP) < 0x00400000u) {
        fprintf(log, "startup_control_flow_result=low_guest_eip\n");
    } else if (limit) {
        fprintf(log,"startup_control_flow_result=%s\n", original_code_address(exe,nt,image,cpu.reg(d2rt::R_EIP)) ? "budget_in_image" : "budget_outside_image");
    }
    fprintf(log, "startup_limit_hit=%s\n", limit ? "yes" : "no");
    fprintf(log, "startup_version_calls=%u\n", services.version_calls());
    fprintf(log, "startup_module_calls=%u\n", services.module_calls());
    fprintf(log, "startup_serviced_imports=%u\n", services.version_calls() + services.module_calls() + services.heap_create_calls() + services.heap_alloc_calls() + services.proc_address_calls() + services.critical_init_calls() + services.tls_calls() + services.process_calls() + services.environment_calls() + services.conversion_calls() + services.sync_calls() + services.code_page_calls() + services.string_type_calls() + services.case_map_calls() + services.heap_other_calls() + services.clock_calls() + services.multimedia_calls() + services.event_calls() + services.priority_calls() + (thread_created ? 1u : 0u));
    fprintf(log, "startup_heap_create_calls=%u\n", services.heap_create_calls());
    fprintf(log, "startup_heap_alloc_calls=%u\n", services.heap_alloc_calls());
    fprintf(log, "startup_heap_other_calls=%u\n", services.heap_other_calls());
    fprintf(log, "startup_proc_address_calls=%u\n", services.proc_address_calls());
    fprintf(log, "startup_critical_init_calls=%u\n", services.critical_init_calls());
    fprintf(log, "startup_tls_calls=%u\n", services.tls_calls());
    fprintf(log, "startup_unavailable_export_calls=%u\n", services.unavailable_export_calls());
    fprintf(log, "startup_process_calls=%u\n", services.process_calls());
    fprintf(log, "startup_clock_calls=%u\n", services.clock_calls());
    fprintf(log, "startup_multimedia_calls=%u\n", services.multimedia_calls());
    fprintf(log, "startup_event_calls=%u\n", services.event_calls());
    fprintf(log, "startup_priority_calls=%u\n", services.priority_calls());
    fprintf(log, "startup_game_entry_verified=%s\n", game_entry_chain_verified ? "yes" : "no");
    fprintf(log, "startup_environment_calls=%u\n", services.environment_calls());
    fprintf(log, "startup_conversion_calls=%u\n", services.conversion_calls());
    fprintf(log, "startup_code_page_calls=%u\n", services.code_page_calls());
    fprintf(log, "startup_string_type_calls=%u\n", services.string_type_calls());
    fprintf(log, "startup_case_map_calls=%u\n", services.case_map_calls());
    fprintf(log, "startup_sync_calls=%u\n", services.sync_calls());
    fprintf(log, "startup_message_calls=%u\n", services.message_calls());
    if (!stopped) {
        fprintf(log, "startup_fault=%s\n", fault ? fault : "unknown");
        fprintf(log, "startup_fault_va=0x%08X\n", cpu.fault_addr());
        fprintf(log, "startup_fault_code=0x%08X\n", cpu.fault_code());
    }
    const bool passed = stopped && !limit && !service_failed && expected_boundary && worker_ok;
    fprintf(log, "game_code_executed=%s\n", original_call_valid ? "yes" : "unconfirmed");
    fprintf(log, "startup_result=%s\n", passed ? "reached_next_import_after_heap" :
        (!worker_ok ? "worker_probe_failed" : !stopped ? "cpu_fault" : (limit ? "budget_exhausted" :
        (service_failed ? "service_contract_failed" :
        (import_hit ? "unexpected_import_or_frame" : "unexpected_stop")))));
    fprintf(log,"batch_checks=disabled_user_requested_original_boot_only\n");
    return passed;
}
