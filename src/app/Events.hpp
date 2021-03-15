#pragma once

#include "AppConfig.hpp"

#include "EventEx.hpp"

using namespace EventEx;

/** Defines all of the events in the application */

namespace events {

struct CapActive : public Event {
    CapActive() : CapActive(0, 0) {}

    CapActive(uint16_t _baseline, uint16_t _measurement) : baseline(_baseline), measurement(_measurement) {}

    uint16_t baseline; // Initial zero level
    uint16_t measurement; // Final voltage value
};

struct CapOffsetCalibrationRequest : public Event {}; 

struct CapScan : public Event {
    const uint16_t *measurements; // Size is N_HV507 * 64
};

struct CapGroups : public Event {
    std::array<uint16_t, AppConfig::N_CAP_GROUPS> measurements;
};

struct ElectrodesUpdated : public Event {};

struct GpioControl : public Event {
    uint8_t pin;
    bool value;
    bool outputEnable;
    bool write;
    std::function<void(uint8_t pin, bool value)> callback;
};

struct SetGain : public Event {
    SetGain() : data{0} {}

    uint8_t data[(AppConfig::N_PINS + 3) / 4];

    uint8_t get_channel(uint8_t channel) {
        uint32_t offset = channel / 4;
        uint32_t shift = (channel % 4) * 2;
        return (data[offset] >> shift) & 0x3;
    }

    void set_channel(uint8_t channel, uint8_t value) {
        uint32_t offset = channel / 4;
        uint32_t shift = (channel % 4) * 2;
        if(offset < sizeof(data)/sizeof(data[0])) {
            data[offset] &= ~(0x3 << shift);
            data[offset] |= (value & 0x3) << shift;
        }
    }
};

struct SetElectrodes : public Event {
    uint8_t groupID;
    uint8_t setting;
    uint8_t values[AppConfig::N_BYTES];
};

struct SetDutyCycle : public Event {
    bool updateA;
    bool updateB;
    uint8_t dutyCycleA;
    uint8_t dutyCycleB;
};

struct DutyCycleUpdated : public Event {
    uint8_t dutyCycleA;
    uint8_t dutyCycleB;
};

struct FeedbackCommand : public Event {
    float target;
    uint8_t mode;
    uint8_t measureGroupsPMask;
    uint8_t measureGroupsNMask;
    uint8_t baseline;
};

struct SetParameter : public Event {
    uint32_t paramIdx;
    ConfigOptionValue paramValue;
    // If set, this message is setting a param. If clear, this is requesting the current value.
    uint8_t writeFlag;
    std::function<void(const uint32_t &idx, const ConfigOptionValue &paramValue)> callback;
};

struct SetPwm : public Event {
    uint8_t channel;
    uint16_t duty_cycle;
};

struct TemperatureMeasurement : public Event {
    uint16_t measurements[AppConfig::N_TEMP_SENSOR];
};

struct HvRegulatorUpdate : public Event {
    float voltage;
    uint16_t vTargetOut;
};


} //namespace events