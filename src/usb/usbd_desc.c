#include "usbd_desc.h"

/* USB Standard Device Descriptor */
__ALIGN_BEGIN uint8_t USBD_DeviceDesc[USB_SIZ_DEVICE_DESC] __ALIGN_END = {
  0x12,                         /* bLength */
  USB_DEVICE_DESCRIPTOR_TYPE,   /* bDescriptorType */
  0x00,                         /* bcdUSB */
  0x02,
  0x02,                         /* bDeviceClass */
  0x02,                         /* bDeviceSubClass */
  0x00,                         /* bDeviceProtocol */
  USB_OTG_MAX_EP0_SIZE,         /* bMaxPacketSize */
  LOBYTE(USBD_VID),             /* idVendor */
  HIBYTE(USBD_VID),             /* idVendor */
  LOBYTE(USBD_PID),             /* idVendor */
  HIBYTE(USBD_PID),             /* idVendor */
  0x00,                         /* bcdDevice rel. 2.00 */
  0x02,
  USBD_IDX_MFC_STR,             /* Index of manufacturer string */
  USBD_IDX_PRODUCT_STR,         /* Index of product string */
  USBD_IDX_SERIAL_STR,          /* Index of serial number string */
  USBD_CFG_MAX_NUM              /* bNumConfigurations */
};                              /* USB_DeviceDescriptor */

/* USB Standard Device Descriptor */
__ALIGN_BEGIN uint8_t USBD_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC]
  __ALIGN_END = {
  USB_LEN_DEV_QUALIFIER_DESC,
  USB_DESC_TYPE_DEVICE_QUALIFIER,
  0x00,
  0x02,
  0x00,
  0x00,
  0x00,
  0x40,
  0x01,
  0x00,
};

/* USB Standard Device Descriptor */
__ALIGN_BEGIN uint8_t USBD_LangIDDesc[USB_SIZ_STRING_LANGID] __ALIGN_END = {
  USB_SIZ_STRING_LANGID,
  USB_DESC_TYPE_STRING,
  LOBYTE(USBD_LANGID_STRING),
  HIBYTE(USBD_LANGID_STRING),
};

uint8_t USBD_StringSerial[USB_SIZ_STRING_SERIAL] = {
  USB_SIZ_STRING_SERIAL,
  USB_DESC_TYPE_STRING,
};

__ALIGN_BEGIN uint8_t USBD_StrDesc[USB_MAX_STR_DESC_SIZ] __ALIGN_END;


static void IntToUnicode(uint32_t value, uint8_t * pbuf, uint8_t len);
static void Get_SerialNum(void);

uint8_t *USBD_USR_DeviceDescriptor(uint8_t speed, uint16_t * length)
{
  *length = sizeof(USBD_DeviceDesc);
  return (uint8_t *) USBD_DeviceDesc;
}


uint8_t *USBD_USR_LangIDStrDescriptor(uint8_t speed, uint16_t * length)
{
  *length = sizeof(USBD_LangIDDesc);
  return (uint8_t *) USBD_LangIDDesc;
}

uint8_t *USBD_USR_ProductStrDescriptor(uint8_t speed, uint16_t * length)
{
  

  USBD_GetString((uint8_t *) (uint8_t *) USBD_PRODUCT_STRING, USBD_StrDesc, length);
  
  return USBD_StrDesc;
}

uint8_t *USBD_USR_ManufacturerStrDescriptor(uint8_t speed, uint16_t * length)
{
  USBD_GetString((uint8_t *) (uint8_t *) USBD_MANUFACTURER_STRING, USBD_StrDesc,
                 length);
  return USBD_StrDesc;
}

uint8_t *USBD_USR_SerialStrDescriptor(uint8_t speed, uint16_t * length)
{
  *length = USB_SIZ_STRING_SERIAL;

  /* Update the serial number string descriptor with the data from the unique
   * ID */
  Get_SerialNum();

  return (uint8_t *) USBD_StringSerial;
}

uint8_t *USBD_USR_ConfigStrDescriptor(uint8_t speed, uint16_t * length)
{
  USBD_GetString((uint8_t *) (uint8_t *) USBD_CONFIGURATION_STRING, USBD_StrDesc, length);
  return USBD_StrDesc;
}

uint8_t *USBD_USR_InterfaceStrDescriptor(uint8_t speed, uint16_t * length)
{
  USBD_GetString((uint8_t *) (uint8_t *) USBD_INTERFACE_STRING, USBD_StrDesc, length);
  return USBD_StrDesc;
}

static void Get_SerialNum(void)
{
  uint32_t deviceserial0, deviceserial1, deviceserial2;

  deviceserial0 = *(uint32_t *) DEVICE_ID1;
  deviceserial1 = *(uint32_t *) DEVICE_ID2;
  deviceserial2 = *(uint32_t *) DEVICE_ID3;

  deviceserial0 += deviceserial2;

  if (deviceserial0 != 0)
  {
    IntToUnicode(deviceserial0, &USBD_StringSerial[2], 8);
    IntToUnicode(deviceserial1, &USBD_StringSerial[18], 4);
  }
}

/**
  * @brief  Convert Hex 32Bits value into char 
  * @param  value: value to convert
  * @param  pbuf: pointer to the buffer 
  * @param  len: buffer length
  * @retval None
  */
static void IntToUnicode(uint32_t value, uint8_t * pbuf, uint8_t len)
{
  uint8_t idx = 0;

  for (idx = 0; idx < len; idx++)
  {
    if (((value >> 28)) < 0xA)
    {
      pbuf[2 * idx] = (value >> 28) + '0';
    }
    else
    {
      pbuf[2 * idx] = (value >> 28) + 'A' - 10;
    }

    value = value << 4;

    pbuf[2 * idx + 1] = 0;
  }
}

USBD_DEVICE USR_desc = {
  USBD_USR_DeviceDescriptor,
  USBD_USR_LangIDStrDescriptor,
  USBD_USR_ManufacturerStrDescriptor,
  USBD_USR_ProductStrDescriptor,
  USBD_USR_SerialStrDescriptor,
  USBD_USR_ConfigStrDescriptor,
  USBD_USR_InterfaceStrDescriptor,
};