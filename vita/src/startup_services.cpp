#include "startup_services.h"
#include "runtime/cpu.h"
#include "runtime/pe_image.h"
#include "runtime/guest_thread_ctx.h"
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
    const bool tls_alloc = import.name == "TlsAlloc";
    const bool tls_get = import.name == "TlsGetValue";
    const bool tls_set = import.name == "TlsSetValue";
    const bool tls_free = import.name == "TlsFree";
    const bool tls = tls_alloc || tls_get || tls_set || tls_free;
    const bool get_error = import.name == "GetLastError";
    const bool set_error = import.name == "SetLastError";
    const bool thread_id = import.name == "GetCurrentThreadId";
    const bool startup_info = import.name == "GetStartupInfoA";
    const bool command_line = import.name == "GetCommandLineA";
    const bool std_handle = import.name == "GetStdHandle";
    const bool file_type = import.name == "GetFileType";
    const bool handle_count = import.name == "SetHandleCount";
    const bool environment_get = import.name == "GetEnvironmentStringsW" ||
        import.name == "GetEnvironmentStringsA" || import.name == "GetEnvironmentStrings";
    const bool environment_free = import.name == "FreeEnvironmentStringsW" ||
        import.name == "FreeEnvironmentStringsA";
    const bool environment = environment_get || environment_free;
    const bool wide_to_bytes = import.name == "WideCharToMultiByte";
    const bool process = startup_info || command_line || std_handle || file_type || handle_count;
    const bool heap_create = import.name == "HeapCreate";
    const bool heap_alloc = import.name == "HeapAlloc";
    const bool heap_free = import.name == "HeapFree";
    const bool heap_size = import.name == "HeapSize";
    if (!version && !module && !proc_address && !critical_init && !tls &&
        !get_error && !set_error && !thread_id && !process && !environment && !wide_to_bytes &&
        !heap_create && !heap_alloc && !heap_free && !heap_size)
        return StartupServiceResult::Unsupported;
    const uint32_t esp = cpu_.reg(d2rt::R_ESP);
    const int preserved[] = {d2rt::R_EBX, d2rt::R_EBP, d2rt::R_ESI, d2rt::R_EDI};
    uint32_t before[4];
    for (unsigned i = 0; i < 4; ++i) before[i] = cpu_.reg(preserved[i]);
    uint32_t ret = 0, arg = 0;
    uint32_t expected_eax = 0;
    const uint32_t parameter_count = wide_to_bytes ? 8u :
        ((tls_alloc || get_error || thread_id || command_line || environment_get) ? 0u :
        ((version || module || tls_get || tls_free || set_error || startup_info || std_handle || file_type || handle_count || environment_free) ? 1u :
        ((proc_address || critical_init || tls_set) ? 2u : 3u)));
    const uint32_t cleanup = 4u * (1u + parameter_count);
    if (!in_stack(esp, cleanup) || !cpu_.read(esp, &ret, 4) ||
        (parameter_count && !cpu_.read(esp + 4, &arg, 4))) {
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
        const std::string symbol_name(name);
        if (symbol_name == "FlsAlloc" || symbol_name == "FlsFree" ||
            symbol_name == "FlsGetValue" || symbol_name == "FlsSetValue") {
            // The advertised XP profile uses the CRT's existing TLS fallback.
            expected_eax = 0;
            wx86_set_lasterr(cpu_, 127); // ERROR_PROC_NOT_FOUND
            ++unavailable_export_calls_;
            fprintf(log_, "startup_export_availability=not_available_xp_profile\n");
        } else if (symbol_name == "InitializeCriticalSectionAndSpinCount") {
            expected_eax = critical_init_trap;
        } else return StartupServiceResult::Unsupported;
        cpu_.trap_epilogue(expected_eax, 12, ret);
        ++proc_address_calls_;
        fprintf(log_, "startup_export_resolved_va=0x%08X\n", expected_eax);
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
    } else if (tls) {
        const uint32_t slots = wx86_cur_tib() + 0xE10;
        if (!wx86_cur_tib()) return StartupServiceResult::ContractFailure;
        if (tls_alloc) {
            expected_eax = 0xFFFFFFFFu;
            for (uint32_t i = 0; i < tls_allocated_.size(); ++i) {
                if (tls_allocated_[i]) continue;
                const uint32_t zero = 0;
                if (!cpu_.write(slots + 4 * i, &zero, 4))
                    return StartupServiceResult::ContractFailure;
                tls_allocated_[i] = true;
                expected_eax = i;
                fprintf(log_, "startup_tls_index=%u\n", i);
                break;
            }
            if (expected_eax == 0xFFFFFFFFu) wx86_set_lasterr(cpu_, 259);
        } else if (arg >= tls_allocated_.size() || !tls_allocated_[arg]) {
            wx86_set_lasterr(cpu_, 87);
            expected_eax = 0;
        } else if (tls_get) {
            if (!cpu_.read(slots + 4 * arg, &expected_eax, 4))
                return StartupServiceResult::ContractFailure;
            wx86_set_lasterr(cpu_, 0);
        } else {
            uint32_t value = 0;
            if (tls_set && !cpu_.read(esp + 8, &value, 4))
                return StartupServiceResult::ContractFailure;
            if (!cpu_.write(slots + 4 * arg, &value, 4))
                return StartupServiceResult::ContractFailure;
            if (tls_free) tls_allocated_[arg] = false;
            expected_eax = 1;
        }
        ++tls_calls_;
        cpu_.trap_epilogue(expected_eax, cleanup, ret);
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!%s\n", import.name.c_str());
    } else if (get_error || set_error || thread_id) {
        if (get_error) expected_eax = wx86_get_lasterr(cpu_);
        if (set_error) wx86_set_lasterr(cpu_, arg);
        if (thread_id && (!wx86_cur_tib() || !cpu_.read(wx86_cur_tib() + 0x24, &expected_eax, 4)))
            return StartupServiceResult::ContractFailure;
        cpu_.trap_epilogue(expected_eax, cleanup, ret);
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!%s\n", import.name.c_str());
    } else if (wide_to_bytes) {
        uint32_t args[8] = {};
        if (!cpu_.read(esp + 4, args, sizeof(args))) return StartupServiceResult::ContractFailure;
        const uint32_t page = args[0], flags = args[1], source = args[2];
        const uint32_t count = args[3], dest = args[4], capacity = args[5], used = args[7];
        fprintf(log_, "startup_conversion_code_page=%u\n", page);
        fprintf(log_, "startup_conversion_input_units=%u\n", count);
        if (page != 0 && page != 1 && page != 932 && page != 1252 && page != 65001)
            return StartupServiceResult::Unsupported;
        if (flags != 0) return StartupServiceResult::Unsupported;
        uint32_t error = 0;
        if (!source || !count || (count > 0x7FFFFFFFu && count != 0xFFFFFFFFu) ||
            capacity > 0x7FFFFFFFu || (capacity && (!dest || dest == source)) ||
            (page == 65001 && (args[6] || used))) error = 87;
        std::vector<uint8_t> bytes;
        const uint32_t max_units = 16384;
        if (!error) {
            if (count != 0xFFFFFFFFu && count > max_units) {
                fprintf(log_, "startup_conversion_scope=input_limit\n");
                return StartupServiceResult::Unsupported;
            }
            const uint32_t limit = count == 0xFFFFFFFFu ? max_units : count;
            bool terminated = false;
            for (uint32_t i = 0; i < limit; ++i) {
                uint16_t unit = 0;
                if (uint64_t(source) + 2ull*i + 2 > 0x100000000ull ||
                    !cpu_.read(source + 2*i, &unit, 2)) return StartupServiceResult::ContractFailure;
                if (unit > 127) {
                    fprintf(log_, "startup_conversion_scope=non_ascii_not_implemented\n");
                    return StartupServiceResult::Unsupported;
                }
                bytes.push_back(uint8_t(unit));
                if (count == 0xFFFFFFFFu && unit == 0) { terminated = true; break; }
            }
            if (count == 0xFFFFFFFFu && !terminated) return StartupServiceResult::Unsupported;
            if (capacity && capacity < bytes.size()) error = 122;
        }
        if (error) {
            wx86_set_lasterr(cpu_, error);
        } else {
            uint32_t previous_used = 0;
            if (used && !cpu_.read(used, &previous_used, 4)) return StartupServiceResult::ContractFailure;
            if (capacity) {
                if (uint64_t(dest) + bytes.size() > 0x100000000ull)
                    return StartupServiceResult::ContractFailure;
                std::vector<uint8_t> copy(bytes.size());
                if (!cpu_.read(dest, copy.data(), uint32_t(copy.size())) ||
                    !cpu_.write(dest, bytes.data(), uint32_t(bytes.size())) ||
                    !cpu_.read(dest, copy.data(), uint32_t(copy.size())) || copy != bytes)
                    return StartupServiceResult::ContractFailure;
                fprintf(log_, "startup_conversion_readback=passed\n");
            }
            const uint32_t false_value = 0;
            if (used && !cpu_.write(used, &false_value, 4)) return StartupServiceResult::ContractFailure;
            expected_eax = uint32_t(bytes.size());
            fprintf(log_, "startup_conversion_mode=%s\n", capacity ? "write" : "size_query");
            fprintf(log_, "startup_conversion_output_bytes=%u\n", expected_eax);
        }
        cpu_.trap_epilogue(expected_eax, cleanup, ret);
        ++conversion_calls_;
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!WideCharToMultiByte\n");
    } else if (environment) {
        const bool wide = import.name.back() == 'W';
        if (environment_get) {
            // Owned copy of the empty process environment: two terminating NULs.
            if (!heap_ready_) return StartupServiceResult::ContractFailure;
            if (environment_free_blocks_.empty() && uint64_t(heap_next_) + 16 > 0x01400000u) {
                wx86_set_lasterr(cpu_, 8); // ERROR_NOT_ENOUGH_MEMORY
            } else {
                const uint32_t zero = 0;
                uint32_t copy = 1;
                const uint32_t address = environment_free_blocks_.empty() ?
                    heap_next_ : environment_free_blocks_.back();
                if (!cpu_.write(address, &zero, 4) ||
                    !cpu_.read(address, &copy, 4) || copy != 0)
                    return StartupServiceResult::ContractFailure;
                expected_eax = address;
                if (environment_free_blocks_.empty()) heap_next_ += 16;
                else environment_free_blocks_.pop_back();
                environment_blocks_.emplace(expected_eax, wide);
                fprintf(log_, "startup_environment_block_va=0x%08X\n", expected_eax);
                fprintf(log_, "startup_environment_profile=empty\n");
                fprintf(log_, "startup_environment_readback=passed\n");
            }
        } else {
            const auto block = environment_blocks_.find(arg);
            if (block == environment_blocks_.end() || block->second != wide) {
                wx86_set_lasterr(cpu_, 87);
            } else {
                environment_free_blocks_.push_back(arg);
                environment_blocks_.erase(block);
                expected_eax = 1;
                fprintf(log_, "startup_environment_release=passed\n");
            }
        }
        ++environment_calls_;
        cpu_.trap_epilogue(expected_eax, cleanup, ret);
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!%s\n", import.name.c_str());
    } else if (process) {
        if (startup_info) {
            // GUI process without inherited CRT handles or reserved startup data.
            const uint32_t info[17] = {68};
            uint32_t old[17] = {}, copy[17] = {};
            if (!arg || uint64_t(arg) + sizeof(info) > 0x100000000ull ||
                !cpu_.read(arg, old, sizeof(old)) || !cpu_.write(arg, info, sizeof(info)) ||
                !cpu_.read(arg, copy, sizeof(copy)) ||
                !std::equal(std::begin(info), std::end(info), std::begin(copy))) {
                fprintf(log_, "startup_service_error=invalid_startup_info_buffer\n");
                return StartupServiceResult::ContractFailure;
            }
            fprintf(log_, "startup_info_bytes=68\n");
            fprintf(log_, "startup_info_readback=passed\n");
        } else if (command_line) {
            // ThreadSmoke owns the persistent, NUL-terminated ANSI command line.
            expected_eax = 0x00732000;
            char first = 0;
            if (!cpu_.read(expected_eax, &first, 1) || first != '"') {
                fprintf(log_, "startup_service_error=command_line_not_initialized\n");
                return StartupServiceResult::ContractFailure;
            }
            fprintf(log_, "startup_command_line_va=0x%08X\n", expected_eax);
        } else if (std_handle) {
            if (arg != 0xFFFFFFF6u && arg != 0xFFFFFFF5u && arg != 0xFFFFFFF4u) {
                expected_eax = 0xFFFFFFFFu;
                wx86_set_lasterr(cpu_, 87);
            } else {
                // NULL means that this GUI process has no attached console.
                expected_eax = 0;
                fprintf(log_, "startup_standard_handle_profile=no_console\n");
            }
        } else if (handle_count) {
            // Legacy API is a no-op on NT; return the requested count.
            expected_eax = arg;
            fprintf(log_, "startup_handle_count_requested=%u\n", arg);
        } else {
            if (arg != 0 && arg != 0xFFFFFFFFu) return StartupServiceResult::Unsupported;
            expected_eax = 0; // FILE_TYPE_UNKNOWN, invalid NULL/INVALID_HANDLE_VALUE
            wx86_set_lasterr(cpu_, 6);
        }
        cpu_.trap_epilogue(expected_eax, cleanup, ret);
        ++process_calls_;
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!%s\n", import.name.c_str());
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
