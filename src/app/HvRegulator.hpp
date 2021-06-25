#include "modm/platform.hpp"

#include "Analog.hpp"
#include "AppConfig.hpp"
#include "Events.hpp"
#include "PeriodicPollingTimer.hpp"


// Resistor divider ratio of ADC measurement voltage to high voltage rail
static const float VDIVIDER = 6.65 / (412.0*3);
// Scale from ADC counts to HIGH voltage
static const float VSCALE = (3.3 / 4096.) / VDIVIDER;
static const uint32_t N_OVERSAMPLE = 12;
static const float FILTER = 0.25;
static const float K_INTEGRATOR = 0.3;
static const float MAX_INTEGRATOR = 500.0;
static const float SLEW_INTEGRATOR = 3;
// Empirically determined linear relationship between DAC counts and output
// voltage under nominal load
static const float TARGET_SCALE = 5.7;
static const float TARGET_OFFSET = 1325;


struct HvRegulator {
    HvRegulator() : mTimer(CONTROL_PERIOD_US), mIntegral(0.0) {}

    void init(EventEx::EventBroker *broker, Analog *analog) {
        mBroker = broker;
        mAnalog = analog;
        modm::platform::Dac::initialize();
        modm::platform::Dac::enableChannel(modm::platform::Dac::Channel::Channel1);
        modm::platform::Dac::connect<GpioA4::Out1>();
        modm::platform::Dac::enableOutputBuffer(modm::platform::Dac::Channel::Channel1, true);
    }

    void poll() {
        if(mTimer.poll()) {
            events::HvRegulatorUpdate event;
            uint16_t output = 0;
            int32_t vdiff = 0;
            for(uint32_t i=0; i<N_OVERSAMPLE; i++) {
                vdiff += mAnalog->readVhvDiff();
            }
            float vcal = (float)vdiff/N_OVERSAMPLE * VSCALE;
            mVoltageMeasure = mVoltageMeasure * (1 - FILTER) + vcal * FILTER;
            if(AppConfig::HvControlEnabled()) {
                float target = AppConfig::HvControlTarget();
                float delta = K_INTEGRATOR * (vcal - target);
                if(delta > SLEW_INTEGRATOR) {
                    delta = SLEW_INTEGRATOR;
                } else if (delta < -SLEW_INTEGRATOR) {
                    delta = -SLEW_INTEGRATOR;
                }
                mIntegral += delta;
                if(mIntegral > MAX_INTEGRATOR) {
                    mIntegral = MAX_INTEGRATOR;
                } else if (mIntegral < -MAX_INTEGRATOR) {
                    mIntegral = -MAX_INTEGRATOR;
                }
                output = (target * TARGET_SCALE + TARGET_OFFSET) - mIntegral;
            } else {
                output = AppConfig::HvControlOutput();
            }
            modm::platform::Dac::setOutput1(output);
            event.voltage = mVoltageMeasure;
            event.vTargetOut = output;
            mBroker->publish(event);
        }
    }

private:
    EventEx::EventBroker *mBroker;
    Analog *mAnalog;
    PeriodicPollingTimer mTimer;
    float mVoltageMeasure;
    float mIntegral;

    static const uint32_t CONTROL_PERIOD_US = 10000;
};
