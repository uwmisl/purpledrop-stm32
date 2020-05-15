#pragma once

#include "AppConfig.hpp"

#include "EventEx.hpp"

using namespace EventEx;

struct CapScanEvent : public Event {
    const uint16_t *measurements; // Size is N_HV507 * 64
};

