#include "AppConfig.hpp"

#define INTOPT(_id, _defaultValue) {.id=_id, .isFloat=false, .value = {.i32=_defaultValue}, .defaultValue = {.i32=_defaultValue}}
#define FLTOPT(_id, _defaultValue) {.id=_id, .isFloat=true, .value = {.f32=_defaultValue}, .defaultValue = {.f32=_defaultValue}}

ConfigOptionDescriptor AppConfig::optionDescriptors[] = {
    FLTOPT(TargetVoltageId, 200.0),
    INTOPT(CapScanSettleDelayId, 2000),
    INTOPT(HvControlEnabledId, 0),
    FLTOPT(HvControlTargetId, 200.0),
    INTOPT(HvControlOutputId, 1241)
};

const uint32_t AppConfig::N_OPT_DESCRIPTOR = sizeof(AppConfig::optionDescriptors) / sizeof(AppConfig::optionDescriptors[0]);

ConfigOptionValue AppConfig::optionValues[MAX_OPT_ID];

void AppConfig::init() {
    for(uint32_t i=0; i<N_OPT_DESCRIPTOR; i++) {
        optionValues[optionDescriptors[i].id] = optionDescriptors[i].defaultValue;
    }
}
