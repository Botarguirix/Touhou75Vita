#pragma once
#include <cstdint>
#include <cstdio>
#include <set>
#include <array>
#include <map>
#include <vector>
namespace d2rt { struct Cpu; class PeImage; struct ImportRef; }
enum class StartupServiceResult { Unsupported, Serviced, ContractFailure };
// Bounded Win32 contracts for the observed TH075 startup path.
class StartupServices {
public:
    StartupServices(d2rt::Cpu& cpu, const d2rt::PeImage& image, FILE* log)
        : cpu_(cpu), image_(image), log_(log) {}
    StartupServiceResult call(const d2rt::ImportRef& import);
    unsigned version_calls() const { return version_calls_; }
    unsigned module_calls() const { return module_calls_; }
    unsigned heap_create_calls() const { return heap_create_calls_; }
    unsigned heap_alloc_calls() const { return heap_alloc_calls_; }
    unsigned proc_address_calls() const { return proc_address_calls_; }
    unsigned critical_init_calls() const { return critical_init_calls_; }
    unsigned tls_calls() const { return tls_calls_; }
    unsigned unavailable_export_calls() const { return unavailable_export_calls_; }
    unsigned process_calls() const { return process_calls_; }
    unsigned environment_calls() const { return environment_calls_; }
    unsigned conversion_calls() const { return conversion_calls_; }
    unsigned sync_calls() const { return sync_calls_; }
    unsigned code_page_calls() const { return code_page_calls_; }
    unsigned string_type_calls() const { return string_type_calls_; }
    static constexpr uint32_t critical_init_trap = 0x00BFFFE0;
    bool heap_ready() const { return heap_ready_; }
    bool version_globals_match();
private:
    d2rt::Cpu& cpu_;
    const d2rt::PeImage& image_;
    FILE* log_;
    unsigned version_calls_ = 0, module_calls_ = 0;
    unsigned heap_create_calls_ = 0, heap_alloc_calls_ = 0;
    unsigned proc_address_calls_ = 0, critical_init_calls_ = 0;
    std::set<uint32_t> critical_sections_;
    std::array<bool,64> tls_allocated_{};
    unsigned tls_calls_ = 0, unavailable_export_calls_ = 0;
    unsigned process_calls_ = 0;
    unsigned environment_calls_ = 0;
    unsigned conversion_calls_ = 0;
    unsigned sync_calls_ = 0;
    unsigned code_page_calls_ = 0;
    unsigned string_type_calls_ = 0;
    std::map<uint32_t, bool> environment_blocks_; // address -> Unicode variant
    std::vector<uint32_t> environment_free_blocks_;
    bool heap_ready_ = false;
    uint32_t heap_next_ = 0x00C00000;
};
