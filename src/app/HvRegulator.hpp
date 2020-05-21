#include "modm/platform.hpp"

#include "Analog.hpp"
#include "AppConfig.hpp"
#include "Events.hpp"
#include "PeriodicPollingTimer.hpp"

static const float VSCALE = (3.3 / 4096.) * 603.3 / 3.3;
static const uint32_t N_OVERSAMPLE = 12;
static const float FILTER = 0.25;

struct HvRegulator {
    HvRegulator() : mTimer(CONTROL_PERIOD_US) {}

    void init(EventEx::EventBroker *broker, Analog *analog) {
        mBroker = broker;
        mAnalog = analog;
        modm::platform::Dac::initialize();
        modm::platform::Dac::enableChannel(modm::platform::Dac::Channel::Channel1);
        modm::platform::Dac::connect<GpioA4::Out1>();
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
                // Do feedback control
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

    static const uint32_t CONTROL_PERIOD_US = 10000;
};
