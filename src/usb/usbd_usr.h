#pragma once

#include <stdio.h>

/**
 * User defined callbacks for the USB driver
 */

void USBD_USR_Init(void)
{
  printf("USB INIT\n");
}

void USBD_USR_DeviceReset(uint8_t speed)
{
  printf("DEVICE RESET\n");
  switch (speed)
  {
  case USB_OTG_SPEED_HIGH:
  case USB_OTG_SPEED_FULL:
    break;
  }
}


void USBD_USR_DeviceConfigured(void)
{
  printf("DEVICE CONFIGURED\n");
}


void USBD_USR_DeviceSuspended(void)
{
}


void USBD_USR_DeviceResumed(void)
{
}


void USBD_USR_DeviceConnected(void)
{
  printf("DEVICE CONNECTED\n");
}

void USBD_USR_DeviceDisconnected(void)
{
  printf("DEVICE DISCONNECTED\n");
}

USBD_Usr_cb_TypeDef USR_cb = {
  USBD_USR_Init,
  USBD_USR_DeviceReset,
  USBD_USR_DeviceConfigured,
  USBD_USR_DeviceSuspended,
  USBD_USR_DeviceResumed,


  USBD_USR_DeviceConnected,
  USBD_USR_DeviceDisconnected,
};
