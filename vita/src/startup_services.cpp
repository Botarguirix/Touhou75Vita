#include "startup_services.h"
#include "runtime/cpu.h"
#include "runtime/pe_image.h"
#include <array>
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>

namespace {
constexpr uint32_t kStack = 0x00800000, kStackEnd = 0x00A00000;
constexpr uint32_t kMajor = 5, kMinor = 1, kBuild = 2600, kPlatform = 2;
bool in_stack(uint32_t p, uint32_t size) {
    return p >= kStack && uint64_t(p) + size <= kStackEnd;
}
void put32(uint8_t* out, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) out[i] = uint8_t(value >> (8 * i));
}
}

StartupServiceResult StartupServices::call(const d2rt::ImportRef& import) {
    if (import.dll != "KERNEL32.dll") return StartupServiceResult::Unsupported;
    const bool version = import.name == "GetVersionExA";
    const bool module = import.name == "GetModuleHandleA";
    const bool proc_address = import.name == "GetProcAddress";
    const bool critical_init = import.name == "InitializeCriticalSectionAndSpinCount";
    const bool heap_create = import.name == "HeapCreate";
    const bool heap_alloc = import.name == "HeapAlloc";
    const bool heap_free = import.name == "HeapFree";
    const bool heap_size = import.name == "HeapSize";
    if (!version && !module && !proc_address && !critical_init &&
        !heap_create && !heap_alloc && !heap_free && !heap_size)
        return StartupServiceResult::Unsupported;
    const uint32_t esp = cpu_.reg(d2rt::R_ESP);
    const int preserved[] = {d2rt::R_EBX, d2rt::R_EBP, d2rt::R_ESI, d2rt::R_EDI};
    uint32_t before[4];
    for (unsigned i = 0; i < 4; ++i) before[i] = cpu_.reg(preserved[i]);
    uint32_t ret = 0, arg = 0;
    uint32_t expected_eax = 0;
    if (!in_stack(esp, 8) || !cpu_.read(esp, &ret, 4) || !cpu_.read(esp + 4, &arg, 4)) {
        fprintf(log_, "startup_service_error=invalid_call_frame\n");
        return StartupServiceResult::ContractFailure;
    }
    if (version) {
        // This port advertises an explicit XP 5.1/2600 compatibility profile.
        // It isn't host OS detection. Only the observed 148-byte ANSI layout
        // and the first call site are admitted in this iteration.
        uint32_t size = 0;
        if (version_calls_ || module_calls_ || import.iat_va != 0x00657090 || ret != 0x00642352 ||
            !in_stack(arg, 148) || !cpu_.read(arg, &size, 4) || size != 148) {
            fprintf(log_, "startup_service_error=unsupported_version_call\n");
            return StartupServiceResult::ContractFailure;
        }
        std::array<uint8_t, 148> info{};
        put32(info.data(), 148);
        put32(info.data() + 4, kMajor);
        put32(info.data() + 8, kMinor);
        put32(info.data() + 12, kBuild);
        put32(info.data() + 16, kPlatform);
        // szCSDVersion is a fully initialized empty ANSI string (no SP).
        std::array<uint8_t, 148> readback{};
        if (!cpu_.write(arg, info.data(), uint32_t(info.size())) ||
            !cpu_.read(arg, readback.data(), uint32_t(readback.size())) || readback != info) {
            fprintf(log_, "startup_service_error=version_buffer_write_or_readback_failed\n");
            return StartupServiceResult::ContractFailure;
        }
        fprintf(log_, "startup_version_profile=winxp_5.1_2600_no_service_pack\n");
        fprintf(log_, "startup_version_info_size=148\n");
        fprintf(log_, "startup_version_buffer_va=0x%08X\n", arg);
        fprintf(log_, "startup_version_buffer_readback=passed\n");
        fprintf(log_, "startup_version_return_va=0x%08X\n", ret);
        cpu_.trap_epilogue(1, 8, ret); // BOOL TRUE; stdcall ret 4
        expected_eax = 1;
        ++version_calls_;
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!GetVersionExA\n");
    } else if (module) {
        // The runtime implements KERNEL32 imports; identify that compatibility
        // module with an opaque handle. It is not an executable Windows DLL.
        if (arg) {
            char name[64] = {};
            unsigned n = 0;
            for (; n < sizeof(name); ++n) {
                if (uint64_t(arg) + n > 0xFFFFFFFFull || !cpu_.read(arg + n, &name[n], 1))
                    return StartupServiceResult::ContractFailure;
                if (!name[n]) break;
                if (name[n] >= 'A' && name[n] <= 'Z') name[n] += 'a' - 'A';
            }
            if (n == sizeof(name)) return StartupServiceResult::ContractFailure;
            fprintf(log_, "startup_module_requested=%s\n", name);
            if (std::string(name) != "kernel32.dll") return StartupServiceResult::Unsupported;
            const uint32_t handle = 0x00AB1000;
            cpu_.trap_epilogue(handle, 8, ret);
            expected_eax = handle;
            ++module_calls_;
            fprintf(log_, "startup_module_base_returned=0x%08X\n", handle);
            fprintf(log_, "startup_module_handle_kind=runtime_opaque\n");
            fprintf(log_, "startup_serviced_import=KERNEL32.dll!GetModuleHandleA\n");
        } else {
            if (version_calls_ != 1 || module_calls_ || import.iat_va != 0x006570A8 || ret != 0x006423A1) {
                fprintf(log_, "startup_service_error=unsupported_module_call\n");
                return StartupServiceResult::ContractFailure;
            }
            uint16_t mz = 0;
            uint32_t nt_offset = 0, signature = 0;
            const uint32_t base = image_.load_base();
            if (!cpu_.read(base, &mz, 2) || mz != 0x5A4D ||
                !cpu_.read(base + 0x3C, &nt_offset, 4) ||
                uint64_t(nt_offset) + 4 > image_.image_size() ||
                !cpu_.read(base + nt_offset, &signature, 4) || signature != 0x4550) {
                fprintf(log_, "startup_service_error=module_headers_invalid\n");
                return StartupServiceResult::ContractFailure;
            }
            fprintf(log_, "startup_module_base_returned=0x%08X\n", base);
            fprintf(log_, "startup_module_headers=passed\n");
            fprintf(log_, "startup_module_return_va=0x%08X\n", ret);
            cpu_.trap_epilogue(base, 8, ret); // HMODULE; stdcall ret 4
            expected_eax = base;
            ++module_calls_;
            fprintf(log_, "startup_serviced_import=KERNEL32.dll!GetModuleHandleA\n");
        }
    } else if (proc_address) {
        uint32_t symbol = 0;
        if (!in_stack(esp, 12) || !cpu_.read(esp + 8, &symbol, 4) ||
            arg != 0x00AB1000 || symbol <= 0xFFFFu) {
            fprintf(log_, "startup_service_error=unsupported_export_frame\n");
            return StartupServiceResult::ContractFailure;
        }
        char name[128] = {};
        unsigned n = 0;
        for (; n < sizeof(name); ++n) {
            if (uint64_t(symbol) + n > 0xFFFFFFFFull ||
                !cpu_.read(symbol + n, &name[n], 1))
                return StartupServiceResult::ContractFailure;
            if (!name[n]) break;
        }
        if (n == sizeof(name)) return StartupServiceResult::ContractFailure;
        fprintf(log_, "startup_export_requested=%s\n", name);
        if (std::string(name) != "InitializeCriticalSectionAndSpinCount")
            return StartupServiceResult::Unsupported;
        cpu_.trap_epilogue(critical_init_trap, 12, ret);
        expected_eax = critical_init_trap;
        ++proc_address_calls_;
        fprintf(log_, "startup_export_resolved_va=0x%08X\n", critical_init_trap);
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!GetProcAddress\n");
    } else if (critical_init) {
        uint32_t spin = 0;
        uint32_t previous[6] = {};
        if (!in_stack(esp, 12) || !cpu_.read(esp + 8, &spin, 4) || !arg || (arg & 3u) ||
            critical_sections_.count(arg) || !cpu_.read(arg, previous, sizeof(previous))) {
            fprintf(log_, "startup_service_error=invalid_critical_section_frame\n");
            return StartupServiceResult::ContractFailure;
        }
        // x86 RTL_CRITICAL_SECTION: DebugInfo, LockCount, RecursionCount,
        // OwningThread, LockSemaphore, SpinCount. Only initialization is served;
        // lock acquisition remains an explicit boundary until implemented.
        const uint32_t state[6] = {0, 0xFFFFFFFFu, 0, 0, 0, spin};
        uint32_t copy[6] = {};
        if (!cpu_.write(arg, state, sizeof(state)) || !cpu_.read(arg, copy, sizeof(copy)) ||
            !std::equal(std::begin(state), std::end(state), std::begin(copy))) {
            fprintf(log_, "startup_service_error=critical_section_write_failed\n");
            return StartupServiceResult::ContractFailure;
        }
        critical_sections_.insert(arg);
        ++critical_init_calls_;
        cpu_.trap_epilogue(1, 12, ret);
        expected_eax = 1;
        fprintf(log_, "startup_critical_section_va=0x%08X\n", arg);
        fprintf(log_, "startup_critical_section_spin=%u\n", spin);
        fprintf(log_, "startup_critical_section_readback=passed\n");
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!InitializeCriticalSectionAndSpinCount\n");
    } else if (heap_create) {
        uint32_t initial = 0, maximum = 0;
        if (heap_create_calls_ || import.iat_va != 0x00657160 || ret != 0x0064974C ||
            !in_stack(esp, 16) || !cpu_.read(esp + 8, &initial, 4) ||
            !cpu_.read(esp + 12, &maximum, 4) || arg != 0 || initial != 0x1000 || maximum != 0) {
            fprintf(log_, "startup_service_error=unsupported_heap_create_call\n");
            return StartupServiceResult::ContractFailure;
        }
        if (!cpu_.map(heap_next_, 0x00800000, nullptr, d2rt::P_RW)) {
            fprintf(log_, "startup_service_error=heap_map_failed\n");
            return StartupServiceResult::ContractFailure;
        }
        heap_ready_ = true;
        ++heap_create_calls_;
        // trap_epilogue consumes the return address plus all three stdcall arguments.
        cpu_.trap_epilogue(0x00AB0000, 16, ret);
        expected_eax = 0x00AB0000;
        fprintf(log_, "startup_heap_created_handle=0x00AB0000\n");
        fprintf(log_, "startup_heap_initial_bytes=%u\n", initial);
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!HeapCreate\n");
    } else if (heap_alloc) {
        uint32_t flags = 0, size = 0;
        if (!heap_ready_ || !in_stack(esp, 16) || !cpu_.read(esp + 4, &arg, 4) ||
            !cpu_.read(esp + 8, &flags, 4) || !cpu_.read(esp + 12, &size, 4) ||
            arg != 0x00AB0000 || size > 0x007FF000) {
            fprintf(log_, "startup_service_error=unsupported_heap_alloc_call\n");
            return StartupServiceResult::ContractFailure;
        }
        uint32_t bytes = (size + 15u) & ~15u;
        if (!bytes || heap_next_ + bytes > 0x01400000) {
            cpu_.trap_epilogue(0, 16, ret);
            expected_eax = 0;
        } else {
            const uint32_t result = heap_next_;
            heap_next_ += bytes;
            if (flags & 8u) {
                std::vector<uint8_t> zero(bytes, 0);
                cpu_.write(result, zero.data(), bytes);
            }
            ++heap_alloc_calls_;
            cpu_.trap_epilogue(result, 16, ret);
            expected_eax = result;
            fprintf(log_, "startup_heap_alloc_va=0x%08X\n", result);
        }
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!HeapAlloc\n");
    } else if (heap_free) {
        if (!heap_ready_ || import.iat_va != 0x00657098 || !in_stack(esp, 16))
            return StartupServiceResult::ContractFailure;
        cpu_.trap_epilogue(1, 16, ret);
        expected_eax = 1;
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!HeapFree\n");
    } else if (heap_size) {
        if (!heap_ready_ || !in_stack(esp, 16)) return StartupServiceResult::ContractFailure;
        cpu_.trap_epilogue(0, 16, ret);
        expected_eax = 0;
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!HeapSize\n");
    }
    // Check the bridge ABI on each returned API, before another guest block.
    const uint32_t parameter_count = (version || module) ? 1u :
        ((proc_address || critical_init) ? 2u : 3u);
    const uint32_t cleanup = 4u * (1u + parameter_count); // return address + arguments
    fprintf(log_, "startup_service_stack_bytes=%u\n", cleanup);
    bool abi_ok = cpu_.reg(d2rt::R_ESP) == esp + cleanup && cpu_.reg(d2rt::R_EIP) == ret &&
        cpu_.reg(d2rt::R_EAX) == expected_eax;
    for (unsigned i = 0; i < 4; ++i) abi_ok = abi_ok && cpu_.reg(preserved[i]) == before[i];
    if (!abi_ok) {
        fprintf(log_, "startup_service_error=stdcall_epilogue_mismatch\n");
        return StartupServiceResult::ContractFailure;
    }
    fprintf(log_, "startup_service_abi=passed\n");
    return StartupServiceResult::Serviced;
}

bool StartupServices::version_globals_match() {
    // These globals are filled by the ORIGINAL instructions after GetVersionExA.
    // Matching them establishes that the guest consumed our returned struct.
    const uint32_t addresses[] = {0x0068E2E8, 0x0068E2EC, 0x0068E2F0, 0x0068E2F4, 0x0068E2F8};
    const uint32_t expected[] = {kPlatform, kBuild, (kMajor << 8) + kMinor, kMajor, kMinor};
    bool ok = true;
    for (unsigned i = 0; i < 5; ++i) {
        uint32_t value = 0;
        const bool read = cpu_.read(addresses[i], &value, 4);
        fprintf(log_, "startup_version_global=0x%08X value=0x%08X expected=0x%08X\n",
            addresses[i], value, expected[i]);
        ok = ok && read && value == expected[i];
    }
    fprintf(log_, "startup_version_globals=%s\n", ok ? "passed" : "failed");
    return ok;
}
