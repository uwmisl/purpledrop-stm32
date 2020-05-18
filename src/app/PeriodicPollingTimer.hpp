#pragma once
#include "modm/platform.hpp"


/** Uses the systick timer to keep track of elapsed time. 
 * 
 * `poll()` should be called periodically, and will return true
 * when a period has elapsed. 
 */
struct PeriodicPollingTimer {
    PeriodicPollingTimer(uint32_t period_us) : mPeriod(period_us) {}

    void reset() {
        uint32_t curTime = modm::chrono::micro_clock::now().time_since_epoch().count();
        mNextTime = curTime + mPeriod;
    }
    bool poll() {
        uint32_t curTime = modm::chrono::micro_clock::now().time_since_epoch().count();
        if(curTime >= mNextTime) {
            mNextTime += mPeriod;
            return true;
        }
        return false;
    }
private:
    uint32_t mPeriod;
    uint32_t mNextTime;
};
