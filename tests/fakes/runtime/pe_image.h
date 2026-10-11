#pragma once
#include <cstdint>
#include <string>
namespace d2rt {
struct ImportRef {
    std::string dll, name;
    uint32_t ordinal = 0, iat_va = 0, iat_rva = 0;
};
class PeImage {
public:
    uint32_t load_base() const { return 0x00400000; }
    uint32_t image_size() const { return 0x00300000; }
};
}
