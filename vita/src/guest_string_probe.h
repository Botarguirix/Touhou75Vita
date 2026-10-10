#pragma once
#include <array>
#include <cstdint>
#include <cstddef>

// Bounded read-only observation. Raw bytes are retained because MessageBoxA
// strings in the pinned Japanese EXE use CP932, not UTF-8.
namespace guest_string_probe {
enum class Status { Null, Terminated, Truncated, OutOfRange, Unreadable };
struct Snapshot {
    std::array<uint8_t,256> bytes{};
    size_t length=0;
    Status status=Status::Null;
};
inline const char* name(Status s) {
    switch(s){case Status::Null:return "null";case Status::Terminated:return "terminated";
    case Status::Truncated:return "truncated";case Status::OutOfRange:return "out_of_range";
    case Status::Unreadable:return "unreadable";}return "invalid";
}
template<class Read> Snapshot observe(uint32_t address,Read read) {
    Snapshot s;if(!address)return s;
    if(address<0x10000 || address>=0x02000000){s.status=Status::OutOfRange;return s;}
    for(size_t i=0;i<s.bytes.size();++i) {
        const uint64_t next=uint64_t(address)+i;
        if(next>=0x02000000){s.status=Status::OutOfRange;return s;}
        uint8_t b=0;
        if(!read(uint32_t(next),b)){s.status=Status::Unreadable;return s;}
        if(!b){s.status=Status::Terminated;return s;}
        s.bytes[s.length++]=b;
    }
    s.status=Status::Truncated;return s;
}
}
