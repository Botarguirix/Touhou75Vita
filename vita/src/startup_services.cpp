#include "startup_services.h"
#include "runtime/cpu.h"
#include "runtime/pe_image.h"
#include <array>

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
    if (!version && !module) return StartupServiceResult::Unsupported;
    const uint32_t esp = cpu_.reg(d2rt::R_ESP);
    const int preserved[] = {d2rt::R_EBX, d2rt::R_EBP, d2rt::R_ESI, d2rt::R_EDI};
    uint32_t before[4];
    for (unsigned i = 0; i < 4; ++i) before[i] = cpu_.reg(preserved[i]);
    uint32_t ret = 0, arg = 0;
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
        ++version_calls_;
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!GetVersionExA\n");
    } else {
        // Named modules are not handled here. The loaded EXE itself is real
        // guest memory and its DOS/PE headers must be readable before return.
        if (arg) {
            fprintf(log_, "startup_module_variant=named_module_unsupported\n");
            return StartupServiceResult::Unsupported;
        }
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
        ++module_calls_;
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!GetModuleHandleA\n");
    }
    // Check the bridge ABI on each returned API, before another guest block.
    bool abi_ok = cpu_.reg(d2rt::R_ESP) == esp + 8 && cpu_.reg(d2rt::R_EIP) == ret &&
        cpu_.reg(d2rt::R_EAX) == (version ? 1u : image_.load_base());
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
