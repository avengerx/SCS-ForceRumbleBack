#ifndef __FX_H_INCLUDED__
#define __FX_H_INCLUDED__
#include "pch.h"
#include "truck.h"
#include "rumble.h"

namespace fx {
    long RevToForce(float, float);
    long RevToForce(truck_info_t);
    long EngineDrag(truck_info_t, long);

    void StartEngine();
    void StopEngine();
}

#endif