#include "../vita/src/seh_chain.h"
#include <cassert>
#include <map>

int main() {
    std::map<uint32_t, uint32_t> memory;
    auto read = [&](uint32_t p, uint32_t& out) {
        auto it = memory.find(p);
        if (it == memory.end()) return false;
        out = it->second; return true;
    };
    unsigned depth = 0;
    auto valid = [&](uint32_t head) {
        return validate_seh_chain(head, 0x800000, 0xA00000,
            0x400000, 0x700000, read, depth);
    };
    memory = {{0x9FEFE8, 0xFFFFFFFF}, {0x9FEFEC, 0x645468}};
    assert(valid(0x9FEFE8) && depth == 1);
    memory[0x9FEEB8] = 0x9FEFE8; memory[0x9FEEBC] = 0x645468;
    assert(valid(0x9FEEB8) && depth == 2); // actual iteration16 layout
    memory[0x9FEFE8] = 0x9FEEB8;
    assert(!valid(0x9FEEB8)); // cyclic chain
    memory[0x9FEFE8] = 0xFFFFFFFF;
    memory[0x9FEEBC] = 0x3FE0;
    assert(!valid(0x9FEEB8)); // bad handler
    assert(!valid(0x7FFFFC)); // outside stack
    assert(!valid(0x9FFFFC)); // truncated record
    assert(!valid(0x9FEEB9)); // unaligned record
    memory.erase(0x9FEEBC);
    assert(!valid(0x9FEEB8)); // unreadable record
}
