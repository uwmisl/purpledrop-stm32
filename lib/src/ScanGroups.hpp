#pragma once
#include <cstdint>

/** Data structure for representing capacitance scan groups
 * 
 * Used for fine grained control of capacitance measurement
 * 
 * This class provides static storage for scan groups, and provides a link
 * between scan group settings and the HV507 controller that implements
 * scanning.
 */
template <uint32_t N_PINS, uint32_t MAX_GROUPS>
class ScanGroups {
public:
    static const uint32_t N_BYTES = (N_PINS + 7) / 8;
    typedef std::array<uint8_t, N_BYTES> PinMask;

    bool isAnyGroupActive() {
        for(uint8_t group = 0; group < MAX_GROUPS; group++) {
            if(isGroupActive(group)) {
                return true;
            }
        }
        return false;
    }

    bool isGroupActive(uint8_t group) {
        for(uint8_t x : mGroupMasks[group]) {
            if(x != 0) {
                return true;
            }
        }
        return false;
    }

    void setGroup(uint8_t group, uint8_t setting, uint8_t *mask) {
        if(group >= MAX_GROUPS) {
            return;
        }
        mGroupSettings[group] = setting;
        for(uint32_t i=0; i<N_BYTES; i++) {
            mGroupMasks[group][i] = mask[i];
        }
    }

    PinMask getGroupMask(uint8_t group) {
        if(group < MAX_GROUPS) {
            return mGroupMasks[group];
        } else {
            return {0};
        }
    }

    /** Get setting byte for a group */
    uint8_t getGroupSetting(uint8_t group) {
        if(group < MAX_GROUPS) {
            return mGroupSettings[group];
        } else {
            return 0;
        }
    }

    inline bool isPinActive(uint8_t group, uint8_t pin) {
        uint8_t byteidx = pin / 8;
        uint8_t bit = pin % 8;
        return mGroupMasks[group][byteidx] & (1<<bit);
    }

private:
    PinMask mGroupMasks[MAX_GROUPS];
    uint8_t mGroupSettings[MAX_GROUPS];
};