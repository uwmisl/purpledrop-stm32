#pragma once
#include <cstdint>

/** Datastructure for representing capacitance scan groups
 * 
 * Used for fine grained control of capacitance measurement
 * 
 * This class provides static storage for scan groups, and provides a link
 * between scan group settings and the HV507 controller that implements
 * scanning.
 */
template <uint32_t N_PINS>
class ScanGroups {
public:
    typedef std::array<uint8_t, (N_PINS + 7) / 8> PinMask;

    /* Max number of groups supported */ 
    static const uint32_t MaxGroups = 5;

    bool isAnyGroupActive() {
        for(uint8_t group = 0; group < MaxGroups; group++) {
            if(mGroupSizes[group] > 0) {
                return true;
            }
        }
        return false;
    }

    bool isGroupActive(uint8_t group) {
        if(group < MaxGroups && mGroupSizes[group] > 0) {
            return true;
        }
        return false;
    }

    PinMask getGroupMask(uint8_t group) {
        if(group < MaxGroups) {
            return mGroupChannels[group];
        } else {
            return {0};
        }
    }

    /** Get setting byte for a group */
    uint8_t getGroupSetting(uint8_t group) {
        if(group < MaxGroups) {
            return mGroupSettings[group];
        } else {
            return 0;
        }
    }

private:
    PinMask mGroupChannels[MaxGroups];
    uint8_t mGroupSizes[MaxGroups];
    uint8_t mGroupSettings[MaxGroups];
};