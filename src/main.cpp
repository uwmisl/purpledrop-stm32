#include <cstdio>
#include <cstring>

extern "C" {
    #include "stm32f4xx.h"
    #include "system_stm32f4xx.h"
    #include "stm32f4xx_ip_dbg.h"
}

#include "usb_vcp.hpp"
#include "CircularBuffer.hpp"
uint32_t SystickClock = 0;

// void SetupClocks() {
//     // Setup clocks to use 8MHz HSE -> 96MHz sys clock + 48MHz for USB
//     FLASH_SetLatency(FLASH_Latency_3);
//     RCC_HSEConfig(RCC_HSE_ON);
//     RCC_WaitForHSEStartUp();
//     RCC_PLLConfig(
//         RCC_PLLSource_HSE, 
//         8,   // PLLM,
//         384, // PLLN
//         4,   // PLLP
//         8,   // PLLQ
//         2    // PLLR
//     );
//     RCC_PLLCmd(ENABLE);
//     while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY));
//     RCC_HCLKConfig(RCC_SYSCLK_Div1);
//     RCC_PCLK1Config(RCC_HCLK_Div1);
//     RCC_PCLK2Config(RCC_HCLK_Div2);
//     RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
//     RCC_48MHzClockSourceConfig(RCC_CK48CLKSOURCE_PLLQ);

//     /* SysTick end of count event each ms */
//     SysTick_Config(96000);
// }

void SetupClocks() {
    Rcc::enableExternalClock();	// 8MHz
    const Rcc::PllFactors pllFactors{
        .pllMul = 8,
        .pllPrediv = 1
    };
    Rcc::enablePll(Rcc::PllSource::ExternalClock, pllFactors);
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

    /* SysTick end of count event each ms */
    SysTick_Config(96000);
}


extern "C" void SysTick_Handler(void)
{
    SystickClock++;
}


int main() {
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    // Setup global peripheral register structs for use in debugger
    IP_Debug();

    SetupClocks();    
    
    /* Enable required peripheral clocks */ 
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    // RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    // RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    
    printf("Hello world\n");

    //VCP_Setup(&Parser, &USBTxBuffer);

    static const uint32_t UPDATE_PERIOD = 20;
    uint32_t NextUpdateTime = UPDATE_PERIOD;
    while(1) {
        
        if(SystickClock >= NextUpdateTime) {
            NextUpdateTime += UPDATE_PERIOD;
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
    printf("ASSERT FAILURE\n");
    while(1) {}
}