#include <cstdio>
#include <cstring>

extern "C" {
    #include "rcc.h"
    #include "system_stm32f4xx.h"
    #include "stm32f4xx_ip_dbg.h"
}

#include <modm/platform.hpp>
#include <modm/platform/clock/rcc.hpp>
#include <modm/architecture/interface/clock.hpp>

#include "usb_vcp.hpp"
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

StaticCircularBuffer<uint8_t, 512> USBTxBuffer;
StaticCircularBuffer<uint8_t, 512> USBRxBuffer;

AppConfigController appConfigController;
HV507<> hvControl;
EventEx::EventBroker broker;
Comms comms;
HvRegulator hvRegulator;

int main() {
    // Setup global peripheral register structs for use in debugger
    IP_Debug();

    SystemClock::enable();
	SysTickTimer::initialize<SystemClock>();


    printf("Beginning Initialization...\n");
    appConfigController.init(&broker);
    hvControl.init(&broker);
    hvRegulator.init(&broker);

    VCP_Setup(&USBRxBuffer, &USBTxBuffer);
    comms.init(&broker, &USBRxBuffer, &USBTxBuffer, &VCP_FlushTx);

    while(1) {
        comms.poll();
        //TempSensors::poll();
        hvRegulator.poll();
        //hvControl.drive();
    }
}


void assert_failed(uint8_t* file, uint32_t line) {
    (void)file;
    (void)line;
    printf("ASSERT FAILURE\n");
    while(1) {}
}