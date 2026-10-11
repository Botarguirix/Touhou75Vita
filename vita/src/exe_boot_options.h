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
inline constexpr std::array<Patch,4> menu_patches={{
    // Timed logo: leave after its first genuine update/Present, not update 181.
    {0x00425608,6,{0x81,0xF9,0xB4,0,0,0},{0x81,0xF9,0,0,0,0}},
    // The original dispatcher accepts 0x2202 (title) instead of 0x220D (opening).
    {0x00425610,4,{0x66,0xB8,0x0D,0x22},{0x66,0xB8,0x02,0x22}},
    // Keep menu confirmation enabled and suppress its timed attract demo.
    // Only unsigned age thresholds change; input, drawing and cleanup still run.
    {0x0043A2C4,7,{0x81,0x7A,0x0C,0xA8,0xDE,0,0},{0x81,0x7A,0x0C,0xFF,0xFF,0xFF,0xFF}},
    {0x0043A625,7,{0x81,0x79,0x0C,0x60,0xEA,0,0},{0x81,0x79,0x0C,0xFF,0xFF,0xFF,0xFF}}
}};
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
