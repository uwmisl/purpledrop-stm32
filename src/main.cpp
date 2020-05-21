#include <cstdio>
#include <cstring>

extern "C" {
    #include "rcc.h"
    #include "system_stm32f4xx.h"
    #include "stm32f4xx_ip_dbg.h"
}

#include "modm/platform.hpp"
#include "modm/platform/clock/rcc.hpp"
#include "modm/driver/pwm/pca9685.hpp"
#include "modm/architecture/interface/clock.hpp"

#include "usb_vcp.hpp"
#include "Analog.hpp"
#include "AppConfigController.hpp"
#include "Comms.hpp"
#include "EventEx.hpp"
#include "HV507.hpp"
#include "HvRegulator.hpp"
#include "Max31865.hpp"
#include "CircularBuffer.hpp"
#include "SystemClock.hpp"
#include "TempSensors.hpp"

using namespace modm::platform;
using namespace modm::literals;

static const uint8_t PCA9685_I2C_ADDR = 0x40;
using I2C = I2cMaster1;

StaticCircularBuffer<uint8_t, 512> USBTxBuffer;
StaticCircularBuffer<uint8_t, 512> USBRxBuffer;

Analog analog;
AppConfigController appConfigController;
HV507<> hvControl;
EventEx::EventBroker broker;
Comms comms;
HvRegulator hvRegulator;
TempSensors tempSensors;
PeriodicPollingTimer mainLoopTimer(1000, true);
modm::Pca9685<I2C> pwmChip(PCA9685_I2C_ADDR);

using LoopTimingPin = GpioB11;

int main() {
    // Setup global peripheral register structs for use in debugger
    IP_Debug();

    SystemClock::enable();
	SysTickTimer::initialize<SystemClock>();

    printf("Beginning Initialization...\n");
    analog.init();
    pwmChip.initialize();
    appConfigController.init(&broker);
    hvControl.init(&broker, &analog);
    hvRegulator.init(&broker, &analog);
    tempSensors.init(&broker);

    VCP_Setup(&USBRxBuffer, &USBTxBuffer);
    comms.init(&broker, &USBRxBuffer, &USBTxBuffer, &VCP_FlushTx);

    LoopTimingPin::setOutput(Gpio::OutputType::PushPull);
    while(1) {
        if(mainLoopTimer.poll()) {
            LoopTimingPin::set();
            comms.poll();
            tempSensors.poll();
            hvRegulator.poll();
            // hvControl.drive();
            LoopTimingPin::reset();
        }
    }
}


void assert_failed(uint8_t* file, uint32_t line) {
    (void)file;
    (void)line;
    printf("ASSERT FAILURE\n");
    while(1) {}
}