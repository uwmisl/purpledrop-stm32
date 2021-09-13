#pragma once

#include <cstdint>
#include "Events.hpp"
#include <modm/platform.hpp>
#include <modm/driver/pwm/pca9685.hpp>

static constexpr uint8_t PCA9685_I2C_ADDR = 0x40;

template<typename I2C>
struct PwmOutput {

    template<typename SystemClock>
    void init(EventEx::EventBroker *broker) {
        mBroker = broker;
        for(uint32_t i=0; i<N_PWM_CHAN; i++) {
            mUpdateTimes[i] = modm::chrono::milli_clock::time_point();
        }
        mSetPwmHandler.setFunction([this](auto &e) { setDutyCycle(e.channel, e.duty_cycle); });
        broker->registerHandler(&mSetPwmHandler);

        I2C::template initialize<SystemClock>();
        RF_CALL_BLOCKING(pwmChip.setAllChannels(0));
        if ( !RF_CALL_BLOCKING(pwmChip.initialize(0, modm::pca9685::Mode2::MODE2_OUTDRV))) {
            printf("Error initializing PWM\n");
        }
    }

    void poll() {
        auto now = modm::chrono::milli_clock::now();
        for(uint32_t ch=0; ch<MAX_TIMEOUT_CHAN; ch++) {
            if(now - mUpdateTimes[ch] > PWM_TIMEOUT) {
                if(mDutyCycles[ch] != 0) {
                    setDutyCycle(ch, 0);
                }
            }
        }
    }

    void setDutyCycle(uint8_t channel, uint16_t duty_cycle) {
        if(channel > N_PWM_CHAN) {
            printf("Out of range PWM channel: %d\n", channel);
            return;
        }

        mUpdateTimes[channel] = modm::chrono::milli_clock::now();
        mDutyCycles[channel] = duty_cycle;
        RF_CALL_BLOCKING(pwmChip.setChannel(channel, duty_cycle));
    }

private:
    EventEx::EventBroker *mBroker;
    EventHandlerFunction<events::SetPwm> mSetPwmHandler;

    static constexpr uint32_t N_PWM_CHAN = 16;
    static modm::Pca9685<I2C> pwmChip;
    static constexpr modm::chrono::milli_clock::duration PWM_TIMEOUT = 15s;
    static const uint32_t MAX_TIMEOUT_CHAN = 3;
    modm::chrono::milli_clock::time_point mUpdateTimes[N_PWM_CHAN];
    uint16_t mDutyCycles[N_PWM_CHAN];
};

template<typename I2C>
modm::Pca9685<I2C> PwmOutput<I2C>::pwmChip = modm::Pca9685<I2C>(PCA9685_I2C_ADDR);