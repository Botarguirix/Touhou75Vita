#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
namespace d2rt {
enum { R_EAX, R_ECX, R_EDX, R_EBX, R_ESP, R_EBP, R_ESI, R_EDI, R_EIP, P_RW };
struct Cpu {
    std::vector<uint8_t> memory = std::vector<uint8_t>(0x01400000);
    uint32_t registers[9] = {};
    uint32_t reg(int r) const { return registers[r]; }
    bool read(uint32_t p, void* out, uint32_t n) {
        if (uint64_t(p) + n > memory.size()) return false;
        std::memcpy(out, memory.data() + p, n); return true;
    }
    bool write(uint32_t p, const void* in, uint32_t n) {
        if (uint64_t(p) + n > memory.size()) return false;
        std::memcpy(memory.data() + p, in, n); return true;
    }
    bool map(uint32_t p, uint32_t n, const void*, int) {
        return uint64_t(p) + n <= memory.size();
    }
    void trap_epilogue(uint32_t eax, uint32_t bytes, uint32_t eip) {
        registers[R_EAX] = eax; registers[R_ESP] += bytes; registers[R_EIP] = eip;
    }
};
}
