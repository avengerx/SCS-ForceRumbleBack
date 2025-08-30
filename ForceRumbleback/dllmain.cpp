// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <stdio.h>
#include <share.h>

#include "poller.h"
#include "scsutil.h"
#include "log.h"
#include "rumble.h"
#include "truck.h"

#define UNUSED(x)
#define StdCall __stdcall

#pragma comment( linker, "/export:scs_telemetry_init" )
#pragma comment( linker, "/export:scs_telemetry_shutdown" )


#define MINATSVER SCS_TELEMETRY_ATS_GAME_VERSION_1_05
#define MINETS2VER SCS_TELEMETRY_EUT2_GAME_VERSION_1_18

scs_log_t game_log = nullptr;

SCSAPI_VOID telemetry_pause(const scs_event_t event, const void* const UNUSED(event_info), const scs_context_t UNUSED(context)) {
    truck_data.paused = (event == SCS_TELEMETRY_EVENT_paused);
    if (truck_data.paused) {
        log_message("Realtime data resumed.");
    }
    else {
        log_message("Realtime data interrupted.");
    }
}

SCSAPI_VOID telemetry_configuration(const scs_event_t event, const void* const event_info, const scs_context_t UNUSED(context))
{
    // We currently only care for the truck telemetry info.

    const struct scs_telemetry_configuration_t* const info = static_cast<const scs_telemetry_configuration_t*>(event_info);
#ifdef _DEBUGx
    scs_value_dvector_t dpos;
    scs_value_fvector_t pos;
    scs_value_euler_t orient;
    // This portion of the code will only dump to the log file if it's enabled, not to the game's console or own log file.
    log("Dumping all configuration values received for id: %s", info->id);
    for (const scs_named_value_t* current = info->attributes; current->name; ++current) {
#define logType(replacer, type) log("#%u %s (" #type "): " replacer, current->index, current->name, current->value.value_ ## type.value); break
        switch (current->value.type) {
        case SCS_VALUE_TYPE_bool:
            log("#%u %s (bool): %s", current->index, current->name, current->value.value_bool.value ? "true" : "false");
            break;
        case SCS_VALUE_TYPE_double:
            logType("%.4f", double);
        case SCS_VALUE_TYPE_dplacement:
            // dplace.position{x{dbl}, y{dbl}, z{dbl}}, dplace.orient{heading{f}, pitch{f}, roll{f}}, dplace._padding{int}
            dpos = current->value.value_dplacement.position;
            orient = current->value.value_dplacement.orientation;
            log("#%u %s (dplacement): (%.4f,%.4f,%.4f) [head:%.2f, pitch:%.2f, roll:%.2f] pad:%i",
                current->index, current->name,
                dpos.x, dpos.y, dpos.z, orient.heading, orient.pitch, orient.roll,
                current->value.value_dplacement._padding);
            break;
        case SCS_VALUE_TYPE_dvector:
            dpos = current->value.value_dvector;
            log("#%u %s (dvector): (%.4f,%.4f,%.4f)", current->index, current->name, dpos.x, dpos.y, dpos.z);
            break;
        case SCS_VALUE_TYPE_euler:
            orient = current->value.value_euler;
            log("#%u %s (euler): head:%.2f, pitch:%.2f, roll:%.2f", current->index, current->name, orient.heading, orient.pitch, orient.roll);
            break;
        case SCS_VALUE_TYPE_float:
            logType("%.2f", float);
        case SCS_VALUE_TYPE_fplacement:
            // fplace.position{x{f}, y{f}, z{f}}, fplace.orient{heading{f}, pitch{f}, roll{f}}
            pos = current->value.value_fplacement.position;
            orient = current->value.value_fplacement.orientation;
            log("#%u %s (dplacement): (%.2f,%.2f,%.2f) [head:%.2f, pitch:%.2f, roll:%.2f]", current->index, current->name,
                pos.x, pos.y, pos.z, orient.heading, orient.pitch, orient.roll);
            break;
        case SCS_VALUE_TYPE_fvector:
            pos = current->value.value_fvector;
            log("#%u %s (dvector): (%.2f,%.2f,%.2f)", current->index, current->name, pos.x, pos.y, pos.z);
            break;
        case SCS_VALUE_TYPE_INVALID:
            log("#%u %s (invalid): N/A", current->index, current->name);
            break;
            // case SCS_VALUE_TYPE_LAST: // synonym for s64 below
        case SCS_VALUE_TYPE_s32:
            logType("%i", s32);
        case SCS_VALUE_TYPE_s64:
            logType("%l", s64);
        case SCS_VALUE_TYPE_string:
            logType("%s", string);
        case SCS_VALUE_TYPE_u32:
            logType("%u", u32);
        case SCS_VALUE_TYPE_u64:
            logType("%lu", u64);
        default:
            log("- %s (unknown): N/A", current->name);
        }
    }
#endif
    if (strcmp(info->id, SCS_TELEMETRY_CONFIG_truck) != 0) {
        return;
    }

    const scs_named_value_t* const max_rpm_attr = find_attribute(*info, SCS_TELEMETRY_CONFIG_ATTRIBUTE_rpm_limit, SCS_U32_NIL, SCS_VALUE_TYPE_float);

    float max_rpm = max_rpm_attr ? max_rpm_attr->value.value_float.value : 3000;

    truck_data_access.lock();
    // as an example, International HS 2.8L engine (midsize truck) has specs:
    // max rpm: 4640 (standalone engine, I don't think the game uses this)
    // power rpm: 3800, roughly 0.82x
    // torque rpm: 1400 (8b60, vnt, vgt), 1600 (8b61, wastegate), roughly 0.35x
    // idle rpm: 800 (+/-20), maybe enough as a constant?
    truck_data.rpm_max = max_rpm;
    truck_data.rpm_idle = 650; // TODO: find a better value for idle, or "fix" when we find throttle at zero, speed at zero, and which RPM the trucks sits on
    truck_data.rpm_torque = max_rpm - 1000; // TODO: do some math to infer a good value (game has this info for each engine!)
    truck_data.rpm_power = max_rpm; // the game doesn't really care with engine's rated max rpm
    truck_data_access.unlock();

    log_message("Received new truck configuration: max rpm: %.2f, idle rpm: %.2f, torque rpm (estimated): %.2f, power rpm (estimated): %.2f", truck_data.rpm_max, truck_data.rpm_idle, truck_data.rpm_torque, truck_data.rpm_power);
}

