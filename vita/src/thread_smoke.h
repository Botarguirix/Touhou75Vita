#pragma once
#include <array>
#include <cstdint>
#include <cstdio>
namespace d2rt { class Cpu; class Bridge; class PeImage; }
constexpr uint32_t kDiagnosticTeb=0x00730000;
// Single-thread diagnostic environment, not a complete Windows process.
class ThreadSmoke {
public:
    ThreadSmoke(d2rt::Cpu& cpu,FILE* log):cpu_(cpu),log_(log){}
    ~ThreadSmoke();
    bool initialize();
    void install(d2rt::Bridge& bridge);
    bool run(d2rt::Bridge& bridge,const d2rt::PeImage& image,bool& unsupported);
private:
    d2rt::Cpu& cpu_;
    FILE* log_;
    std::array<bool,64> allocated_{};
    bool initialized_=false;
};
