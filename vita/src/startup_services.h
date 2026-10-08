#pragma once
#include <cstdint>
#include <cstdio>
#include <set>
#include <array>
#include <map>
#include <vector>
#include <string>
namespace d2rt { struct Cpu; class PeImage; struct ImportRef; }
enum class StartupServiceResult { Unsupported, Serviced, ContractFailure, Deferred };
struct StartupWorker;
// Bounded Win32 contracts for the observed TH075 startup path.
class StartupServices {
public:
    StartupServices(d2rt::Cpu& cpu, const d2rt::PeImage& image, FILE* log)
        : cpu_(cpu), image_(image), log_(log) {}
    ~StartupServices();
    StartupServiceResult call(const d2rt::ImportRef& import);
    bool window_pending() const { return window_pending_; }
    bool finish_window_creation(); // Call only after Cpu::run has returned.
    void attach_worker(StartupWorker* worker) { worker_ = worker; }
    void attach_audio_worker(StartupWorker* worker) { audio_worker_ = worker; }
    unsigned priority_calls() const { return priority_calls_; }
    unsigned version_calls() const { return version_calls_; }
    unsigned module_calls() const { return module_calls_; }
    unsigned heap_create_calls() const { return heap_create_calls_; }
    unsigned heap_alloc_calls() const { return heap_alloc_calls_; }
    unsigned heap_other_calls() const { return heap_other_calls_; }
    unsigned proc_address_calls() const { return proc_address_calls_; }
    unsigned critical_init_calls() const { return critical_init_calls_; }
    unsigned tls_calls() const { return tls_calls_; }
    unsigned unavailable_export_calls() const { return unavailable_export_calls_; }
    unsigned process_calls() const { return process_calls_; }
    unsigned clock_calls() const { return clock_calls_; }
    unsigned multimedia_calls() const { return multimedia_calls_; }
    unsigned event_calls() const { return event_calls_; }
    unsigned environment_calls() const { return environment_calls_; }
    unsigned conversion_calls() const { return conversion_calls_; }
    unsigned sync_calls() const { return sync_calls_; }
    unsigned code_page_calls() const { return code_page_calls_; }
    unsigned string_type_calls() const { return string_type_calls_; }
    unsigned case_map_calls() const { return case_map_calls_; }
    static constexpr uint32_t critical_init_trap = 0x00BFFFE0;
    static constexpr uint32_t processor_feature_trap = 0x00BFFFD0;
    bool heap_ready() const { return heap_ready_; }
    bool com_ready(uint32_t tib) const {
        const auto it=com_apartments_.find(tib);
        return it!=com_apartments_.end() && it->second>0;
    }
    bool event_unsignaled(uint32_t handle) const {
        const auto it = events_.find(handle);
        return it != events_.end() && !it->second.signaled;
    }
    bool version_globals_match();
private:
    d2rt::Cpu& cpu_;
    const d2rt::PeImage& image_;
    FILE* log_;
    StartupWorker* worker_ = nullptr;
    StartupWorker* audio_worker_ = nullptr;
    unsigned priority_calls_ = 0;
    unsigned version_calls_ = 0, module_calls_ = 0;
    uint32_t d3d8_module_refs_ = 1; // Already linked through the PE IAT backend.
    unsigned heap_create_calls_ = 0, heap_alloc_calls_ = 0;
    unsigned heap_other_calls_ = 0;
    struct HeapBlock { uint32_t size, capacity; };
    std::map<uint32_t, HeapBlock> heap_blocks_;
    unsigned proc_address_calls_ = 0, critical_init_calls_ = 0;
    std::set<uint32_t> critical_sections_;
    std::array<bool,64> tls_allocated_{};
    unsigned tls_calls_ = 0, unavailable_export_calls_ = 0;
    unsigned process_calls_ = 0;
    unsigned clock_calls_ = 0;
    unsigned multimedia_calls_ = 0, timer_period_requests_ = 0;
    struct EventState { bool manual_reset, signaled; };
    std::map<uint32_t, EventState> events_;
    uint32_t next_event_handle_ = 0x00AB2000;
    unsigned event_calls_ = 0;
    unsigned environment_calls_ = 0;
    unsigned conversion_calls_ = 0;
    unsigned sync_calls_ = 0;
    unsigned code_page_calls_ = 0;
    unsigned string_type_calls_ = 0;
    unsigned case_map_calls_ = 0;
    std::map<uint32_t, bool> environment_blocks_; // address -> Unicode variant
    std::vector<uint32_t> environment_free_blocks_;
    bool heap_ready_ = false;
    uint32_t heap_next_ = 0x00C00000;
    std::map<uint32_t,uint32_t> heap_free_ranges_;
    uint32_t reserve_heap(uint32_t capacity);
    void reclaim_heap(uint32_t address,uint32_t capacity);
    uint32_t unhandled_filter_ = 0;
    struct IconObject { uint32_t handle; std::vector<uint32_t> pixels; };
    std::map<std::string,IconObject> icons_;
    std::vector<uint32_t> arrow_cursor_;
    struct WindowClass { uint16_t atom; std::array<uint32_t,12> fields; };
    std::map<std::string,WindowClass> window_classes_;
    struct Window {
        uint32_t handle=0, procedure=0, frame=0, return_address=0;
        std::array<uint32_t,12> args{};
        std::vector<uint32_t> surface;
        bool created=false, visible=false, invalidated=false;
    } window_;
    bool window_pending_=false;
    uint32_t window_callback_message_=0, window_callback_parameter_=0;
    uint32_t window_callback_wparam_=0;
    bool window_show_pending_=false, window_was_visible_=false;
    bool window_paint_pending_=false;
    std::map<uint32_t,unsigned> com_apartments_;
    std::map<uint32_t,FILE*> files_;
    std::set<uint32_t> writable_files_;
    // Stable std::map nodes keep setvbuf storage alive until fclose. At most 32 files.
    std::map<uint32_t,std::array<char,65536>> readonly_file_buffers_;
    uint32_t next_file_handle_=0x00AB8000;
};
