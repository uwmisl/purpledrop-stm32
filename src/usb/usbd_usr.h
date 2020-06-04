#pragma once

#include <stdio.h>



/**
 * User defined callbacks for the USB driver
 */

extern void USBD_USR_Init(void);

extern void USBD_USR_DeviceReset(uint8_t speed);

extern void USBD_USR_DeviceConfigured(void);

extern void USBD_USR_DeviceSuspended(void);

extern void USBD_USR_DeviceResumed(void);

extern void USBD_USR_DeviceConnected(void);

extern void USBD_USR_DeviceDisconnected(void);

USBD_Usr_cb_TypeDef USR_cb = {
  USBD_USR_Init,
  USBD_USR_DeviceReset,
  USBD_USR_DeviceConfigured,
  USBD_USR_DeviceSuspended,
  USBD_USR_DeviceResumed,


  USBD_USR_DeviceConnected,
  USBD_USR_DeviceDisconnected,
};