/**
 * @brief Telemetry API initialization function.
 *
 * See scssdk_telemetry.h
 */
SCSAPI_RESULT scs_telemetry_init(const scs_u32_t version, const scs_telemetry_init_params_t* const params)
{
    if (version != SCS_TELEMETRY_VERSION_1_01) return SCS_RESULT_unsupported;

    const scs_telemetry_init_params_v101_t* const version_params = static_cast<const scs_telemetry_init_params_v101_t*>(params);

    const scs_sdk_init_params_v100_t *common = &version_params->common;
    game_log = common->log;

    log_message("Initializing");

    const char* game_name;

    if (strcmp(common->game_id, SCS_GAME_ID_EUT2) == 0) {
        game_name = "Euro Truck Simulator 2";
        const scs_u32_t MINIMAL_VERSION = MINETS2VER;
        if (common->game_version < MINIMAL_VERSION)
            log_warning("WARNING: Too old version of the game, some features might behave incorrectly");

        // Future versions are fine as long the major version is not changed.
        const scs_u32_t IMPLEMENTED_VERSION = SCS_TELEMETRY_EUT2_GAME_VERSION_CURRENT;
        if (SCS_GET_MAJOR_VERSION(common->game_version) > SCS_GET_MAJOR_VERSION(IMPLEMENTED_VERSION))
            log_warning("WARNING: This plugin was implemented for ETS2 version %u.%u. Current version is %u.%u. Some or all features might not work.",
                SCS_GET_MAJOR_VERSION(IMPLEMENTED_VERSION), SCS_GET_MINOR_VERSION(IMPLEMENTED_VERSION),
                SCS_GET_MAJOR_VERSION(common->game_version), SCS_GET_MINOR_VERSION(common->game_version));
    } else if (strcmp(common->game_id, SCS_GAME_ID_ATS) == 0) {
        game_name = "American Truck Simulator";
        const scs_u32_t MINIMAL_VERSION = MINATSVER;
        if (common->game_version < MINIMAL_VERSION)
            log_warning("WARNING: Too old version of the game, some features might behave incorrectly");

        // Future versions are fine as long the major version is not changed.
        const scs_u32_t IMPLEMENTED_VERSION = SCS_TELEMETRY_ATS_GAME_VERSION_CURRENT;
        if (SCS_GET_MAJOR_VERSION(common->game_version) > SCS_GET_MAJOR_VERSION(IMPLEMENTED_VERSION))
            log_warning("WARNING: This plugin was implemented for ATS version %u.%u. Current version is %u.%u. Some or all features might not work.",
                SCS_GET_MAJOR_VERSION(IMPLEMENTED_VERSION), SCS_GET_MINOR_VERSION(IMPLEMENTED_VERSION),
                SCS_GET_MAJOR_VERSION(common->game_version), SCS_GET_MINOR_VERSION(common->game_version));
    } else {
        game_name = "Unsupported game (!)";
        log_error("ERROR: Unsupported game, aborting initialization.");
        return SCS_RESULT_unsupported;
    }

    log_message("Game session: %s (%s) v%u.%u", game_name, common->game_id,
        SCS_GET_MAJOR_VERSION(common->game_version), SCS_GET_MINOR_VERSION(common->game_version));

    // Register for events. Note that failure to register those basic events
    // likely indicates invalid usage of the api or some critical problem. As the
    // example requires all of them, we can not continue if the registration fails.

    const bool events_registered =
        (version_params->register_for_event(SCS_TELEMETRY_EVENT_paused, telemetry_pause, NULL) == SCS_RESULT_ok) &&
        (version_params->register_for_event(SCS_TELEMETRY_EVENT_started, telemetry_pause, NULL) == SCS_RESULT_ok) &&
        (version_params->register_for_event(SCS_TELEMETRY_EVENT_configuration, telemetry_configuration, NULL) == SCS_RESULT_ok)
        ;
    if (!events_registered) {
        // Registrations created by unsuccessfull initialization are
        // cleared automatically so we can simply exit.
        log_error("ERROR: Unable to register event callbacks");
        return SCS_RESULT_generic_error;
    }

    // Register for the configuration info. As this example only prints the retrieved
    // data, it can operate even if that fails.

    // Register for channels. The channel might be missing if the game does not support
    // it (SCS_RESULT_not_found) or if does not support the requested type
    // (SCS_RESULT_unsupported_type).
    // !For purpose of this example we ignore the failures
    // !so the unsupported channels will remain at theirs default value.

    scs_result_t retstat;
#define SCS_EtoS retstat == SCS_RESULT_already_registered ? "already registered" : \
    retstat == SCS_RESULT_invalid_parameter ? "invalid parameter" : \
    retstat == SCS_RESULT_not_found ? "not found" : \
    retstat == SCS_RESULT_not_now ? "not now" : \
    retstat == SCS_RESULT_unsupported ? "unsupported" : \
    retstat == SCS_RESULT_unsupported_type ? "unsupported type" : \
    retstat == SCS_RESULT_generic_error ? "generic error" : "unknown error"

#define HANDLE_NOK(what) if(retstat != SCS_RESULT_ok) { \
        log_error("ERROR: Unable to register function to fetch " what ": %s", SCS_EtoS); \
        return SCS_RESULT_generic_error; \
    }

