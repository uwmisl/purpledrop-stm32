#include <cstdio>
#include <cstring>

extern "C" {
    #include "rcc.h"
    #include "system_stm32f4xx.h"
    #include "stm32f4xx_ip_dbg.h"
}

#include "modm/platform.hpp"
#include "modm/platform/clock/rcc.hpp"
#include "modm/architecture/interface/clock.hpp"

#include "usb_vcp.hpp"
#include "Analog.hpp"
#include "AuxGpios.hpp"
#include "CircularBuffer.hpp"
#include "AppConfigController.hpp"
#include "Comms.hpp"
#include "EventEx.hpp"
#include "HV507.hpp"
#include "HvRegulator.hpp"
#include "Max31865.hpp"
#include "PwmOutput.hpp"
#include "ScanGroups.hpp"
#include "SystemClock.hpp"
#include "TempSensors.hpp"

using namespace modm::platform;
using namespace modm::literals;

StaticCircularBuffer<uint8_t, 4096> USBTxBuffer;
StaticCircularBuffer<uint8_t, 512> USBRxBuffer;

using AuxGpioArray = GpioArray<
    GpioB0,
    GpioB2,
    GpioC5
>;


Analog analog;
AppConfigController appConfigController;
AuxGpios<AuxGpioArray> auxGpios;
HV507 hvControl;
EventEx::EventBroker broker;
Comms comms;
HvRegulator hvRegulator;
TempSensors tempSensors;
PeriodicPollingTimer mainLoopTimer(1000, true);
PwmOutput pwmOutput;
ScanGroups<AppConfig::N_PINS> scanGroups;

using LoopTimingPin = GpioB11;
using SwitchGreenPin = GpioC8;
using SwitchRedPin = GpioA8;


void update_switch_led(events::HvRegulatorUpdate &e) {
    if(e.voltage < 15.0) {
        SwitchRedPin::setOutput(true);
    } else {
        SwitchRedPin::setOutput(false);
    }
}
EventEx::EventHandlerFunction<events::HvRegulatorUpdate> 
switchUpdateHandler([](auto &e) { update_switch_led(e); });

int main() {
    // Setup global peripheral register structs for use in debugger
    IP_Debug();

    SystemClock::enable();
	SysTickTimer::initialize<SystemClock>();

    printf("Beginning Initialization...\n");
    analog.init();
    appConfigController.init(&broker);
    auxGpios.init(&broker);
    hvControl.init(&broker, &analog, &scanGroups);
    hvRegulator.init(&broker, &analog);
    tempSensors.init(&broker);
    pwmOutput.init(&broker);

    broker.registerHandler(&switchUpdateHandler);

    VCP_Setup(&USBRxBuffer, &USBTxBuffer);
    comms.init(&broker, &USBRxBuffer, &USBTxBuffer, &VCP_FlushTx);

    LoopTimingPin::setOutput(Gpio::OutputType::PushPull);
    while(1) {
        if(mainLoopTimer.poll()) {
            LoopTimingPin::set();
            comms.poll();
            tempSensors.poll();
            hvRegulator.poll();
            pwmOutput.poll();
            hvControl.drive();
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