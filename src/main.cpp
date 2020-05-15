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
#include "HV507.hpp"
#include "Max31865.hpp"
#include "CircularBuffer.hpp"
#include "SystemClock.hpp"

using namespace modm::platform;
using namespace modm::literals;

StaticCircularBuffer<uint8_t, 512> USBTxBuffer;
StaticCircularBuffer<uint8_t, 512> USBRxBuffer;

Max31865<SpiMaster2, GpioB13> Rtd0;
Max31865<SpiMaster2, GpioB12> Rtd1;
Max31865<SpiMaster2, GpioB10> Rtd2;
Max31865<SpiMaster2, GpioB1> Rtd3;

HV507<> HvControl;
MessageSender messageSender;

int main() {
    // Setup global peripheral register structs for use in debugger
    IP_Debug();

    SystemClock::enable();
	SysTickTimer::initialize<SystemClock>();


    printf("Beginning Initialization...\n");
    HvControl.init(&messageSender);

    //HvControl.setUpdateCallback(|| {})

    VCP_Setup(&USBRxBuffer, &USBTxBuffer);

    static const uint32_t UPDATE_PERIOD = 20;
    uint32_t NextUpdateTime = UPDATE_PERIOD;
    uint8_t rxBuf[64];
    uint8_t txBuf[8];
    while(1) {
        int rdCount = 0;
        while(!USBRxBuffer.empty()) {
            rxBuf[rdCount++] = USBRxBuffer.pop();
        }
        if(rdCount > 0) {
            printf("Sending data\n");
            VCP_Send(rxBuf, rdCount);
        }
        if(modm::chrono::milli_clock::now().time_since_epoch().count() >= NextUpdateTime) {
            NextUpdateTime += UPDATE_PERIOD;

            SpiMaster3::transferBlocking(txBuf, 0, 8);
            // float cur_rpm = RpmSense.value() * 60 / 7;
            // uint32_t pulse_width = Motor.update(cur_rpm);
            // if(ReportEnable) {
            //     snprintf(fmtBuf, MAX_FMT_SIZE, "MTR %lu %.1f %d\n", SystickClock, cur_rpm, (int)pulse_width);
            //     VCP_Send((uint8_t*)fmtBuf, strlen(fmtBuf));
            // }
        }
        //Parser.poll();
    }
}


void assert_failed(uint8_t* file, uint32_t line) {
    (void)file;
    (void)line;
    printf("ASSERT FAILURE\n");
    while(1) {}
}