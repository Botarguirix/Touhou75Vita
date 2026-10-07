#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>

#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "runtime/cpu.h"
#include "runtime/pe_image.h"
#include "platform/vita_host.h"
#include "import_smoke.h"
#include "sha256.h"
#include "diagnostic_screen.h"
#include "startup_probe.h"
#include "th075_assets.h"

#define APP_DIR "ux0:data/TH075Vita"
#define GAME_EXE_PATH APP_DIR "/TH075.exe"
#define LOG_PATH APP_DIR "/iteration49.log"
#define BUILD_ID "iteration49-boot-startup-r1"

static const uint32_t kArenaGuestLimit = 0x01000000;
static const uint32_t kSmokeResult = 0x00000075;
static const char* const kExpectedGameSha256 =
    "BD441E99075436E8DCAD26F86FFCF5E6AAC4F58B0ED3EE7442E4CB39D8E22C98";

extern "C" const char* const wx86_vita_progress_path = APP_DIR "/iteration49-runtime.log";

static void write_u32_le(uint8_t* out, uint32_t value) {
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8);
    out[2] = (uint8_t)(value >> 16);
    out[3] = (uint8_t)(value >> 24);
}

static bool read_file(const char* path, std::vector<uint8_t>& bytes,
                      FILE* log, const char* prefix) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(log, "%s_file_result=missing_or_unreadable\n", prefix);
        return false;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fprintf(log, "%s_file_result=seek_failed\n", prefix);
        fclose(file);
        return false;
    }

    long length = ftell(file);
    if (length < 0 || length > 64 * 1024 * 1024 ||
        fseek(file, 0, SEEK_SET) != 0) {
        fprintf(log, "%s_file_result=invalid_size_or_seek_failed\n", prefix);
        fclose(file);
        return false;
    }

    bytes.resize((size_t)length);
    const bool ok = bytes.empty() ||
                    fread(bytes.data(), 1, bytes.size(), file) == bytes.size();
    fclose(file);
    if (!ok) {
        bytes.clear();
        fprintf(log, "%s_file_result=short_read\n", prefix);
        return false;
    }

    fprintf(log, "%s_file_size_bytes=%u\n", prefix,
            (unsigned)bytes.size());
    fprintf(log, "%s_expected_sha256=%s\n", prefix, kExpectedGameSha256);
    const std::string actual = sha256_hex(bytes.data(), bytes.size());
    const bool matched = actual == kExpectedGameSha256;
    fprintf(log, "%s_actual_sha256=%s\n", prefix, actual.c_str());
    fprintf(log, "%s_sha256_result=%s\n", prefix, matched ? "passed" : "failed");
    fprintf(log, "%s_sha256_verified_on_vita=%s\n", prefix, matched ? "yes" : "no");
    return matched;
}

static bool load_game_pe(const std::vector<uint8_t>& bytes,
                         d2rt::PeImage& image, FILE* log) {
    if (bytes.size() != 2576384) {
        fprintf(log, "game_file_result=unexpected_size\n");
        return false;
    }

    std::string error;
    /* A zero base asks PeImage to use the PE's preferred ImageBase. */
    if (!image.load(bytes, 0, error)) {
        fprintf(log, "game_pe_load_result=failed\n");
        fprintf(log, "game_pe_load_error=%s\n", error.c_str());
        return false;
    }

    fprintf(log, "game_image_base=0x%08X\n", image.load_base());
    fprintf(log, "game_image_size_bytes=%u\n", image.image_size());
    fprintf(log, "game_expected_image_base=0x00400000\n");
    if (image.load_base() != 0x00400000) {
        fprintf(log, "game_pe_load_result=unexpected_image_base\n");
        return false;
    }
    fprintf(log, "game_pe_load_result=loaded_by_winvita_peimage\n");
    fprintf(log, "game_entry_rva=0x%08X\n", image.entry_rva());
    fprintf(log, "game_entry_va=0x%08X\n", image.entry_va());
    fprintf(log, "game_subsystem=%u\n", image.subsystem());
    fprintf(log, "game_imports_total=%u\n",
            (unsigned)image.imports().size());

    std::map<std::string, unsigned> imports_by_module;
    for (const auto& import : image.imports()) {
        ++imports_by_module[import.dll];
    }
    fprintf(log, "game_import_modules=%u\n",
            (unsigned)imports_by_module.size());
    for (const auto& module : imports_by_module) {
        fprintf(log, "game_import_module=%s count=%u\n",
                module.first.c_str(), module.second);
    }
    for (const auto& import : image.imports()) {
        const std::string symbol = import.name.empty()
            ? ("#" + std::to_string(import.ordinal)) : import.name;
        fprintf(log, "game_import=%s!%s iat=0x%08X\n",
                import.dll.c_str(), symbol.c_str(), import.iat_va);
    }
    fprintf(log, "game_import_resolution=not_attempted\n");
    return true;
}

