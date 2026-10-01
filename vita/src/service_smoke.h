#pragma once
#include "runtime/guest_region.h"
#include <cstdio>
#include <cstdint>
#include <vector>
namespace d2rt { class Cpu; class Bridge; class PeImage; }
// Owned only for the diagnostic session; not a Windows process environment.
class ServiceSmoke {
public:
    ServiceSmoke(d2rt::Cpu& cpu, FILE* log);
    ~ServiceSmoke();
    void install(d2rt::Bridge& bridge);
    bool run(d2rt::Bridge& bridge, const d2rt::PeImage& image,
             const std::vector<uint8_t>& exe, bool& unsupported);
private:
    d2rt::Cpu& cpu_;
    FILE* log_;
    void set_error(uint32_t value);
    uint32_t last_error() const;
    wx86::GuestRegion heap_;
    FILE* file_=nullptr;
    unsigned calls_=0;
};
