#include "import_smoke.h"
#include "runtime/bridge.h"
#include "service_smoke.h"
#include <psp2/kernel/processmgr.h>
#include <string>
#include <cstdlib>

namespace {
// Keep every routine on its own page, distinct from the basic smoke at
// 0x00700000. Remapping a page clears protection bookkeeping and can make
// invalidate_code() miss a previously translated block at that address.
constexpr uint32_t kLastErrorCode = 0x00710000;
constexpr uint32_t kTickCode = 0x00711000;
constexpr uint32_t kStack = 0x00800000;
constexpr uint32_t kTrap = 0x00B00000;
constexpr uint32_t kError = 0x00000775;

void emit32(std::vector<uint8_t>& code, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) code.push_back((uint8_t)(value >> (8 * i)));
}
void emit_call(std::vector<uint8_t>& code, uint32_t iat) {
    code.push_back(0xFF); code.push_back(0x15); // call dword ptr [iat]
    emit32(code, iat);
}
bool probe(d2rt::Bridge& bridge, uint32_t code_va, const std::vector<uint8_t>& code,
           const char* name, bool check_value, uint32_t expected,
           bool& unsupported, FILE* log) {
    auto& cpu = *bridge.cpu();
    cpu.discard_code(code_va, 0x1000);
    if (!cpu.write(code_va, code.data(), (uint32_t)code.size())) {
        fprintf(log, "import_smoke_error=code_write_failed\n");
        return false;
    }
    std::vector<uint8_t> readback(code.size());
    if (!cpu.read(code_va, readback.data(), (uint32_t)readback.size()) || readback != code) {
        fprintf(log, "import_smoke_error=code_readback_mismatch\n");
        return false;
    }
    cpu.set_reg(d2rt::R_EFLAGS, 0x202);
    fprintf(log, "import_smoke_begin=%s\n", name);
    fprintf(log, "import_smoke_%s_code_va=0x%08X\n", name, code_va);
    uint32_t result = 0;
    const char* fault = nullptr;
    const bool stopped = bridge.call_va(code_va, {}, result, &fault);
    const bool passed = stopped && !unsupported &&
        cpu.reg(d2rt::R_EIP) == bridge.sentinel() &&
        cpu.reg(d2rt::R_ESP) == kStack + 0x00200000 - 0x1000 &&
        (!check_value || result == expected);
    fprintf(log, "import_smoke_%s_eax=0x%08X\n", name, result);
    fprintf(log, "import_smoke_%s_esp=0x%08X\n", name, cpu.reg(d2rt::R_ESP));
    fprintf(log, "import_smoke_%s_eip=0x%08X\n", name, cpu.reg(d2rt::R_EIP));
    fprintf(log, "import_smoke_%s_result=%s\n", name, passed ? "passed" : "failed");
    if (!stopped) fprintf(log, "import_smoke_fault=%s\n", fault ? fault : "unknown");
    return passed;
}
}

