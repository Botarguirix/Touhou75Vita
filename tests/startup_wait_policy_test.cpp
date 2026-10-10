#include "../vita/src/startup_wait_policy.h"
#include "../vita/src/startup_limits.h"
#include <cassert>
#include <cstdio>
#include <limits>

using namespace startup_wait;
int main() {
    Event automatic{false,false};
    assert(!automatic.acquire() && automatic.generation==0);
    automatic.set();automatic.set();
    assert(automatic.generation==2 && automatic.acquire());
    assert(!automatic.acquire() && !automatic.signaled && automatic.generation==2);
    puts("PASS auto-reset consumes one real signal; repeated SetEvent does not queue permits");

    Event manual{true,false};manual.set();
    assert(manual.acquire() && manual.acquire() && manual.signaled);
    manual.signaled=false;assert(!manual.acquire());
    puts("PASS manual-reset remains signaled until reset");

    assert(!timeout_elapsed(1000,16,16999));
    assert(timeout_elapsed(1000,16,17000));
    assert(!timeout_elapsed(1000,infinite,1000000000));
    assert(!timeout_elapsed(1000,16,999));
    const uint64_t end=std::numeric_limits<uint64_t>::max();
    assert(!timeout_elapsed(end-100,16,end));
    assert(timeout_elapsed(end-17000,16,end));
    puts("PASS real deadline, INFINITE, clock rollback and overflow-safe elapsed calculation");

    assert(audio_trap_due(true,true,true,false,0x00730000,1000,80,81000));
    assert(!audio_trap_due(true,true,true,false,0x00730000,1000,80,80999));
    assert(!audio_trap_due(false,true,true,false,0x00730000,1000,80,81000));
    assert(!audio_trap_due(true,false,true,false,0x00730000,1000,80,81000));
    assert(!audio_trap_due(true,true,false,false,0x00730000,1000,80,81000));
    assert(!audio_trap_due(true,true,true,true,0x00730000,1000,80,81000));
    assert(!audio_trap_due(true,true,true,false,0x00770000,1000,80,81000));
    assert(!audio_trap_due(true,true,true,false,0x00730000,1000,infinite,81000));
    assert(!audio_trap_due(true,true,true,false,0x00730000,1000,16,81000));
    assert(!audio_trap_due(true,true,true,false,0x00730000,1000,80,999));
    uint32_t section[6]={0,0xFFFFFFFF,0,0,0,0};
    assert(critical_dispatch_safe(section));
    section[1]=0;section[2]=1;section[3]=4;assert(!critical_dispatch_safe(section));
    section[1]=0xFFFFFFFF;section[2]=0;assert(!critical_dispatch_safe(section));
    section[3]=0;section[1]=0;assert(!critical_dispatch_safe(section));
    puts("PASS audio dispatch requires real deadline main context active playback and unlocked critical sections");

    uint32_t wait[3]={0x00603545,0x00AB2004,infinite};
    assert(observed_frame_wait(0x00730000,0x009FEC48,wait));
    assert(!observed_frame_wait(0x00760000,0x009FEC48,wait));
    assert(!observed_frame_wait(0x00730000,0x009FFFF8,wait));
    for(unsigned i=0;i<3;++i) {
        const auto original=wait[i];wait[i]^=4;
        assert(!observed_frame_wait(0x00730000,0x009FEC48,wait));wait[i]=original;
    }
    puts("PASS observed main wait rejects foreign TIB, invalid stack and unrelated frames");

    d2rt::X86Context main{},worker{};
    main.eip=0x00B003C0;main.gpr[4]=0x009FEC48;main.fs_base=0x00730000;
    main.fpu_valid=true;main.fpu[767]=0xA5;
    worker=main;assert(same_context(main,worker));
    worker.gpr[0]^=1;assert(!same_context(main,worker));worker=main;
    worker.eflags^=1;assert(!same_context(main,worker));worker=main;
    worker.fs_base=0x00760000;assert(!same_context(main,worker));worker=main;
    worker.fpu[767]^=1;assert(!same_context(main,worker));worker=main;
    worker.fpu_valid=false;assert(!same_context(main,worker));
    puts("PASS main context comparison includes flags, FS and complete valid FPU/SIMD state");

    for(unsigned i=0;i<=8;++i)assert(slice_cap(i)==32+4*i);
    assert(slice_cap(100)==64);
    assert(startup_limits::present_frames==240 && startup_limits::frame_wait_resumes==240);
    assert(startup_limits::draw_calls==2048 && startup_limits::draw_pixels==384u*1024u*1024u);
    assert(startup_limits::audio_trap_dispatches==1024);
    assert(startup_limits::time_cap_us<startup_limits::watchdog_timeout_us);
    assert(slice_cap(startup_limits::frame_wait_resumes)==64);
    Event frame{false,false};
    const auto before=frame.generation;
    assert(!frame.signaled && !frame.acquire()); // An infinite main wait cannot finish.
    frame.set(); // Own pattern modeling a serviced original producer's SetEvent.
    assert(frame.generation!=before && frame.acquire() && !frame.signaled);
    assert(!frame.acquire());
    puts("PASS continuation stays bounded and frame wait completes only after a signal");
}
