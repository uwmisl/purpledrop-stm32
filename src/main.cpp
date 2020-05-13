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

using namespace modm::platform;
using namespace modm::literals;

StaticCircularBuffer<uint8_t, 512> USBTxBuffer;
StaticCircularBuffer<uint8_t, 512> USBRxBuffer;

Max31865<SpiMaster2, GpioB13> Rtd0;
Max31865<SpiMaster2, GpioB12> Rtd1;
Max31865<SpiMaster2, GpioB10> Rtd2;
Max31865<SpiMaster2, GpioB1> Rtd3;

HV507 HvControl;

struct SystemClock {
	static constexpr uint32_t Frequency = 96_MHz;
	static constexpr uint32_t Ahb = Frequency;
	static constexpr uint32_t Apb1 = Frequency;
	static constexpr uint32_t Apb2 = Frequency / 2;

	static constexpr uint32_t Adc    = Apb2;

	static constexpr uint32_t Spi1   = Apb2;
	static constexpr uint32_t Spi2   = Apb1;
	static constexpr uint32_t Spi3   = Apb1;
	static constexpr uint32_t Spi4   = Apb2;
	static constexpr uint32_t Spi5   = Apb2;

	static constexpr uint32_t Usart1 = Apb2;
	static constexpr uint32_t Usart2 = Apb1;
	static constexpr uint32_t Usart3 = Apb1;
	static constexpr uint32_t Uart4  = Apb1;
	static constexpr uint32_t Uart5  = Apb1;
	static constexpr uint32_t Usart6 = Apb2;

	static constexpr uint32_t I2c1   = Apb1;
	static constexpr uint32_t I2c2   = Apb1;
	static constexpr uint32_t I2c3   = Apb1;

	static constexpr uint32_t Apb1Timer = Apb1 * 2;
	static constexpr uint32_t Apb2Timer = Apb2 * 1;
	static constexpr uint32_t Timer1  = Apb2Timer;
	static constexpr uint32_t Timer2  = Apb1Timer;
	static constexpr uint32_t Timer3  = Apb1Timer;
	static constexpr uint32_t Timer4  = Apb1Timer;
	static constexpr uint32_t Timer5  = Apb1Timer;
	static constexpr uint32_t Timer9  = Apb2Timer;
	static constexpr uint32_t Timer10 = Apb2Timer;
	static constexpr uint32_t Timer11 = Apb2Timer;

	static bool inline
	enable()
	{
        // Setup clocks to use 8MHz HSE -> 96MHz sys clock + 48MHz for USB
        Rcc::enableExternalClock();	// 8MHz

        RCC_PLLConfig(
            RCC_PLLSource_HSE,
            8,   // PLLM,
            384, // PLLN
            4,   // PLLP
            8,   // PLLQ
            2    // PLLR
        );
        RCC->CR |= RCC_CR_PLLON;
        while(!(RCC->CR & RCC_CR_PLLRDY));
        // set flash latency for 72MHz
        Rcc::setFlashLatency<Frequency>();
        // switch system clock to PLL output
        Rcc::enableSystemClock(Rcc::SystemClockSource::Pll);
        Rcc::setAhbPrescaler(Rcc::AhbPrescaler::Div1);
        // APB1 has max. 36MHz
        Rcc::setApb1Prescaler(Rcc::Apb1Prescaler::Div2);
        Rcc::setApb2Prescaler(Rcc::Apb2Prescaler::Div1);
        // update frequencies for busy-wait delay functions
        Rcc::updateCoreFrequency<Frequency>();

		return true;
	}
};

int main() {
    // Setup global peripheral register structs for use in debugger
    IP_Debug();

    SystemClock::enable();
	SysTickTimer::initialize<SystemClock>();

    SpiMaster3::connect<GpioC10::Sck, GpioC12::Mosi, GpioUnused::Miso>();
    SpiMaster3::initialize<SystemClock, 6000000>();

    printf("Hello world\n");

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