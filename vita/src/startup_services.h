#pragma once
#include <cstdint>
#include <cstdio>
namespace d2rt { struct Cpu; class PeImage; struct ImportRef; }
enum class StartupServiceResult { Unsupported, Serviced, ContractFailure };
// Narrow contracts for the two imports observed at the beginning of TH075.
class StartupServices {
public:
    StartupServices(d2rt::Cpu& cpu, const d2rt::PeImage& image, FILE* log)
        : cpu_(cpu), image_(image), log_(log) {}
    StartupServiceResult call(const d2rt::ImportRef& import);
    unsigned version_calls() const { return version_calls_; }
    unsigned module_calls() const { return module_calls_; }
    unsigned heap_create_calls() const { return heap_create_calls_; }
    unsigned heap_alloc_calls() const { return heap_alloc_calls_; }
    bool heap_ready() const { return heap_ready_; }
    bool version_globals_match();
private:
    d2rt::Cpu& cpu_;
    const d2rt::PeImage& image_;
    FILE* log_;
    unsigned version_calls_ = 0, module_calls_ = 0;
    unsigned heap_create_calls_ = 0, heap_alloc_calls_ = 0;
    bool heap_ready_ = false;
    uint32_t heap_next_ = 0x00C00000;
};
