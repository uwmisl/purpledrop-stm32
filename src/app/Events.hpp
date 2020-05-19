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

struct CapScan : public Event {
    const uint16_t *measurements; // Size is N_HV507 * 64
};

struct ElectrodesUpdated : public Event {};

struct SetParameter : public Event {
    uint32_t paramIdx;
    ConfigOptionValue paramValue;
    // If set, this message is setting a param. If clear, this is requesting the current value.
    uint8_t writeFlag;
};

struct SetParameterAck : public Event {
    uint32_t paramIdx;
    union {
        float f32;
        int32_t i32;
    } paramValue;
};

struct SetElectrodes : public Event {};

struct TemperatureMeasurement : public Event {
    uint16_t measurements[AppConfig::N_TEMP_SENSOR];
};


} //namespace events