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
    typedef std::conditional<N_PINS < 256, uint8_t, uint16_t> pin_t;
    typedef std::array<uint8_t, (N_PINS + 7) / 8> PinMask;

    /* Max number of groups supported */ 
    static const uint32_t MaxGroups = 5;
    /* Max number of channels (pins) in a single group */
    static const uint32_t MaxChannels = 8;

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

    mask_t getGroupMask(uint8_t group) {
        mask_t ret = {0};
        uint8_t group_size = 0;
        if(group < MaxGroups) {
            group_size = mGroupSizes[group];
        }
        for(uint32_t ch = 0; ch < group_size; ch++) {
            pin_t pin = mGroupChannels[group][ch];
            uint32_t byte = pin / 8;
            uint32_t bit = pin % 8;
            if(pin < N_PINS) {
                ret[byte] |= bit;
            }
        }
        return ret;
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
    pin_t mGroupChannels[MaxGroups][MaxChannels];
    uint8_t mGroupSizes[MaxGroups];
    uint8_t mGroupSettings[MaxGroups];
};