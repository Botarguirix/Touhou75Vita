#include "startup_probe.h"
#include "startup_services.h"
#include "seh_chain.h"
#include "thread_smoke.h"
#include "runtime/cpu.h"
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
// Allow the original entrypoint to traverse the post-HeapCreate allocator
// setup while retaining the 15-second watchdog as the hard safety bound.
constexpr uint64_t kRunBudget = 65536, kTimeoutUs = 30000000;
// The VitaSDK example and the pinned WinVita native threads use this class.
// 0x10000040 used by r1 was rejected on hardware with ILLEGAL_PRIORITY.
constexpr int kWatchdogPriority = 0x10000100;
const char* const kWatchdogPath = "ux0:data/TH075Vita/iteration30-watchdog.log";

bool stack_range(uint32_t address, uint32_t size) {
    return address >= kStack && uint64_t(address) + size <= kStackEnd;
}
bool file_u32(const std::vector<uint8_t>& file, uint64_t offset, uint32_t& out) {
    if (offset > file.size() || file.size() - offset < 4) return false;
    const auto* p = file.data() + size_t(offset);
    out = uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
          (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
    return true;
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
        fprintf(report_, "watchdog_revision=iteration30-r2\n");
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
    const d2rt::ImportRef dynamic_critical = {
        "KERNEL32.dll", "InitializeCriticalSectionAndSpinCount", 0, 0, 0};
    const d2rt::ImportRef dynamic_processor = {
        "KERNEL32.dll", "IsProcessorFeaturePresent", 0, 0, 0};
    cpu.set_trap(kTrap, kTrapEnd, [&](d2rt::Cpu& c, uint32_t trap) {
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
        fprintf(log, "startup_call_trap_va=0x%08X\n", trap);
        fprintf(log, "startup_return_va=0x%08X\n", ret);
        fprintf(log, "startup_arg0_va=0x%08X\n", arg);
        fprintf(log, "startup_call_frame=%s\n", frame_ok ? "valid" : "invalid");
        fprintf(log, "startup_seh_record_va=0x%08X\n", seh);
        fprintf(log, "startup_seh_previous=0x%08X\n", prev);
        fprintf(log, "startup_seh_handler=0x%08X\n", handler);
        fprintf(log, "startup_seh_registration=%s\n", seh_ok ? "passed" : "failed");
        fprintf(log, "startup_seh_chain_depth=%u\n", seh_depth);
        fprintf(log, "startup_exception_dispatch=not_implemented\n");
        const char* registers[] = {"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"};
        for (int r = d2rt::R_EAX; r <= d2rt::R_EDI; ++r)
            fprintf(log, "startup_%s=0x%08X\n", registers[r], c.reg(r));
        if (!original_call_valid || !frame_ok || !seh_ok) {
            service_failed = true;
            fprintf(log, "startup_service_error=entry_frame_or_seh_mismatch\n");
            fprintf(log, "startup_stop_import=%s\n", tag.c_str());
            return false;
        }
        const StartupServiceResult service = services.call(imp);
        if (service == StartupServiceResult::Serviced) return true;
        if (service == StartupServiceResult::ContractFailure) {
            service_failed = true;
            fprintf(log, "startup_stop_import=%s\n", tag.c_str());
            return false;
        }
        // Preserve the unsupported API's complete call frame and registers.
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
    });
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
    fprintf(log, "startup_budget_unit=approx_instructions_via_512_block_entries\n");
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
    const bool stopped = cpu.run(kEntry, &fault);
    watchdog.finish();
    const bool limit = cpu.take_limit_hit();
    fprintf(log, "startup_elapsed_us=%llu\n",
        (unsigned long long)(sceKernelGetProcessTimeWide() - start));
    fprintf(log, "startup_final_eip=0x%08X\n", cpu.reg(d2rt::R_EIP));
    fprintf(log, "startup_final_esp=0x%08X\n", cpu.reg(d2rt::R_ESP));
    fprintf(log, "startup_final_ebp=0x%08X\n", cpu.reg(d2rt::R_EBP));
    if (limit && cpu.reg(d2rt::R_EIP) < 0x00400000u) {
        fprintf(log, "startup_control_flow_result=low_guest_eip\n");
    } else if (limit) {
        fprintf(log, "startup_control_flow_result=budget_in_image\n");
    }
    fprintf(log, "startup_limit_hit=%s\n", limit ? "yes" : "no");
    fprintf(log, "startup_version_calls=%u\n", services.version_calls());
    fprintf(log, "startup_module_calls=%u\n", services.module_calls());
    fprintf(log, "startup_serviced_imports=%u\n", services.version_calls() + services.module_calls() + services.heap_create_calls() + services.heap_alloc_calls() + services.proc_address_calls() + services.critical_init_calls() + services.tls_calls() + services.process_calls() + services.environment_calls() + services.conversion_calls() + services.sync_calls() + services.code_page_calls() + services.string_type_calls() + services.case_map_calls());
    fprintf(log, "startup_heap_create_calls=%u\n", services.heap_create_calls());
    fprintf(log, "startup_heap_alloc_calls=%u\n", services.heap_alloc_calls());
    fprintf(log, "startup_proc_address_calls=%u\n", services.proc_address_calls());
    fprintf(log, "startup_critical_init_calls=%u\n", services.critical_init_calls());
    fprintf(log, "startup_tls_calls=%u\n", services.tls_calls());
    fprintf(log, "startup_unavailable_export_calls=%u\n", services.unavailable_export_calls());
    fprintf(log, "startup_process_calls=%u\n", services.process_calls());
    fprintf(log, "startup_environment_calls=%u\n", services.environment_calls());
    fprintf(log, "startup_conversion_calls=%u\n", services.conversion_calls());
    fprintf(log, "startup_code_page_calls=%u\n", services.code_page_calls());
    fprintf(log, "startup_string_type_calls=%u\n", services.string_type_calls());
    fprintf(log, "startup_case_map_calls=%u\n", services.case_map_calls());
    fprintf(log, "startup_sync_calls=%u\n", services.sync_calls());
    if (!stopped) {
        fprintf(log, "startup_fault=%s\n", fault ? fault : "unknown");
        fprintf(log, "startup_fault_va=0x%08X\n", cpu.fault_addr());
        fprintf(log, "startup_fault_code=0x%08X\n", cpu.fault_code());
    }
    const bool passed = stopped && !limit && !service_failed && expected_boundary;
    fprintf(log, "game_code_executed=%s\n", original_call_valid ? "yes" : "unconfirmed");
    fprintf(log, "startup_result=%s\n", passed ? "reached_next_import_after_heap" :
        (!stopped ? "cpu_fault" : (limit ? "budget_exhausted" :
        (service_failed ? "service_contract_failed" :
        (import_hit ? "unexpected_import_or_frame" : "unexpected_stop")))));
    return passed;
}
