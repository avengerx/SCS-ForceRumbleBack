#include "pch.h"
#include "fx.h"

namespace fx {
#define EngineIdleSpeed 650
#define EngineIdleForce 900

#define EngineCruiseSpeed 1400
#define EngineCruiseForce 9400

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
        idleratio = EngineIdleSpeed / state.rpm_max; // this could be saved in truck data and updated on change

        if (rpmratio <= idleratio) {
            // ramp from 2500 to 900
            return (long)((-5517.24138 * rpmratio) + 2555.172413793);
        } else {
            // ramp from 900 to 10k
            return (long)((13000 * rpmratio) - 3000);
        }
    }

    void StartEngine() {
        SendForceWait(5500, 50);
        SendForceWait(8500, 50);
        SendForceWait(10000, 200);
        SendForceWait(10000, 200);
        SendForceWait(4500, 100);
        SendForceWait(2500, 200);
        SendForce(EngineIdleForce);
    }

    void StopEngine() {
        SendForceWait(2500, 100);
        SendForceWait(6500, 100);
        SendForceWait(10000, 300);
        SendForceWait(8500, 300);
        SendForceWait(6500, 300);
        SendForceWait(2500, 300);
        SendForceWait(950, 600);
        SendForceWait(850, 600);
        SendForceWait(400, 100);
        SendForce(0);
    }
}