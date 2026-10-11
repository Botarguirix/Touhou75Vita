#pragma once
#include <psp2/power.h>
#include <cstdio>

// Raise only a lower clock. Preserve a user's higher plugin setting and
// restore only values still equal to our request when the probe ends.
class PerformanceClocks {
public:
    explicit PerformanceClocks(FILE* log):log_(log) {
        arm_=scePowerGetArmClockFrequency();bus_=scePowerGetBusClockFrequency();
        const int a=arm_>0 && arm_<444?scePowerSetArmClockFrequency(444):0;
        const int b=bus_>0 && bus_<222?scePowerSetBusClockFrequency(222):0;
        changed_arm_=arm_>0 && arm_<444 && a>=0;
        changed_bus_=bus_>0 && bus_<222 && b>=0;
        fprintf(log_,"startup_performance_clocks=before_arm:%d before_bus:%d arm_rc:0x%08X bus_rc:0x%08X actual_arm:%d actual_bus:%d gpu:%d xbar:%d gpu_change:none\n",
            arm_,bus_,unsigned(a),unsigned(b),scePowerGetArmClockFrequency(),scePowerGetBusClockFrequency(),
            scePowerGetGpuClockFrequency(),scePowerGetGpuXbarClockFrequency());
    }
    ~PerformanceClocks() {
        if(changed_arm_ && scePowerGetArmClockFrequency()==444)scePowerSetArmClockFrequency(arm_);
        if(changed_bus_ && scePowerGetBusClockFrequency()==222)scePowerSetBusClockFrequency(bus_);
        fprintf(log_,"startup_performance_clocks_restore=arm:%d bus:%d\n",scePowerGetArmClockFrequency(),scePowerGetBusClockFrequency());
    }
private:
    FILE* log_;int arm_=0,bus_=0;bool changed_arm_=false,changed_bus_=false;
};
