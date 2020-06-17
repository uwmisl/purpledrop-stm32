#include "AppConfig.hpp"

#define INTOPT(_id, _defaultValue) {.id=_id, .isFloat=false, .value = {.i32=_defaultValue}, .defaultValue = {.i32=_defaultValue}}
#define FLTOPT(_id, _defaultValue) {.id=_id, .isFloat=true, .value = {.f32=_defaultValue}, .defaultValue = {.f32=_defaultValue}}

ConfigOptionDescriptor AppConfig::optionDescriptors[] = {
    FLTOPT(TargetVoltageId, 200.0),
    INTOPT(CapScanSettleDelayId, 2000),
    INTOPT(HvControlEnabledId, 0),
    FLTOPT(HvControlTargetId, 200.0),
    INTOPT(HvControlOutputId, 120),
    INTOPT(ScanSyncPinId, 0),
    INTOPT(ScanStartDelayId, 200000),
    INTOPT(ScanBlankDelayId, 4000),
    INTOPT(SampleDelayId, 10000),
    INTOPT(BlankingDelayId, 14000),
    INTOPT(IntegratorResetDelayId, 1000),
    INTOPT(AugmentTopPlateLowSideId, 0),
    INTOPT(TopPlatePinId, 97)
};

const uint32_t AppConfig::N_OPT_DESCRIPTOR = sizeof(AppConfig::optionDescriptors) / sizeof(AppConfig::optionDescriptors[0]);

ConfigOptionValue AppConfig::optionValues[MAX_OPT_ID];

void AppConfig::init() {
    for(uint32_t i=0; i<N_OPT_DESCRIPTOR; i++) {
        optionValues[optionDescriptors[i].id] = optionDescriptors[i].defaultValue;
    }
}
