#include "modm/platform.hpp"

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
static const float MAX_INTEGRATOR = 750.0;
static const float SLEW_INTEGRATOR = 3;
// Empirically determined linear relationship between DAC counts and output
// voltage under nominal load
static const float TARGET_SCALE = 5.7;
static const float TARGET_OFFSET = 1325;

template<class T>
concept IDacOut = requires(T a) {
    {a.setOutput((uint16_t)0)} -> std::convertible_to<void>;
};

template<IDacOut Dac, class Analog>
struct HvRegulator {
    HvRegulator() : mTimer(CONTROL_PERIOD_US), mIntegral(0.0) {}

    void init(EventEx::EventBroker *broker) {
        mBroker = broker;
    }

    void poll() {
        if(mTimer.poll()) {
            events::HvRegulatorUpdate event;
            uint16_t output = 0;
            int32_t vdiff = 0;
            for(uint32_t i=0; i<N_OVERSAMPLE; i++) {
                vdiff += Analog::readVhvDiff();
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
            Dac::setOutput(output);
            event.voltage = mVoltageMeasure;
            event.vTargetOut = output;
            mBroker->publish(event);
        }
    }

private:
    EventEx::EventBroker *mBroker;
    PeriodicPollingTimer mTimer;
    float mVoltageMeasure;
    float mIntegral;

    static const uint32_t CONTROL_PERIOD_US = 10000;
};
