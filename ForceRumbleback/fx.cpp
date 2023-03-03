#include "pch.h"
#include "fx.h"

namespace fx {
    long RevToForce(float maxrev, float currev) {
        // maxrev - FFNOMINALMAX
        // currev - x   => x = currev * FFNOM / maxrev
        return (long)((currev * DI_FFNOMINALMAX) / maxrev);
    }

    long RevToForce(truck_info_t state) {
        float rpmratio, idleratio;
        if (state.rpm <= FORCEGRANULARITY) {
            return 0;
        }

        rpmratio = state.rpm / state.rpm_max;
        idleratio = 650 / state.rpm_max; // this could be saved in truck data and updated on change

        if (rpmratio <= idleratio) {
            // ramp from 2500 to 900
            return (long)((-5517.24138 * rpmratio) + 2555.172413793);
        } else {
            // ramp from 900 to 10k
            return (long)((13000 * rpmratio) - 3000);
        }
    }

    long EngineIdlespeed = 900;
    long EngineCruisespeed = 9400;

    void StartEngine() {
        SendForceWait(500, 350);
        SendForceWait(2500, 150);
        SendForceWait(2500, 150);
        SendForceWait(4500, 150);
        SendForceWait(6500, 150);
        SendForceWait(10000, 200);
        SendForceWait(8500, 100);
        SendForceWait(6500, 100);
        SendForceWait(2500, 200);
        SendForceWait(EngineIdlespeed, 500);
    }

    void StopEngine() {
        SendForceWait(2500, 100);
        SendForceWait(6500, 100);
        SendForceWait(10000, 200);
        SendForceWait(8500, 100);
        SendForceWait(6500, 100);
        SendForceWait(2500, 200);
        SendForceWait(850, 600);
        SendForceWait(500, 300);
        SendForceWait(0, 100);
    }
}