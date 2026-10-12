#pragma once
#include "runtime/cpu.h"
#include <array>
#include <cstdint>

// PE32 TIME_ZONE_INFORMATION, explicitly serialized rather than a host struct.
// Microsoft: UTC = local + Bias. An offset snapshot supplies no annual DST
// rules; zero transition months with TIME_ZONE_ID_UNKNOWN is a fixed zone.
namespace startup_time_zone {
constexpr uint32_t size=172, unknown=0, invalid=0xFFFFFFFFu, clock_error=31;
constexpr uint32_t bias_offset=0, standard_name=4, standard_date=68,
    standard_bias=84, daylight_name=88, daylight_date=152, daylight_bias=168;
static_assert(daylight_bias+4==size,"PE32 timezone layout");
struct Snapshot { uint64_t utc=0,local=0;uint32_t resolution=0;int32_t rc=0; };
enum class Status { Success, RtcFailure, InvalidOffset, OutputFault };
struct Reply { Status status=Status::OutputFault;uint32_t value=invalid;int32_t bias=0,rtc_rc=0; };

inline bool encode(const Snapshot& s,std::array<uint8_t,size>& bytes,int32_t& bias) {
    bytes.fill(0);bias=0;
    if(s.rc<0 || !s.resolution)return false;
    const uint64_t minute=uint64_t(s.resolution)*60;
    const bool east=s.local>=s.utc;
    const uint64_t delta=east?s.local-s.utc:s.utc-s.local;
    // Bounded current-offset profile, not a guessed geographic timezone.
    // Subtract unsigned ticks before narrowing; no signed overflow at extremes.
    if(delta>minute*24*60 || delta%minute)return false;
    const auto minutes=int32_t(delta/minute);
    bias=east?-minutes:minutes;
    const auto word=uint32_t(bias);
    for(unsigned i=0;i<4;++i)bytes[i]=uint8_t(word>>(8*i));
    // Names may be empty. Both dates/months and biases remain zero: no DST.
    return true;
}

// Called after StartupServices validates the one-argument stdcall frame.
// The shared implementation includes the actual CPU epilogue, so host checks
// can exercise the same output/return behavior that the Vita service uses.
template<class Provider,class SetError>
Reply invoke(d2rt::Cpu& cpu,uint32_t output,uint32_t ret,Provider provider,SetError set_error) {
    Reply reply;std::array<uint8_t,size> before{},bytes{},after{};
    if(output<0x10000 || uint64_t(output)+size>0x02000000 ||
       !cpu.read(output,before.data(),size))return reply;
    const auto snapshot=provider();reply.rtc_rc=snapshot.rc;
    if(snapshot.rc<0 || !encode(snapshot,bytes,reply.bias)) {
        reply.status=snapshot.rc<0?Status::RtcFailure:Status::InvalidOffset;
        if(!set_error(clock_error) || !cpu.read(output,after.data(),size) || after!=before) {
            reply.status=Status::OutputFault;return reply;
        }
        // RTC failure is a real Win32 API failure, with unchanged output.
        cpu.trap_epilogue(invalid,8,ret);return reply;
    }
    if(!cpu.write(output,bytes.data(),size) || !cpu.read(output,after.data(),size) || after!=bytes)
        return reply;
    reply.status=Status::Success;reply.value=unknown;
    cpu.trap_epilogue(unknown,8,ret);return reply;
}
}
