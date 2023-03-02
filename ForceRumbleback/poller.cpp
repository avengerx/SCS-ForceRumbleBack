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

    log("Thread started polling.");
    while (polling) {
        LOCK;
        if (truck_data.paused) {
            UNLOCK;
            if (!last.paused) {
                log("Paused. Interrupting all rumble effects.");
                // stop all effects, but be ready to resume where they were once it is unpaused.
                last.paused = true;
            }
            WAITNEXT;
        } else if (last.paused) {
            UNLOCK;
            log("Unpaused. Resuming all rumble effects.");
            // resume effects the way they were before pausing
            last.paused = false;
            WAITNEXT;
        }
        current = truck_data;
        UNLOCK;

        WAITPOLL;
    }
    log("Thread stopped polling.");
    threadlock.unlock();
}