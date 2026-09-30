#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>

#include <limits.h>
#include <stdint.h>
#include <stdio.h>

#define APP_DIR "ux0:data/TH075Vita"
#define GAME_EXE_PATH APP_DIR "/TH075.exe"
#define ENGLISH_PATCH_PATH APP_DIR "/TH075E.exe"
#define LOG_PATH APP_DIR "/iteration06.log"

enum {
    PE_PROBE_OK = 0,
    PE_PROBE_BAD_DOS_HEADER = 1,
    PE_PROBE_BAD_PE_HEADER = 2,
    PE_PROBE_UNSUPPORTED_TARGET = 3,
};

static uint16_t read_u16_le(const uint8_t *bytes) {
    return (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
}

static uint32_t read_u32_le(const uint8_t *bytes) {
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
}

static int read_at(FILE *file, long file_size, uint64_t offset,
                   void *buffer, size_t length) {
    if (offset > (uint64_t)file_size ||
        (uint64_t)length > (uint64_t)file_size - offset ||
        offset > (uint64_t)LONG_MAX) {
        return 0;
    }

    if (fseek(file, (long)offset, SEEK_SET) != 0) {
        return 0;
    }

    return fread(buffer, 1, length, file) == length;
}

static int inspect_pe32(FILE *exe, FILE *log, const char *prefix) {
    uint8_t dos_header[64];
    uint8_t nt_headers[24];
    uint8_t optional_header[72];
    long file_size;
    uint32_t pe_offset;
    uint16_t machine;
    uint16_t section_count;
    uint16_t optional_size;
    uint16_t optional_magic;
    uint16_t subsystem;
    uint32_t entry_rva;
    uint32_t image_base;

    if (fseek(exe, 0, SEEK_END) != 0 || (file_size = ftell(exe)) < 0) {
        fprintf(log, "%s_probe_error=file_size_unavailable\n", prefix);
        return PE_PROBE_BAD_DOS_HEADER;
    }

    fprintf(log, "%s_exe_size_bytes=%ld\n", prefix, file_size);

    if (!read_at(exe, file_size, 0, dos_header, sizeof(dos_header)) ||
        dos_header[0] != 'M' || dos_header[1] != 'Z') {
        fprintf(log, "%s_probe_error=invalid_dos_header\n", prefix);
        return PE_PROBE_BAD_DOS_HEADER;
    }

    pe_offset = read_u32_le(&dos_header[0x3c]);
    fprintf(log, "%s_pe_header_offset=0x%08X\n", prefix,
            (unsigned)pe_offset);

    if (!read_at(exe, file_size, pe_offset, nt_headers, sizeof(nt_headers)) ||
        nt_headers[0] != 'P' || nt_headers[1] != 'E' ||
        nt_headers[2] != 0 || nt_headers[3] != 0) {
        fprintf(log, "%s_probe_error=invalid_pe_signature\n", prefix);
        return PE_PROBE_BAD_PE_HEADER;
    }

    machine = read_u16_le(&nt_headers[4]);
    section_count = read_u16_le(&nt_headers[6]);
    optional_size = read_u16_le(&nt_headers[20]);
    fprintf(log, "%s_pe_machine=0x%04X\n", prefix, (unsigned)machine);
    fprintf(log, "%s_pe_sections=%u\n", prefix, (unsigned)section_count);
    fprintf(log, "%s_pe_optional_header_size=%u\n", prefix,
            (unsigned)optional_size);

    if (optional_size < sizeof(optional_header) ||
        !read_at(exe, file_size, (uint64_t)pe_offset + sizeof(nt_headers),
                 optional_header, sizeof(optional_header))) {
        fprintf(log, "%s_probe_error=optional_header_truncated\n", prefix);
        return PE_PROBE_BAD_PE_HEADER;
    }

    optional_magic = read_u16_le(&optional_header[0]);
    fprintf(log, "%s_pe_optional_magic=0x%04X\n", prefix,
            (unsigned)optional_magic);
    if (optional_magic != 0x010b) {
        fprintf(log, "%s_probe_error=not_pe32\n", prefix);
        return PE_PROBE_UNSUPPORTED_TARGET;
    }

    entry_rva = read_u32_le(&optional_header[16]);
    image_base = read_u32_le(&optional_header[28]);
    subsystem = read_u16_le(&optional_header[68]);
    fprintf(log, "%s_pe_entry_rva=0x%08X\n", prefix, (unsigned)entry_rva);
    fprintf(log, "%s_pe_image_base=0x%08X\n", prefix, (unsigned)image_base);
    fprintf(log, "%s_pe_entry_va=0x%08llX\n", prefix,
            (unsigned long long)((uint64_t)image_base + entry_rva));
    fprintf(log, "%s_pe_subsystem=%u\n", prefix, (unsigned)subsystem);

    if (machine != 0x014c) {
        fprintf(log, "%s_probe_result=valid_pe32_but_not_x86\n", prefix);
        return PE_PROBE_UNSUPPORTED_TARGET;
    }

    fprintf(log, "%s_probe_result=recognized_x86_pe32\n", prefix);
    return PE_PROBE_OK;
}

static void inspect_executable(const char *path, const char *prefix, FILE *log) {
    FILE *exe = fopen(path, "rb");

    fprintf(log, "%s_exe_path=%s\n", prefix, path);
    if (exe == NULL) {
        fprintf(log, "%s_probe_result=game_executable_missing\n", prefix);
        return;
    }

    inspect_pe32(exe, log, prefix);
    fclose(exe);
}

int main(void) {
    FILE *log;

    /* The title can start without game files; keep all proprietary data external. */
    sceIoMkdir(APP_DIR, 0777);

    log = fopen(LOG_PATH, "wb");
    if (log == NULL) {
        return 2;
    }

    fprintf(log, "Touhou 7.5 Vita - Iteration 06 game and translation patch diagnostic\n");
    fprintf(log, "execution=not_attempted\n");
    inspect_executable(GAME_EXE_PATH, "game", log);
    inspect_executable(ENGLISH_PATCH_PATH, "english_patch", log);

    fclose(log);
    return 0;
}
