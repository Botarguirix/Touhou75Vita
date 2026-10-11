#pragma once
#include <psp2/ctrl.h>
#include <cstdint>

namespace th075 {
// Peek is nonblocking. Timestamped history retains a short action-button tap
// between slow guest polls without advancing guest frames or adding a thread.
// Movement remains the latest held state. Multiple taps per poll coalesce.
class PadHistory {
public:
    static constexpr unsigned capacity=64;
    static constexpr uint32_t actions=SCE_CTRL_CROSS|SCE_CTRL_CIRCLE|
        SCE_CTRL_SQUARE|SCE_CTRL_TRIANGLE|SCE_CTRL_START|SCE_CTRL_SELECT|
        SCE_CTRL_LTRIGGER|SCE_CTRL_RTRIGGER;
    struct Result { SceCtrlData latest{};uint32_t buttons=0,short_presses=0;unsigned fresh=0; };
    Result observe(SceCtrlData* samples,unsigned count) {
        if(!samples || !count || count>capacity)return {};
        // SDK history order is normalized without a temporary heap allocation.
        for(unsigned i=1;i<count;++i) {
            const auto value=samples[i];unsigned j=i;
            while(j && samples[j-1].timeStamp>value.timeStamp){samples[j]=samples[j-1];--j;}
            samples[j]=value;
        }
        Result r;r.latest=samples[count-1];r.buttons=r.latest.buttons;
        if(!initialized_ || !r.latest.timeStamp) {
            initialized_=true;last_stamp_=r.latest.timeStamp;previous_=r.latest.buttons;
            return r; // Do not replay buttons pressed before Acquire.
        }
        uint32_t pressed=0;
        for(unsigned i=0;i<count;++i) {
            const auto& sample=samples[i];
            if(!sample.timeStamp || sample.timeStamp<=last_stamp_)continue;
            pressed|=(sample.buttons&~previous_)&actions;
            previous_=sample.buttons;last_stamp_=sample.timeStamp;++r.fresh;
        }
        r.short_presses=pressed&~r.latest.buttons;
        r.buttons|=r.short_presses;
        return r;
    }
    void reset(){initialized_=false;last_stamp_=0;previous_=0;}
private:
    bool initialized_=false;uint64_t last_stamp_=0;uint32_t previous_=0;
};
}