static bool run_dynarec_smoke(d2rt::Cpu& cpu, uint32_t code_va,
                              uint32_t stack_va, uint32_t trap_va,
                              FILE* log) {
    uint8_t code[10] = {
        0xB8, 0x75, 0x00, 0x00, 0x00, /* mov eax, 0x75 */
        0xE9, 0x00, 0x00, 0x00, 0x00  /* jmp trap */
    };
    const uint32_t next_eip = code_va + sizeof(code);
    write_u32_le(&code[6], trap_va - next_eip);

    if (!cpu.map(code_va, 0x1000, nullptr, d2rt::P_RWX)) {
        fprintf(log, "dynarec_smoke_result=failed\n");
        fprintf(log, "dynarec_smoke_error=code_map_failed\n");
        return false;
    }
    if (!cpu.map(stack_va, 0x1000, nullptr, d2rt::P_RW)) {
        fprintf(log, "dynarec_smoke_result=failed\n");
        fprintf(log, "dynarec_smoke_error=stack_map_failed\n");
        return false;
    }
    if (!cpu.write(code_va, code, sizeof(code))) {
        fprintf(log, "dynarec_smoke_result=failed\n");
        fprintf(log, "dynarec_smoke_error=guest_code_write_failed\n");
        return false;
    }

    // CpuBox86 initializes trap stubs only for its first trap window.
    // Reserve the same complete window that Bridge will use afterwards.
    cpu.set_trap(trap_va, trap_va + 0x00100000,
                 [](d2rt::Cpu&, uint32_t) { return false; });
    cpu.set_reg(d2rt::R_ESP, stack_va + 0x1000);
    cpu.set_reg(d2rt::R_EFLAGS, 0x202);

    const char* fault = nullptr;
    const bool clean_stop = cpu.run(code_va, &fault);
    const uint32_t eax = cpu.reg(d2rt::R_EAX);
    const uint32_t final_eip = cpu.reg(d2rt::R_EIP);
    fprintf(log, "dynarec_smoke_guest_code_va=0x%08X\n", code_va);
    fprintf(log, "dynarec_smoke_trap_va=0x%08X\n", trap_va);
    fprintf(log, "dynarec_smoke_eax=0x%08X\n", eax);
    fprintf(log, "dynarec_smoke_final_eip=0x%08X\n", final_eip);
    if (!clean_stop) {
        fprintf(log, "dynarec_smoke_result=failed\n");
        fprintf(log, "dynarec_smoke_fault=%s\n",
                fault != nullptr ? fault : "unknown");
        fprintf(log, "dynarec_smoke_fault_va=0x%08X\n", cpu.fault_addr());
        return false;
    }

    const bool passed = eax == kSmokeResult && final_eip == trap_va;
    fprintf(log, "dynarec_smoke_result=%s\n", passed ? "passed" : "failed");
    return passed;
}

