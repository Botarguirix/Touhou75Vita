#pragma once
#include "runtime/guest_thread.h"
#include <cstdint>
#include <cstring>

namespace startup_wait {
constexpr uint32_t infinite=0xFFFFFFFFu;
struct Event {
    bool manual_reset=false, signaled=false;
    uint32_t generation=0; // Records actual SetEvent calls, not queued permits.
    void set() { signaled=true; ++generation; }
    bool acquire() {
        if(!signaled)return false;
        if(!manual_reset)signaled=false;
        return true;
    }
};
inline bool timeout_elapsed(uint64_t started,uint32_t timeout,uint64_t now) {
    return timeout!=infinite && now>=started && now-started>=uint64_t(timeout)*1000;
}
inline bool observed_frame_wait(uint32_t tib,uint32_t esp,const uint32_t (&words)[3]) {
    return tib==0x00730000 && esp>=0x00800000 && uint64_t(esp)+12<=0x00A00000 &&
        words[0]==0x00603545 && words[1]==0x00AB2004 && words[2]==infinite;
}
inline bool same_context(const d2rt::X86Context& a,const d2rt::X86Context& b) {
    return !std::memcmp(a.gpr,b.gpr,sizeof(a.gpr)) && a.eip==b.eip &&
        a.eflags==b.eflags && a.fs_base==b.fs_base && a.fpu_valid==b.fpu_valid &&
        (!a.fpu_valid || !std::memcmp(a.fpu,b.fpu,sizeof(a.fpu)));
}
inline unsigned slice_cap(unsigned completed_frame_waits) {
    // Extra slices are earned only after a genuine frame event was consumed.
    return 32u+4u*(completed_frame_waits<8u ? completed_frame_waits:8u);
}
}
