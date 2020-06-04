// Defines global functions used by USB_OTG driver to setup hardware

extern "C" {
#include "usb_bsp.h"
}

#include <modm/platform.hpp>

using namespace modm::platform;
using namespace std::chrono_literals;
#define GPIO_AF_OTG1_FS 10

extern "C" void USB_OTG_BSP_Init(USB_OTG_CORE_HANDLE * pdev)
{
  (void)pdev;
  printf("OTG BSP INIT\n");
  GpioA12::setOutput(Gpio::OutputType::PushPull);
  GpioA12::reset();
  modm::delay(10ms);
  GpioA12::setInput();
  GpioA11::setAlternateFunction(GPIO_AF_OTG1_FS);
  GpioA12::setAlternateFunction(GPIO_AF_OTG1_FS);
  //GpioA9::setAlternateFunction(GPIO_AF_OTG1_FS);
  GpioA9::setInput();
  // GpioA9 is used as VBUS sense input, but is left in default input state

  RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN; __DSB();
  RCC->AHB2RSTR |= RCC_AHB2RSTR_OTGFSRST; __DSB();
  RCC->AHB2RSTR &= ~RCC_AHB2RSTR_OTGFSRST;
}

extern "C" void USB_OTG_BSP_EnableInterrupt(USB_OTG_CORE_HANDLE * pdev)
{
  (void)pdev;
  printf("OTG IRQ ENABLE\n");
  NVIC_SetPriorityGrouping(1);
  NVIC_SetPriority(OTG_FS_IRQn, NVIC_EncodePriority(1, 1, 3));
  NVIC_EnableIRQ(OTG_FS_IRQn);
}

#define count_us   35
extern "C" void USB_OTG_BSP_uDelay(const uint32_t usec)
{
  uint32_t count = count_us * usec;
  do
  {
    if (--count == 0)
    {
      return;
    }
  }
  while (1);
}

extern "C" void USB_OTG_BSP_mDelay(const uint32_t msec)
{
  USB_OTG_BSP_uDelay(msec * 1000);
}
