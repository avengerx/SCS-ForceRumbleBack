#include "pch.h"
#include "log.h"
#include "poller.h"
#include "truck.h"

#define LOCK truck_data_access.lock();
#define UNLOCK truck_data_access.unlock();

#define POLL_INTERVAL 10

#define WAITPOLL Sleep(POLL_INTERVAL);
#define WAITNEXT WAITPOLL continue;

bool concurrent_thread_running = false;
bool polling = false;

HRESULT StartPolling() {
    polling = true;
    log("Polling for truck state changes...");
    concurrent_thread_running = true;
    return S_OK;
}

HRESULT StopPolling() {
    polling = false;
    log("Stopped polling for truck state changes.");
    // don't just set it to false, wait the thread to exit!
    concurrent_thread_running = false;
    return S_OK;
}

void Poll() {
    truck_info_t last, current;
    memset(&last, 0, sizeof(current));

    while (polling) {
        LOCK
        if (truck_data.paused) {
            UNLOCK
            if (!last.paused) {
                // stop all effects, but be ready to resume where they were once it is unpaused.
            }
            WAITNEXT
        } else if (last.paused) {
            UNLOCK
            // resume effects the way they were before pausing
            WAITNEXT
        }
        current = truck_data;
        UNLOCK

        WAITPOLL
    }
}