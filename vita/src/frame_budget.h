#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace th075 {
// Observe genuine wait-to-wait cycles with exactly one confirmed Present.
// This is telemetry, not frame pacing: it never advances guest time or input.
class FrameBudget {
public:
    static constexpr unsigned target_fps=60,budget_us=16667,capacity=512;
    struct Report {
        unsigned frames=0,within_budget=0,missed=0,no_present=0,multi_present=0,invalid=0,dropped=0;
        uint64_t total_us=0,min_us=0,max_us=0,p50_us=0,p95_us=0,p99_us=0;
        unsigned all_frames=0,all_within=0;uint64_t all_total_us=0;
    };
    bool observe(uint64_t elapsed_us,unsigned confirmed_presents) {
        if(!confirmed_presents){++no_present_;return false;}
        if(confirmed_presents!=1){++multi_present_;return false;}
        if(!elapsed_us || elapsed_us>330000000u){++invalid_;return false;}
        ++all_frames_;all_total_+=elapsed_us;if(elapsed_us<=budget_us)++all_within_;
        if(count_==capacity){++dropped_;return false;}
        samples_[count_++]=elapsed_us;total_+=elapsed_us;
        if(elapsed_us<=budget_us)++within_;
        return true;
    }
    Report report() const {
        Report r;r.frames=count_;r.within_budget=within_;r.missed=count_-within_;
        r.no_present=no_present_;r.multi_present=multi_present_;r.invalid=invalid_;r.dropped=dropped_;r.total_us=total_;
        r.all_frames=all_frames_;r.all_within=all_within_;r.all_total_us=all_total_;
        if(!count_)return r;
        auto sorted=samples_;std::sort(sorted.begin(),sorted.begin()+count_);
        r.min_us=sorted[0];r.max_us=sorted[count_-1];
        // Nearest-rank quantiles, integer microseconds. No interpolated claim.
        r.p50_us=sorted[(count_*50+99)/100-1];
        r.p95_us=sorted[(count_*95+99)/100-1];
        r.p99_us=sorted[(count_*99+99)/100-1];
        return r;
    }
    void log(FILE* file) const {
        const auto r=report();
        fprintf(file,"startup_60fps_budget=target_fps:%u budget_us:%u frames:%u within:%u missed:%u total_us:%llu min_us:%llu p50_us:%llu p95_us:%llu p99_us:%llu max_us:%llu no_present:%u multi_present:%u invalid:%u dropped:%u scope:timer_and_main_one_confirmed_present\n",
            target_fps,budget_us,r.frames,r.within_budget,r.missed,(unsigned long long)r.total_us,
            (unsigned long long)r.min_us,(unsigned long long)r.p50_us,(unsigned long long)r.p95_us,
            (unsigned long long)r.p99_us,(unsigned long long)r.max_us,r.no_present,r.multi_present,r.invalid,r.dropped);
        fprintf(file,"startup_60fps_all_cycles=frames:%u within:%u missed:%u total_us:%llu quantile_sample:first_%u_valid_cycles scope:timer_and_main_one_confirmed_present\n",r.all_frames,r.all_within,r.all_frames-r.all_within,(unsigned long long)r.all_total_us,capacity);
    }
private:
    std::array<uint64_t,capacity> samples_{};
    unsigned count_=0,within_=0,no_present_=0,multi_present_=0,invalid_=0,dropped_=0;
    uint64_t total_=0;
    unsigned all_frames_=0,all_within_=0;uint64_t all_total_=0;
};
}