#define REGISTER_TELEMETRY(channel, type, tdMember, desc) \
    retstat = version_params->register_for_channel( \
        SCS_TELEMETRY_TRUCK_CHANNEL_ ## channel, SCS_U32_NIL, \
        SCS_VALUE_TYPE_ ## type, SCS_TELEMETRY_CHANNEL_FLAG_none, \
        update_ ## type ## _value, &truck_data. ## tdMember); \
    HANDLE_NOK(desc);

    REGISTER_TELEMETRY(effective_throttle, float, throttle, "effective throttle position");
    REGISTER_TELEMETRY(speed, float, speed, "current speed");
    REGISTER_TELEMETRY(engine_rpm, float, rpm, "current rpm");
    REGISTER_TELEMETRY(engine_gear, s32, gear, "current gear");
    REGISTER_TELEMETRY(engine_enabled, bool, revving, "engine state");
    REGISTER_TELEMETRY(motor_brake, bool, engbrake, "engine brake engaged state");

    InitTruckData();

    StartPolling();

    log_message("Initialization complete");

    return SCS_RESULT_ok;
}

SCSAPI_VOID scs_telemetry_shutdown() {
    log_message("Shutting down.");

    StopPolling();
    ShutdownDirectInput();
    game_log = nullptr;
}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}