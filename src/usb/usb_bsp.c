// Defines global functions used by USB_OTG driver to setup hardware

#include "usb_bsp.h"


void USB_OTG_BSP_Init(USB_OTG_CORE_HANDLE * pdev)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

  // // Pull-down the DP pin briefly so that host can detect when we reset
  // // This is primarily to support debugging
  // GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
  // GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  // GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  // GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  // GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  // GPIO_Init(GPIOA, &GPIO_InitStructure);
  // GPIO_ResetBits(GPIOA, GPIO_Pin_12);
  // USB_OTG_BSP_mDelay(10);

  /* Configure DM DP Pins */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_OTG1_FS);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource12, GPIO_AF_OTG1_FS);

  /* Configure VBUS Pin */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_OTG1_FS);

  // /* Configure ID pin */
  // GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  // GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  // GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  // GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  // GPIO_Init(GPIOA, &GPIO_InitStructure);
  // GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_OTG1_FS);

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
  RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_OTG_FS, ENABLE);
}

void USB_OTG_BSP_EnableInterrupt(USB_OTG_CORE_HANDLE * pdev)
{
  NVIC_InitTypeDef NVIC_InitStructure;

  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  NVIC_InitStructure.NVIC_IRQChannel = OTG_FS_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

#define count_us   35
void USB_OTG_BSP_uDelay(const uint32_t usec)
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

void USB_OTG_BSP_mDelay(const uint32_t msec)
{
  USB_OTG_BSP_uDelay(msec * 1000);
}
