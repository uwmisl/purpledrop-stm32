#pragma once

#include <cstdint>
#include "Events.hpp"
#include "modm/platform.hpp"
#include "modm/driver/pwm/pca9685.hpp"

static const uint32_t N_PWM_CHAN = 16;

struct PwmOutput {
    void init(EventEx::EventBroker *broker);

    void poll();

    void setDutyCycle(uint8_t channel, uint16_t duty_cycle);

private:
    EventEx::EventBroker *mBroker;
    EventHandlerFunction<events::SetPwm> mSetPwmHandler;
    modm::chrono::milli_clock::time_point mUpdateTimes[N_PWM_CHAN];
    uint16_t mDutyCycles[N_PWM_CHAN];
};
