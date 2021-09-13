#pragma once

#include <cstdint>

#include "modm/platform.hpp"

using namespace modm::literals;

struct SystemClock {
	static constexpr uint32_t Frequency = 96_MHz;
	static constexpr uint32_t Ahb = Frequency;
	static constexpr uint32_t Apb1 = Frequency / 2;
	static constexpr uint32_t Apb2 = Frequency;

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
        modm::platform::Rcc::enableExternalCrystal(); // 8MHz

        // RCC_PLLConfig(
        //     RCC_PLLSource_HSE,
        //     8,   // PLLM,
        //     384, // PLLN
        //     4,   // PLLP
        //     8,   // PLLQ
        //     2    // PLLR
        // );
		modm::platform::Rcc::PllFactors pllFactors = {
			.pllM = 8,
			.pllN = 384,
			.pllP  = 4,
			.pllQ = 8
		};
		modm::platform::Rcc::enablePll(modm::platform::Rcc::PllSource::Hse, pllFactors);
        RCC->CR |= RCC_CR_PLLON;
        while(!(RCC->CR & RCC_CR_PLLRDY));
        // set flash latency for 72MHz
        modm::platform::Rcc::setFlashLatency<Frequency>();
        // switch system clock to PLL output
        modm::platform::Rcc::enableSystemClock(modm::platform::Rcc::SystemClockSource::Pll);
        modm::platform::Rcc::setAhbPrescaler(modm::platform::Rcc::AhbPrescaler::Div1);
        // APB1 has max. 36MHz
        modm::platform::Rcc::setApb1Prescaler(modm::platform::Rcc::Apb1Prescaler::Div2);
        modm::platform::Rcc::setApb2Prescaler(modm::platform::Rcc::Apb2Prescaler::Div1);
        // update frequencies for busy-wait delay functions
        modm::platform::Rcc::updateCoreFrequency<Frequency>();

		return true;
	}
};