static int run(FILE* log) {
    if(!th075::load_title_asset(log))fprintf(log,"dat_title_preview=unavailable\n");
    fprintf(log, "Touhou 7.5 Vita - Iteration 49 original EXE process startup\n");
    fprintf(log, "build_id=%s\n", BUILD_ID);
    errno = 0;
    const int old_watchdog = remove(APP_DIR "/iteration49-watchdog.log");
    const int watchdog_errno = errno;
    fprintf(log, "watchdog_previous_log_cleared=%s\n",
        old_watchdog == 0 || watchdog_errno == ENOENT ? "yes" : "no");
    if (old_watchdog != 0 && watchdog_errno != ENOENT)
        fprintf(log, "watchdog_previous_log_clear_errno=%d\n", watchdog_errno);
    fprintf(log, "target_cpu=ARMv7\n");
    fprintf(log, "execution=diagnostics_then_bounded_original_exe_startup\n");
    fprintf(log, "game_entrypoint=not_attempted\n");
    fprintf(log, "game_code_executed=no\n");
    fprintf(log, "tls_callbacks=not_attempted\n");
    fprintf(log, "translation_patch=not_loaded\n");
    const char* abc = "abc";
    const char* long_vector = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    const bool hash_selfcheck =
        sha256_hex(nullptr, 0) == "E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855" &&
        sha256_hex(reinterpret_cast<const uint8_t*>(abc), 3) ==
            "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD" &&
        sha256_hex(reinterpret_cast<const uint8_t*>(long_vector), strlen(long_vector)) ==
            "248D6A61D20638B8E5C026930C3E6039A33CE45964FF2167F6ECEDD419DB06C1";
    fprintf(log, "sha256_selfcheck=%s\n", hash_selfcheck ? "passed" : "failed");
    if (!hash_selfcheck) { fprintf(log, "result=hash_selfcheck_failed\n"); return 1; }

    std::vector<uint8_t> exe_bytes;
    d2rt::PeImage game_image;
    const bool file_read = read_file(GAME_EXE_PATH, exe_bytes, log, "game");
    const bool pe_loaded = file_read && load_game_pe(exe_bytes, game_image, log);
    if (!pe_loaded) { fprintf(log, "result=input_validation_failed\n"); return 1; }

    if (setenv("WX86_ARENA", "02000000", 1) != 0) {
        fprintf(log, "dynarec_backend=Box86-derived-ARMv7\n");
        fprintf(log, "dynarec_init_result=failed\n");
        fprintf(log, "dynarec_init_error=setenv_failed\n");
        return 1;
    }
    fprintf(log, "guest_arena_requested_bytes=0x02000000\n");
    fprintf(log, "guest_address_span_for_smoke=0x01000000\n");

    std::unique_ptr<d2rt::Cpu> cpu(d2rt::make_cpu_box86());
    if (!cpu) {
        fprintf(log, "dynarec_backend=Box86-derived-ARMv7\n");
        fprintf(log, "dynarec_init_result=failed\n");
        fprintf(log, "dynarec_init_error=cpu_backend_unavailable\n");
        return 1;
    }
    fprintf(log, "dynarec_backend=Box86-derived-ARMv7\n");
    fprintf(log, "dynarec_init_result=created\n");

    bool game_mapped = false;
    if (pe_loaded) {
        if ((uint64_t)game_image.load_base() + game_image.image_size() > 0x00700000) {
            fprintf(log, "game_guest_map_error=image_exceeds_reserved_region\n");
            return 1;
        }
        const bool mapped = cpu->map(game_image.load_base(),
                                     game_image.image_size(),
                                     game_image.image().data(), d2rt::P_RWX);
        game_mapped = mapped;
        fprintf(log, "game_guest_map_result=%s\n", mapped ? "passed" : "failed");
        if (!mapped) {
            fprintf(log, "game_guest_map_error=cpu_map_failed\n");
        }
    } else {
        fprintf(log, "game_guest_map_result=skipped\n");
    }

    const uint64_t code64 = 0x00700000;
    const uint64_t stack64 = 0x00800000;
    const uint64_t trap64 = 0x00B00000;
    if (trap64 + 0x00100000u > kArenaGuestLimit) {
        fprintf(log, "dynarec_smoke_result=failed\n");
        fprintf(log, "dynarec_smoke_error=guest_arena_too_small_for_image\n");
        return 1;
    }

    const bool smoke_passed = run_dynarec_smoke(
        *cpu, (uint32_t)code64, (uint32_t)stack64, (uint32_t)trap64, log);
    const bool imports_passed = pe_loaded && game_mapped && smoke_passed &&
        run_import_smoke(*cpu, exe_bytes, log);
    const bool diagnostics_passed = pe_loaded && game_mapped && smoke_passed && imports_passed;
    fprintf(log, "preflight_result=%s\n", diagnostics_passed ? "passed" : "failed");
    const bool passed = diagnostics_passed && run_startup_probe(*cpu, game_image, exe_bytes, log);
    if (!diagnostics_passed) fprintf(log, "startup_result=preflight_failed\n");
    fprintf(log, "result=%s\n", passed ? "real_entrypoint_version_module_passed" : "failed");
    return passed ? 0 : 1;
}

int main(void) {
    sceIoMkdir(APP_DIR, 0777);
    FILE* progress = fopen(wx86_vita_progress_path, "wb");
    if (progress) fclose(progress);
    FILE* log = fopen(LOG_PATH, "wb");
    if (log == NULL) {
        return 2;
    }
    // Group each import's diagnostic lines into one native write. The
    // startup trap flushes at every boundary; the watchdog has its own file.
    static char diagnostic_buffer[8192];
    setvbuf(log, diagnostic_buffer, _IOFBF, sizeof(diagnostic_buffer));

    const int result = run(log);
    fflush(log); // The results screen reopens this file to read final status.
    show_diagnostic_screen(LOG_PATH, result, log);
    fflush(log);
    fclose(log);
    return result;
}
