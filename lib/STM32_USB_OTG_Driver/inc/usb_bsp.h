#pragma once

#include "usb_core.h"

#ifdef __cplusplus
extern "C" {
#endif

void USB_OTG_BSP_Init (USB_OTG_CORE_HANDLE *pdev);
void USB_OTG_BSP_uDelay (const uint32_t usec);
void USB_OTG_BSP_mDelay (const uint32_t msec);
void USB_OTG_BSP_EnableInterrupt (USB_OTG_CORE_HANDLE *pdev);
//void USB_OTG_BSP_TimerIRQ (void);
#ifdef __cplusplus
}
#endif

