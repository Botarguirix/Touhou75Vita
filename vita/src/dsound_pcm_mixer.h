#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace dsound_pcm {
constexpr unsigned output_hz=48000, block_frames=1024, capacity=128;
struct Buffer {
    uint32_t refs=0,flags=0,hz=0,original_hz=0,channels=0,bits=0,align=0,position=0;
    int32_t volume=0,pan=0;
    std::vector<uint8_t> bytes;
    bool playing=false,looping=false;
    uint64_t phase=0,start_phase=0,epoch=0; // source-frame units / 48000
    uint64_t played_phase=0; // Monotonic within a playback/seek epoch, before ring modulo.
};
struct Pending {uint64_t phase=0,epoch=0;bool active=false;};
using Buffers=std::array<Buffer,capacity>;
using Block=std::array<int16_t,block_frames*2>;
using Commit=std::array<Pending,capacity>;

inline uint32_t gain(int32_t db) {
    return db<=-10000?0u:uint32_t(std::llround(std::pow(10.0,double(db)/2000.0)*65536.0));
}
inline int sample(const Buffer& b,uint64_t frame,unsigned channel) {
    const uint64_t frames=b.bytes.size()/b.align;
    if(!b.looping && frame>=frames)return 0;
    frame%=frames;
    const size_t offset=size_t(frame)*b.align+(b.channels==1?0:channel)*(b.bits/8);
    if(b.bits==8)return (int(b.bytes[offset])-128)*256;
    const unsigned word=unsigned(b.bytes[offset])|(unsigned(b.bytes[offset+1])<<8);
    return word<32768?int(word):int(word)-65536;
}
// Prepare from owned samples without changing a live cursor. The caller
// commits only after the output API accepts the block successfully.
inline unsigned prepare(const Buffers& buffers,Block& block,Commit& pending) {
    std::array<int64_t,block_frames*2> mixed{};pending={};unsigned active=0;
    for(unsigned k=0;k<buffers.size();++k) {
        const auto& b=buffers[k];if(!b.refs || !b.playing)continue;
        ++active;uint64_t phase=b.phase;
        const uint32_t left=gain(b.volume-std::max(b.pan,0));
        const uint32_t right=gain(b.volume+std::min(b.pan,0));
        for(unsigned i=0;i<block_frames;++i,phase+=b.hz) {
            const uint64_t frame=phase/output_hz;const unsigned fraction=phase%output_hz;
            for(unsigned c=0;c<2;++c) {
                const int64_t a=sample(b,frame,c),next=sample(b,frame+1,c);
                const int64_t value=(a*(output_hz-fraction)+next*fraction)/output_hz;
                mixed[2*i+c]+=value*(c?right:left);
            }
        }
        pending[k]={phase,b.epoch,true};
    }
    for(unsigned i=0;i<block.size();++i)
        block[i]=int16_t(std::clamp<int64_t>(mixed[i]/65536,-32768,32767));
    return active;
}
inline void commit(Buffers& buffers,const Commit& pending) {
    for(unsigned i=0;i<buffers.size();++i) {
        auto& b=buffers[i];const auto& p=pending[i];
        if(p.active && b.refs && b.playing && b.epoch==p.epoch)b.phase=p.phase;
    }
}
// Native queued output frames are subtracted from submitted source progress.
// This is a block-resolution estimate, not a hardware sample-clock readback.
inline void positions(Buffers& buffers,unsigned queued_frames) {
    for(auto& b:buffers)if(b.refs && b.playing) {
        const uint64_t lead=uint64_t(queued_frames)*b.hz;
        // Native queue occupancy can grow before Output's accepted phase is
        // committed. Keep the last estimate through that gap: physical81
        // otherwise reported a backward cursor (11052 -> 7524 bytes).
        const uint64_t played=std::max(b.played_phase,
            std::max(b.start_phase,b.phase>lead?b.phase-lead:0));
        b.played_phase=played;
        const uint64_t frames=b.bytes.size()/b.align;
        if(!b.looping && played>=frames*output_hz) {
            b.playing=false;b.phase=b.start_phase=b.played_phase=0;b.position=0;++b.epoch;
        }else b.position=uint32_t((played/output_hz)%frames)*b.align;
    }
}
inline uint32_t write_cursor(const Buffer& b,unsigned queued_frames) {
    if(!b.playing)return b.position;
    // Protect queued audio, a block copied before Output accepts it, and one
    // interpolation lookahead frame. This deliberately conservative lead
    // also covers the brief native-accept/phase-commit gap.
    const uint64_t lead=((uint64_t(queued_frames)+block_frames)*b.hz+output_hz-1)/output_hz+1;
    return uint32_t((uint64_t(b.position)+lead*b.align)%b.bytes.size());
}
inline void seek(Buffer& b,uint32_t byte_offset) {
    b.position=byte_offset;b.phase=b.start_phase=b.played_phase=uint64_t(byte_offset/b.align)*output_hz;++b.epoch;
}
inline void stop(Buffer& b) {b.playing=false;seek(b,b.position);}
}
