#pragma once
#include "modm/platform.hpp"


/** Uses the systick timer to keep track of elapsed time. 
 * 
 * `poll()` should be called periodically, and will return true
 * when a period has elapsed. 
 */
struct PeriodicPollingTimer {
    PeriodicPollingTimer(uint32_t period_us, bool drop_on_overrun = false) : 
        mPeriod(period_us), 
        mDropOnOverrun(drop_on_overrun) 
    {}

    void reset() {
        uint32_t curTime = modm::chrono::micro_clock::now().time_since_epoch().count();
        mNextTime = curTime + mPeriod;
    }
    bool poll() {
        uint32_t curTime = modm::chrono::micro_clock::now().time_since_epoch().count();

        if(curTime >= mNextTime || mNextTime - curTime > (1UL << 31)) {
            mNextTime += mPeriod;
            // In drop-on-overrun mode, we don't accumulate events if we have a bubble
            // between poll calls
            if(mDropOnOverrun) {
                if(mNextTime < curTime) {
                    mNextTime = curTime + mPeriod;
                    return false;
                }
            }
            return true;
        }
        return false;
    }
private:
    uint32_t mPeriod;
    uint32_t mNextTime;
    bool mDropOnOverrun;
};
