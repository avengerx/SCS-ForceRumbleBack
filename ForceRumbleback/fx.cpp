#include "pch.h"
#include "fx.h"

namespace fx {
#define EngineIdleSpeed 650
#define EngineIdleHalfSpeed EngineIdleSpeed / 2.0
#define EngineIdleForce 900
#define EngineIdleHalfSpeedForce 4000

#define EngineCruiseSpeed 1400
#define EngineCruiseForce 9400

#define line_slope(x1, x2, y1, y2) ((y2) - (y1)) / ((x2) - (x1))
#define line_eq(x1, x2, y1, y2, x) (((x) - (x1)) * (line_slope(x1, x2, y1, y2)) + (y1))

    long RevToForce(float maxrev, float currev) {
        // maxrev - FFNOMINALMAX
        // currev - x   => x = currev * FFNOM / maxrev
        return (long)((currev * DI_FFNOMINALMAX) / maxrev);
    }

    long RevToForce(truck_info_t state) {
        if (state.rpm < 50) return 0;

        if (state.rpm <= EngineIdleHalfSpeed)
            //   50 - 0      => m = (EIHSF - 0) / (EIHS - 50) = EIHSF / EIHS - 50 = ((EIHS[/F] = 325/10000))
            // EIHS - EIHSF       = 10000 / (325 - 50) = 10000 / 275 = 36.36363636
            // y = mx+c = 36.36x+c =>
            return (long)(line_eq(50, EngineIdleHalfSpeed, 0, EngineIdleHalfSpeedForce, state.rpm));

        if (state.rpm <= EngineIdleSpeed)
            // EIHS - EIHSF => m = (EIF - EIHSF) / (EIS - EIHS) = 650 - 10000 / (650 - 325) =
            //  EIS - EIF        = -9350 / 325 = -28.7692308
            //
            return (long)(line_eq(EngineIdleHalfSpeed, EngineIdleSpeed, EngineIdleHalfSpeedForce, EngineIdleForce, state.rpm));
        if (state.rpm <= EngineIdleSpeed + 200)
            return (long)(line_eq(EngineIdleSpeed, EngineIdleSpeed + 200, EngineIdleForce, 600, state.rpm));

        float half85prpm = (EngineIdleSpeed + 200) + (((0.85f * state.rpm_max) - (EngineIdleSpeed + 200)) / 2);
        // Between idle speed and about half of 85%maxrpm, ramp down slightly, very smooth from 600 to 400 force
        if (state.rpm <= half85prpm)
            return (long)(line_eq(EngineIdleSpeed + 200, half85prpm, 600, 350, state.rpm));

        // Between about half of 85%maxrpm, until 85% max rpm ramp up from 400 to 2500 force (25% max force)
        if (state.rpm <= (0.85 * state.rpm_max))
            return (long)(line_eq(half85prpm, 0.85 * state.rpm_max, 350, 1000, state.rpm));

        // Between 85% max rpm and max rpm, ramp real fast to 100% force
        return (long)(line_eq(0.85 * state.rpm_max, state.rpm_max, 1000, 10000, state.rpm));
    }

    // Gets engine drag force based on current and expected RPM given current throttle position
    long EngineDrag(truck_info_t state, long ref_force) {
        // Engine rpm follows throttle as: idleRpm - 0% throttle; maxRpm - 50% throttle
        // pull will be the difference between non-load rpm at current throttle depression vs
        // actual rpm with that throttle position
        // drag will be majored if engine brake is engaged
        float expected_rpm;;

        if (state.engbrake && state.throttle == 0.0f)
            expected_rpm = EngineIdleSpeed;
        else {
            expected_rpm = line_eq(0.0f, 0.5f, EngineIdleSpeed, state.rpm_max, state.throttle);
            if (expected_rpm < EngineIdleSpeed || expected_rpm > (2 * state.rpm_max)) {
                log("Weird expected RPM: throttle: %.2f, curr rpm: %.2f, expected rpm: %.2f",
                    state.throttle, state.rpm, expected_rpm);
            }
            expected_rpm = max(EngineIdleSpeed, min(expected_rpm, state.rpm_max));
        }

        float deviation = expected_rpm - state.rpm;

        if (deviation < 0) deviation *= state.engbrake ? -1.0f : -0.1f;

        if (deviation < 20.0f) return ref_force;

        //log("Throttle deviation: %.2frpm (current:%.2frpm, expected:%.2frpm, tps:%.2f).", deviation, state.rpm, expected_rpm, state.throttle);
        return ref_force + (long)(line_eq(0, state.rpm_max / 2, 0, 2000, deviation));
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