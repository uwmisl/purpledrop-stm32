#include "modm/platform.hpp"

#include "AppConfig.hpp"
#include "Events.hpp"
#include "PeriodicPollingTimer.hpp"

struct HvRegulator {
    HvRegulator() : mTimer(CONTROL_PERIOD_US) {}

    void init(EventEx::EventBroker *broker) {
        mBroker = broker;
        modm::platform::Dac::initialize();
        modm::platform::Dac::enableChannel(modm::platform::Dac::Channel::Channel1);
        modm::platform::Dac::connect<GpioA4::Out1>();
    }

    void poll() {
        if(mTimer.poll()) {
            if(AppConfig::HvControlEnabled()) {
                // Do feedback control
            } else {
                modm::platform::Dac::setOutput1(AppConfig::HvControlOutput());
            }
        }
    }

private:
    EventEx::EventBroker *mBroker;
    PeriodicPollingTimer mTimer;


    static const uint32_t CONTROL_PERIOD_US = 10000;
};
