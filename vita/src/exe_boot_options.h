#pragma once
#include <array>
#include <cstdint>

// User-requested boot preset for the hash-checked Japanese 1.11 EXE only.
// Applied to mapped code before execution, never to the file or guest counters.
namespace exe_boot {
struct Patch {
    uint32_t address;
    unsigned size;
    std::array<uint8_t,7> original, replacement;
};
inline constexpr std::array<Patch,10> menu_patches={{
    // Timed logo: leave after its first genuine update/Present, not update 181.
    {0x00425608,6,{0x81,0xF9,0xB4,0,0,0},{0x81,0xF9,0,0,0,0}},
    // The original dispatcher accepts 0x2202 (title) instead of 0x220D (opening).
    {0x00425610,4,{0x66,0xB8,0x0D,0x22},{0x66,0xB8,0x02,0x22}},
    // Keep menu confirmation enabled and suppress its timed attract demo.
    // Only unsigned age thresholds change; input, drawing and cleanup still run.
    {0x0043A2C4,7,{0x81,0x7A,0x0C,0xA8,0xDE,0,0},{0x81,0x7A,0x0C,0xFF,0xFF,0xFF,0xFF}},
    {0x0043A625,7,{0x81,0x79,0x0C,0x60,0xEA,0,0},{0x81,0x79,0x0C,0xFF,0xFF,0xFF,0xFF}},
    // Initial owner: allocate the real title object's 0x3C bytes, call its
    // original constructor and enter its original 0x2002 state. This avoids
    // constructing/loading/drawing the logo altogether. Allocation failure,
    // common initialization, state setup and owner cleanup stay intact.
    {0x00602EAF,2,{0x6A,0x10},{0x6A,0x3C}},
    {0x00602ECE,5,{0xE8,0xBD,0x25,0xE2,0xFF},{0xE8,0x4D,0x71,0xE3,0xFF}},
    {0x00602EFD,6,{0x66,0xC7,0x45,0xC4,0x04,0x20},{0x66,0xC7,0x45,0xC4,0x02,0x20}},
    // Title layers start at the same opaque endpoint reached by the original
    // fade (5 + 25*10). Menu highlight alpha, input and scene exits are kept.
    {0x0043A0F2,4,{0xC6,0x41,0x21,0x05},{0xC6,0x41,0x21,0xFF}},
    {0x0043A0F9,4,{0xC6,0x42,0x22,0x05},{0xC6,0x42,0x22,0xFF}},
    {0x0043A100,4,{0xC6,0x40,0x23,0x05},{0xC6,0x40,0x23,0xFF}}
}};
inline unsigned modified_bytes() {
    unsigned count=0;
    for(const auto& patch:menu_patches)for(unsigned i=0;i<patch.size;++i)
        count+=patch.original[i]!=patch.replacement[i];
    return count;
}
enum class Result { Applied, FingerprintMismatch, ReadFailure, WriteFailure, RollbackFailure };
template<class Read,class Write>
Result apply_menu_preset(Read read,Write write) {
    // Validate every site before the first mutation.
    for(const auto& patch:menu_patches) {
        std::array<uint8_t,7> bytes{};
        if(!read(patch.address,bytes.data(),patch.size))return Result::ReadFailure;
        for(unsigned i=0;i<patch.size;++i)
            if(bytes[i]!=patch.original[i])return Result::FingerprintMismatch;
    }
    auto rollback=[&] {
        bool good=true;
        for(const auto& patch:menu_patches) {
            if(!write(patch.address,patch.original.data(),patch.size))good=false;
            std::array<uint8_t,7> bytes{};
            if(!read(patch.address,bytes.data(),patch.size))good=false;
            for(unsigned i=0;i<patch.size;++i)if(bytes[i]!=patch.original[i])good=false;
        }
        return good;
    };
    for(const auto& patch:menu_patches) {
        std::array<uint8_t,7> bytes{};
        bool good=write(patch.address,patch.replacement.data(),patch.size) &&
            read(patch.address,bytes.data(),patch.size);
        for(unsigned i=0;i<patch.size;++i)good=good && bytes[i]==patch.replacement[i];
        if(!good)return rollback()?Result::WriteFailure:Result::RollbackFailure;
    }
    return Result::Applied;
}
}
