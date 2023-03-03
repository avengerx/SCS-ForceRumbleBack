#include "pch.h"
#include "log.h"
#include "poller.h"
#include "fx.h"
#include "rumble.h"
#include "truck.h"

#define LOCK truck_data_access.lock();
#define UNLOCK truck_data_access.unlock();

#define POLL_INTERVAL 10

#define WAITPOLL Sleep(POLL_INTERVAL);
#define WAITNEXT WAITPOLL continue;

bool concurrent_thread_running = false;
bool polling = false;
std::mutex threadlock;

void Poll();

HRESULT StartPolling() {
    polling = true;
    log("Polling for truck state changes...");
    new std::thread(Poll);
    concurrent_thread_running = true;
    return S_OK;
}

HRESULT StopPolling() {
    polling = false;
    threadlock.lock();
    log("Stopped polling for truck state changes.");
    // don't just set it to false, wait the thread to exit!
    concurrent_thread_running = false;
    threadlock.unlock();
    return S_OK;
}

void Poll() {
    threadlock.lock();
    truck_info_t last, current;
    memset(&last, 0, sizeof(current));

    bool need_update = false;
    bool need_engine_toggle = false;
    long currforce = 0, newforce;
    bool di8failure = false;
#define SendForceCHK(x) di8failure = SendForce(x) != DI_OK
    SendForceCHK(currforce);
    log("Thread started polling.");
    while (polling) {
        LOCK;
        if (truck_data.paused) {
            UNLOCK;
            if (!last.paused) {
                log("Paused. Interrupting all rumble effects.");
                // stop all effects, but be ready to resume where they were once it is unpaused.
                SendForceCHK(0);
                last.paused = true;
            }
            WAITNEXT;
        } else if (last.paused) {
            log("Unpaused. Resuming all rumble effects.");
            // resume effects the way they were before pausing

            last = truck_data;
            UNLOCK;
            SendForceCHK(currforce);
            WAITNEXT;
        }

        if (truck_data.revving != last.revving) {
            need_engine_toggle = true;
            last = truck_data;
        } else if (truck_data.rpm != last.rpm || truck_data.rpm_max != last.rpm_max) {
            last = truck_data;
            need_update = true;
        }
        current = truck_data;
        UNLOCK;

        if (need_engine_toggle) {
            last.revving ? fx::StartEngine() : fx::StopEngine();
            need_engine_toggle = false;
        } else if (need_update) {
            need_update = false;

            newforce = fx::RevToForce(last);

            // Only send the effect update if it grew something the
            // device will actually be able to reflect.
            if (newforce < (currforce - FORCEGRANULARITY) || newforce >(currforce + FORCEGRANULARITY)) {
                currforce = newforce;
                SendForceCHK(currforce);
            }
        } else if (di8failure) {
            // always send current force if trying to recover from a failed SendForce() attempt.
            SendForceCHK(fx::RevToForce(last));
        }

        // Add a much longer delay in case the command failed to avoid flooding the bus
        if (di8failure) Sleep(1000);
        WAITPOLL;
    }
    SendForceCHK(0);
    log("Thread stopped polling.");
    threadlock.unlock();
}