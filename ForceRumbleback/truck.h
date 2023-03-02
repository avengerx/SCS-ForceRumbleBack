#ifndef __TRUCK_H_INCLUDED__
#define __TRUCK_H_INCLUDED__
#include <mutex>

extern std::mutex truck_data_access;

// The game should keep updating this structure.
// We may create a separate thread loop to poll this and, when
// it detect changes, apply the effects accordingly.
struct truck_info_t {
    bool paused; // if the game is paused, in menu, etc, should stop all rumbling.
    bool revving; // if engine is on (enabled?)
    bool engbrake; // if engine brake is engaged
    int gear; // usually -4..-1, 0 (N), 1 .. 18, depending on gearbox.
    float rpm_idle; // this should be about 700-1000
    float rpm_torque; // probably about 1600-1800?
    float rpm_power; // about high, 2800-3000
    float rpm_max; // about high, 2800-3000
    float speed; // in combination with gear, could help interpret throttle if not available
    float rpm; // should this really be a float?
    float throttle; // how much is the throttle pedal depressed
};

extern truck_info_t truck_data;

HRESULT InitTruckData();

#endif