#include "AppConfig.hpp"

#define BOOLOPT(_id, _defaultValue, _name, _description) {.id=_id, .isFloat=false, \
    .defaultValue={.i32=_defaultValue}, .name=_name, .description=_description, .type="bool"}
#define INTOPT(_id, _defaultValue, _name, _description)  {.id=_id, .isFloat=false, \
    .defaultValue={.i32=_defaultValue}, .name=_name, .description=_description, .type="int"}
#define FLTOPT(_id, _defaultValue, _name, _description)  {.id=_id, .isFloat=true, \
    .defaultValue={.f32=_defaultValue}, .name=_name, .description=_description, .type="float"}

ConfigOptionDescriptor AppConfig::optionDescriptors[] = {
    BOOLOPT(HvControlEnabledId, 0, "HV Control Enabled", "Enables feedback control of HV supply"),
    FLTOPT(HvControlTargetId, 200.0, "HV Voltage Setting", "Target voltage for HV supply"),
    INTOPT(HvControlOutputId, 120, "HV Manual Output", "CONTROL output voltage when feedback control is disabled"),
    INTOPT(ScanSyncPinId, 0, "Scan Sync Pin", "Set which scan channel to assert SYNC out on gpio C1; -1 means active sample"),
    INTOPT(ScanStartDelayId, 200000, "Scan Start Delay", "ns; delay between polarity switch and first scan measurement"),
    INTOPT(ScanBlankDelayId, 4000, "Scan Blank Delay", "ns; delay after asserting blank between each scan measurement"),
    INTOPT(SampleDelayId, 10000, "Sample Delay (high gain)", "ns; Duration of current integration"),
    INTOPT(SampleDelayLowGainId, 40000, "Sample Delay (low gain)", "ns; Duration of current integration"),
    INTOPT(BlankingDelayId, 14000, "Active Blank Delay", "ns; delay between blank and integrator reset for active measurement"),
    INTOPT(IntegratorResetDelayId, 1000, "Integrator reset delay", "ns; time between reset release and first sample"),
    BOOLOPT(AugmentTopPlateLowSideId, 0, "Enable Top Plate Augment FET", "Enable extra FET to drive top plate to GND"),
    INTOPT(TopPlatePinId, 97, "Top Plate Pin", "Pin number of HV507 output driving top plate"),
    BOOLOPT(InvertedOptoId, 0, "Inverting Optoisolators", "Invert all opto-isolator IOs to support alternative parts"),
    FLTOPT(FeedbackGainPId, 0.0, "Feedback KP", "Proportional gain for feedback drop control"),
    FLTOPT(FeedbackGainIId, 0.0, "Feedback KI", "Integral gain for feedback drop control"),
    FLTOPT(FeedbackGainDId, 0.0, "Feedback KD", "Differential gain for feedback drop control")
};

const uint32_t AppConfig::N_OPT_DESCRIPTOR = sizeof(AppConfig::optionDescriptors) / sizeof(AppConfig::optionDescriptors[0]);

ConfigOptionValue AppConfig::optionValues[MAX_OPT_ID];

void AppConfig::init() {
    for(uint32_t i=0; i<N_OPT_DESCRIPTOR; i++) {
        optionValues[optionDescriptors[i].id] = optionDescriptors[i].defaultValue;
    }
}
