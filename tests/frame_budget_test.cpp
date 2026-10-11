#include "../vita/src/frame_budget.h"
#include <cassert>
#include <cstring>
#include <limits>

using th075::FrameBudget;
int main() {
    FrameBudget budget;auto r=budget.report();
    assert(!r.frames && !r.total_us && !r.p95_us && !r.missed);
    puts("PASS empty frame budget reports no invented FPS or quantiles");
    for(uint64_t us:{1u,16666u,16667u,16668u,118517u})assert(budget.observe(us,1));
    r=budget.report();assert(r.frames==5 && r.within_budget==3 && r.missed==2);
    assert(r.min_us==1 && r.max_us==118517 && r.p50_us==16667 && r.p95_us==118517);
    puts("PASS exact 60 FPS deadline boundary and nearest-rank quantiles");
    assert(!budget.observe(500,0) && !budget.observe(500,2));
    assert(!budget.observe(0,1) && !budget.observe(std::numeric_limits<uint64_t>::max(),1));
    r=budget.report();assert(r.frames==5 && r.no_present==1 && r.multi_present==1 && r.invalid==2);
    puts("PASS no-Present multi-Present and invalid cycles excluded without altering samples");
    FrameBudget full;
    for(unsigned i=0;i<FrameBudget::capacity;++i)assert(full.observe(210000000u,1));
    assert(!full.observe(1,1));r=full.report();
    assert(r.frames==512 && r.dropped==1 && r.total_us==uint64_t(512)*210000000u && r.within_budget==0);
    puts("PASS bounded storage and 64-bit duration accumulation without allocation");
    FrameBudget ranks;for(unsigned i=100;i;--i)assert(ranks.observe(i,1));
    r=ranks.report();assert(r.p50_us==50 && r.p95_us==95 && r.p99_us==99);
    assert(ranks.report().total_us==5050 && ranks.observe(101,1));
    puts("PASS sorting snapshots leaves future observations intact");
    FILE* file=std::tmpfile();assert(file);budget.log(file);std::rewind(file);
    char line[1024];assert(std::fgets(line,sizeof(line),file));std::fclose(file);
    assert(std::strstr(line,"startup_60fps_budget=target_fps:60 budget_us:16667 frames:5 within:3 missed:2"));
    assert(std::strstr(line,"scope:timer_and_main_one_confirmed_present"));
    puts("PASS telemetry logs explicit scope and real misses without changing pacing");
}
