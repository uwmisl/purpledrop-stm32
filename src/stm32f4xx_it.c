/*
* Define interrupt vectors
*/

#include "stm32f4xx.h"
#include "usbd_cdc_core_loopback.h"

void NMI_Handler(void)
{
    while(1);
}

void HardFault_Handler(void)
{
    while(1);
}

void MemManage_Handler(void)
{
    while(1);
}

void BusFault_Handler(void)
{
    while(1);
}

void UsageFault_Handler(void)
{
    while(1);
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

extern USB_OTG_CORE_HANDLE USB_OTG_dev;
extern uint32_t USBD_OTG_ISR_Handler(USB_OTG_CORE_HANDLE * pdev);
void OTG_FS_IRQHandler(void)
{
  USBD_OTG_ISR_Handler(&USB_OTG_dev);
}

