#include "../vita/src/pad_history.h"
#include <array>
#include <cassert>
#include <cstdio>

SceCtrlData sample(uint64_t time,uint32_t buttons=0){SceCtrlData s{};s.timeStamp=time;s.buttons=buttons;return s;}
int main() {
    th075::PadHistory history;
    std::array<SceCtrlData,3> boot={sample(1,SCE_CTRL_CROSS),sample(2),sample(3)};
    auto r=history.observe(boot.data(),boot.size());assert(!r.buttons && !r.short_presses);
    puts("PASS initial controller history does not replay a pre-Acquire action");
    std::array<SceCtrlData,3> tap={sample(4),sample(5,SCE_CTRL_CROSS),sample(6)};
    r=history.observe(tap.data(),tap.size());assert(r.buttons==SCE_CTRL_CROSS && r.short_presses==SCE_CTRL_CROSS && r.fresh==3);
    r=history.observe(tap.data(),tap.size());assert(!r.buttons && !r.short_presses && !r.fresh);
    puts("PASS brief action press released between polls delivered once without stale replay");
    std::array<SceCtrlData,4> reverse={sample(9),sample(8,SCE_CTRL_CIRCLE|SCE_CTRL_DOWN),sample(8,SCE_CTRL_CIRCLE|SCE_CTRL_DOWN),sample(7)};
    r=history.observe(reverse.data(),reverse.size());assert(r.buttons==SCE_CTRL_CIRCLE && r.fresh==3);
    assert(r.latest.timeStamp==9 && !(r.buttons&SCE_CTRL_DOWN));
    puts("PASS reversed history and duplicate timestamps normalized while movement remains latest");
    auto held=sample(10,SCE_CTRL_CROSS|SCE_CTRL_UP);held.lx=193;
    r=history.observe(&held,1);assert(r.buttons==held.buttons && r.latest.lx==193 && !r.short_presses);
    r=history.observe(&held,1);assert(r.buttons==held.buttons && !r.fresh);
    auto released=sample(11);r=history.observe(&released,1);assert(!r.buttons);
    puts("PASS held actions analog state and real release preserved across overlapping polls");
    assert(!history.observe(nullptr,1).buttons && !history.observe(&held,0).buttons && !history.observe(&held,65).buttons);
    std::array<SceCtrlData,3> simultaneous={sample(12),sample(13,th075::PadHistory::actions),sample(14)};
    r=history.observe(simultaneous.data(),3);assert(r.buttons==th075::PadHistory::actions);
    history.reset();r=history.observe(simultaneous.data(),3);assert(!r.buttons);
    auto no_time=sample(0,SCE_CTRL_CROSS);assert(history.observe(&no_time,1).buttons==SCE_CTRL_CROSS);
    puts("PASS invalid spans reset and timestamp-free fallback preserve bounds and latest state");
    std::array<SceCtrlData,64> maximum{};
    for(unsigned i=0;i<maximum.size();++i)maximum[i]=sample(100+63-i,i==31?SCE_CTRL_TRIANGLE:0);
    r=history.observe(maximum.data(),maximum.size());assert(r.fresh==64 && r.buttons==SCE_CTRL_TRIANGLE && r.latest.timeStamp==163);
    r=history.observe(maximum.data(),maximum.size());assert(!r.buttons && !r.fresh);
    puts("PASS maximum 64-sample ring preserves bounded single-poll action delivery");
}
