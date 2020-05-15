#pragma once
#include <cstdint>

struct AppConfig {
    static uint32_t CommonPin() { return 15; }

    static const uint32_t N_HV507 = 2;
    static const uint32_t N_PINS = N_HV507 * 64;
    static const uint32_t N_BYTES = N_HV507 * 8;
};