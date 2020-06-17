#pragma once
#include <cstdint>

#include "EventEx.hpp"

// Enum for option IDs, to ensure uniqueness
enum ConfigOptionIds : uint8_t {
    TargetVoltageId = 0,
    CapScanSettleDelayId = 1,
    HvControlEnabledId = 10,
    HvControlTargetId = 11,
    HvControlOutputId = 12,
    ScanSyncPinId = 20,
    ScanStartDelayId = 21,
    ScanBlankDelayId = 22,
    SampleDelayId = 23,
    BlankingDelayId = 24,
    IntegratorResetDelayId = 25,
    AugmentTopPlateLowSideId = 26,
    TopPlatePinId = 30,

};

union ConfigOptionValue {
    uint32_t i32;
    float f32;
};

struct ConfigOptionDescriptor {
    const uint16_t id;
    const bool isFloat;
    ConfigOptionValue value;
    const ConfigOptionValue defaultValue;
};

struct AppConfig {
    
    static void init();

    static bool HvControlEnabled() { return (bool)optionValues[HvControlEnabledId].i32; }
    static float HvControlTarget() { return optionValues[HvControlTargetId].f32; }
    static int32_t HvControlOutput() { return optionValues[HvControlOutputId].i32; }

    static int32_t ScanSyncPin() { return optionValues[ScanSyncPinId].i32; }
    static int32_t ScanStartDelay() { return optionValues[ScanStartDelayId].i32; }
    static int32_t ScanBlankDelay() { return optionValues[ScanBlankDelayId].i32; }
    static int32_t SampleDelay() { return optionValues[SampleDelayId].i32; }
    static int32_t BlankingDelay() { return optionValues[BlankingDelayId].i32; }
    static int32_t IntegratorResetDelay() { return optionValues[IntegratorResetDelayId].i32; }
    static bool AugmentTopPlateLowSide() { return (bool)optionValues[AugmentTopPlateLowSideId].i32; }

    static int32_t TopPlatePin() { return optionValues[TopPlatePinId].i32; }

    // Number of options defined in options
    static const uint32_t N_OPT_DESCRIPTOR;
    // Maximum supported ID value for options 
    static const uint32_t MAX_OPT_ID = 64;
    static ConfigOptionDescriptor optionDescriptors[];
    // Trade some memory for access speed
    static ConfigOptionValue optionValues[MAX_OPT_ID];


    static const uint32_t N_HV507 = 2;
    static const uint32_t N_PINS = N_HV507 * 64;
    static const uint32_t N_BYTES = N_HV507 * 8;

    static const uint32_t N_TEMP_SENSOR = 4;
    static const uint32_t TEMP_READ_PERIOD = 250000; // us
};

