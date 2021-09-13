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
    SampleDelayLowGainId = 27,
    ActiveCapLowGainId = 28,
    TopPlatePinId = 30,
    LowGainRId = 31,
    HighGainRId = 32,
    AutoSampleDelayId = 33,
    AutoSampleTimeoutId = 34,
    AutoSampleThresholdId = 35,
    AutoSampleHoldoffId = 36,
    InvertedOptoId = 75,
    FeedbackGainPId = 100,
    FeedbackGainIId = 101,
    FeedbackGainDId = 102
};

union ConfigOptionValue {
    uint32_t i32;
    float f32;
};

struct ConfigOptionDescriptor {
    const uint16_t id;
    const bool isFloat;
    const ConfigOptionValue defaultValue;
    const char *name;
    const char *description;
    const char *type;
};

struct AppConfig {

    static void init();

    static inline bool HvControlEnabled() { return (bool)optionValues[HvControlEnabledId].i32; }
    static inline float HvControlTarget() { return optionValues[HvControlTargetId].f32; }
    static inline int32_t HvControlOutput() { return optionValues[HvControlOutputId].i32; }

    static inline int32_t ScanSyncPin() { return optionValues[ScanSyncPinId].i32; }
    static inline int32_t ScanStartDelay() { return optionValues[ScanStartDelayId].i32; }
    static inline int32_t ScanBlankDelay() { return optionValues[ScanBlankDelayId].i32; }
    static inline int32_t SampleDelay() { return optionValues[SampleDelayId].i32; }
    static inline int32_t SampleDelayLowGain() { return optionValues[SampleDelayLowGainId].i32; }
    static inline int32_t BlankingDelay() { return optionValues[BlankingDelayId].i32; }
    static inline int32_t IntegratorResetDelay() { return optionValues[IntegratorResetDelayId].i32; }
    static inline bool AugmentTopPlateLowSide() { return (bool)optionValues[AugmentTopPlateLowSideId].i32; }
    static inline bool ActiveCapLowGain() { return (bool)optionValues[ActiveCapLowGainId].i32; }

    static inline int32_t TopPlatePin() { return optionValues[TopPlatePinId].i32; }

    // Sense resistor used for low gain capacitance sense setting
    static inline float LowGainR() { return optionValues[LowGainRId].f32; }
    // Sense resistor used for high gain capacitance sense setting
    static inline float HighGainR() { return optionValues[HighGainRId].f32; }
    // Gain of integrator after the sense resistor, with units of V per V-sec.
    // Used to convert ADC voltage to real capacitance units for convenience
    static inline float CapAmplifierGain() {  return 2.0*22.36*25000.0; }

    static inline bool AutoSampleDelay() { return (bool)optionValues[AutoSampleDelayId].i32; }
    static inline int32_t AutoSampleHoldoff() { return (float)optionValues[AutoSampleHoldoffId].i32; }
    static inline int32_t AutoSampleTimeout() { return optionValues[AutoSampleTimeoutId].i32; }
    static inline float AutoSampleThreshold() { return optionValues[AutoSampleThresholdId].f32; }

    static inline bool InvertedOpto() { return optionValues[InvertedOptoId].i32 != 0; }
    static inline float FeedbackKp() { return optionValues[FeedbackGainPId].f32; }
    static inline float FeedbackKi() { return optionValues[FeedbackGainIId].f32; }
    static inline float FeedbackKd() { return optionValues[FeedbackGainDId].f32; }

    // Number of options defined in options
    static const uint32_t N_OPT_DESCRIPTOR;
    // Maximum supported ID value for options
    static const uint32_t MAX_OPT_ID = 128;
    static ConfigOptionDescriptor optionDescriptors[];
    // Trade some memory for access speed
    static ConfigOptionValue optionValues[MAX_OPT_ID];

    static const uint32_t N_HV507 = 2;
    static const uint32_t N_PINS = N_HV507 * 64;
    static const uint32_t N_BYTES = N_HV507 * 8;

    static const uint32_t N_TEMP_SENSOR = 4;
    static const uint32_t TEMP_READ_PERIOD = 250000; // us

    static const uint32_t N_CAP_GROUPS = 5;
};

