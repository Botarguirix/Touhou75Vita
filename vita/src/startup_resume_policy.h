#pragma once
#include <cstdint>
#include <cstddef>
// Only a registered, exact trap-slot entry can be recharged. Caller must also
// validate mapping, main-thread TIB, bounded stack and executable return address.
inline bool startup_known_resume_trap(uint32_t ip,uint32_t base,size_t count,
                                      uint32_t critical,uint32_t feature,bool owned_com) {
    if(ip & 15u)return false;
    return (ip>=base && uint64_t(ip)-base<uint64_t(count)*16u) ||
        ip==critical || ip==feature || owned_com;
}
