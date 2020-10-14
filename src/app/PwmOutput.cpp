
#include "PwmOutput.hpp"
#include "SystemClock.hpp"

using namespace std::chrono_literals;

static const uint8_t PCA9685_I2C_ADDR = 0x40;
using I2C = modm::platform::I2cMaster1;
using SDA_PIN = modm::platform::GpioB7;
using SCL_PIN = modm::platform::GpioB6;
static modm::Pca9685<modm::platform::I2cMaster1> pwmChip(PCA9685_I2C_ADDR);
static const modm::chrono::milli_clock::duration PWM_TIMEOUT = 15s;


void PwmOutput::init(EventEx::EventBroker *broker) {
    mBroker = broker;
    for(uint32_t i=0; i<N_PWM_CHAN; i++) {
        mUpdateTimes[i] = modm::chrono::milli_clock::time_point();
    }
    mSetPwmHandler.setFunction([this](auto &e) { setDutyCycle(e.channel, e.duty_cycle); });
    broker->registerHandler(&mSetPwmHandler);

    I2C::connect<SDA_PIN::Sda, SCL_PIN::Scl>();
    I2C::initialize<SystemClock>();
    RF_CALL_BLOCKING(pwmChip.setAllChannels(0));
    if ( !RF_CALL_BLOCKING(pwmChip.initialize(0, modm::pca9685::Mode2::MODE2_OUTDRV))) {
        printf("Error initializing PWM\n");
    }
}

void PwmOutput::poll() {
    auto now = modm::chrono::milli_clock::now();
    for(uint32_t ch=0; ch<N_PWM_CHAN; ch++) {
        if(now - mUpdateTimes[ch] > PWM_TIMEOUT) {
            if(mDutyCycles[ch] != 0) {
                setDutyCycle(ch, 0);
            }
        }
    }
}

void PwmOutput::setDutyCycle(uint8_t chan, uint16_t duty_cycle) {
    printf("Setting PWM %d to %d\n", (int)chan,(int)duty_cycle);
    if(chan > N_PWM_CHAN) {
        printf("Out of range PWM channel: %d\n", chan);
        return;
    }

    mUpdateTimes[chan] = modm::chrono::milli_clock::now();
    mDutyCycles[chan] = duty_cycle;
    RF_CALL_BLOCKING(pwmChip.setChannel(chan, duty_cycle));
}