bool run_import_smoke(d2rt::Cpu& cpu, const std::vector<uint8_t>& exe, FILE* log) {
    // All mapped regions fit the 16 MiB guest span used in Iteration 07.
    if (setenv("WX86_MODBASE", "00C00000", 1) ||
        setenv("WX86_STACKBASE", "00800000", 1) ||
        setenv("WX86_TRAPBASE", "00B00000", 1)) return false;
    d2rt::Bridge bridge(&cpu);
    bridge.set_module_limit(0x01000000);
    std::string error;
    auto* image = bridge.add_module("TH075.exe", exe, error);
    if (!image || image->load_base() != 0x00400000 ||
        (uint64_t)image->load_base() + image->image_size() > 0x00700000) {
        fprintf(log, "import_bridge_error=invalid_image_or_layout %s\n", error.c_str());
        return false;
    }

    // These are diagnostic single-thread shims. They are not a process/TEB
    // implementation and must not be used to launch the game's threads.
    uint32_t last_error = 0;
    ServiceSmoke services(cpu, log, last_error);
    services.install(bridge);
    unsigned set_calls = 0, get_calls = 0, tick_calls = 0;
    bool unsupported = false;
    d2rt::Shim set; set.argc = 1;
    set.fn = [&](d2rt::Cpu& c) {
        ++set_calls; last_error = c.arg(0);
        fprintf(log, "import_shim=SetLastError arg0=0x%08X\n", last_error);
        return 0u;
    };
    bridge.register_shim("KERNEL32.dll", "SetLastError", set);
    d2rt::Shim get;
    get.fn = [&](d2rt::Cpu&) {
        ++get_calls;
        fprintf(log, "import_shim=GetLastError return=0x%08X\n", last_error);
        return last_error;
    };
    bridge.register_shim("KERNEL32.dll", "GetLastError", get);
    d2rt::Shim tick;
    tick.fn = [&](d2rt::Cpu&) {
        ++tick_calls;
        fprintf(log, "import_shim=GetTickCount\n");
        return (uint32_t)(sceKernelGetProcessTimeWide() / 1000ull);
    };
    bridge.register_shim("KERNEL32.dll", "GetTickCount", tick);
    bridge.set_default_shim([&](d2rt::Cpu&, const std::string& tag) {
        unsupported = true;
        fprintf(log, "unsupported_import_called=%s\n", tag.c_str());
        bridge.request_yield();
        return 0u;
    });

    uint32_t set_iat = 0, get_iat = 0, tick_iat = 0;
    for (const auto& imp : image->imports()) {
        // Only explicitly installed diagnostic APIs are resolved.
        const bool implemented = !imp.name.empty() && bridge.shim_trap(imp.dll, imp.name);
        fprintf(log, "import_support=%s!%s status=%s\n", imp.dll.c_str(),
                imp.name.c_str(), implemented ? "diagnostic_shim" : "unsupported");
        if (implemented && imp.name == "SetLastError") set_iat = imp.iat_va;
        if (implemented && imp.name == "GetLastError") get_iat = imp.iat_va;
        if (implemented && imp.name == "GetTickCount") tick_iat = imp.iat_va;
    }
    if (!set_iat || !get_iat || !tick_iat) {
        fprintf(log, "import_bridge_error=required_import_missing\n");
        return false;
    }
    if (!bridge.commit(error) || !bridge.link(error) ||
        !cpu.map(kLastErrorCode, 0x2000, nullptr, d2rt::P_RWX)) {
        fprintf(log, "import_bridge_error=%s\n", error.c_str());
        return false;
    }
    fprintf(log, "import_bridge_unresolved=%u\n", bridge.unresolved_count());
    fprintf(log, "game_import_resolution=partial_diagnostic_only\n");
    fprintf(log, "import_bridge_stack_base=0x%08X\n", kStack);
    fprintf(log, "import_bridge_trap_base=0x%08X\n", kTrap);
    fprintf(log, "import_bridge_lasterror_scope=diagnostic_single_thread\n");
    fprintf(log, "import_bridge_tick_origin=vita_process_start\n");
    for (const char* name : {"SetLastError", "GetLastError", "GetTickCount"}) {
        const uint32_t iat = std::string(name) == "SetLastError" ? set_iat :
            (std::string(name) == "GetLastError" ? get_iat : tick_iat);
        const uint32_t expected = bridge.shim_trap_existing("KERNEL32.dll", name);
        uint32_t target = 0;
        if (!cpu.read(iat, &target, sizeof(target)) || !expected || target != expected) {
            fprintf(log, "import_bridge_error=iat_target_mismatch name=%s\n", name);
            cpu.set_trap(0, 0, [](d2rt::Cpu&, uint32_t) { return false; });
            return false;
        }
        fprintf(log, "import_bridge_binding=%s iat=0x%08X target=0x%08X\n", name, iat, target);
    }

    std::vector<uint8_t> code = {0x68}; // push kError
    emit32(code, kError);
    emit_call(code, set_iat);
    emit_call(code, get_iat);
    code.push_back(0xC3); // ret to bridge sentinel
    bool passed = probe(bridge, kLastErrorCode, code, "lasterror", true, kError, unsupported, log);
    if (passed) {
        code.clear(); emit_call(code, tick_iat); code.push_back(0xC3);
        const uint32_t before = (uint32_t)(sceKernelGetProcessTimeWide() / 1000ull);
        passed = probe(bridge, kTickCode, code, "tickcount", false, 0, unsupported, log);
        const uint32_t observed = cpu.reg(d2rt::R_EAX);
        const uint32_t after = (uint32_t)(sceKernelGetProcessTimeWide() / 1000ull);
        const bool in_range = observed - before <= after - before;
        fprintf(log, "import_smoke_tickcount_range=%s\n", in_range ? "passed" : "failed");
        passed = passed && in_range;
    }
    passed = passed && set_calls == 1 && get_calls == 1 && tick_calls == 1;
    fprintf(log, "import_smoke_calls=set:%u get:%u tick:%u\n", set_calls, get_calls, tick_calls);
    fprintf(log, "import_smoke_result=%s\n", passed ? "passed" : "failed");
    const bool services_passed = passed && services.run(bridge, *image, exe, unsupported);
    // bridge owns the installed handler; clear it before bridge leaves scope.
    cpu.set_trap(0, 0, [](d2rt::Cpu&, uint32_t) { return false; });
    return passed && services_passed;
}
