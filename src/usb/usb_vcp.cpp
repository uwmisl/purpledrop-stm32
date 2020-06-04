/** USB Serial Port Driver using ST OTG library.
 *
 * Call VCP_Setup with circular buffers for Rx and Tx queues.
 *
 * All received data will be placed into the receive buffer. For sending data,
 * call VCP_Send() in order to properly initiate a new USB transcation --
 * writing to the buffer is not sufficient.
 */
extern "C" {
    #include "usbd_desc.h"
    #include "usbd_cdc_core.h"
    #include "usbd_usr.h"
}

#include "CircularBuffer.hpp"
#include "modm/platform.hpp"

#define RX_BUF_SIZE CDC_DATA_MAX_PACKET_SIZE
#define TX_BUF_SIZE CDC_DATA_MAX_PACKET_SIZE
__ALIGN_BEGIN USB_OTG_CORE_HANDLE USB_OTG_dev __ALIGN_END;
static volatile bool UsbConnected = false;

/* These are external variables imported from CDC core to be used for IN
   transfer management. */
extern uint8_t  APP_Rx_Buffer [];
extern uint32_t APP_Rx_ptr_in;
extern uint32_t APP_Rx_ptr_out;
extern uint8_t USB_Tx_State;

static IProducer<uint8_t> *RxQueue = NULL;
static CircularBuffer<uint8_t> *TxQueue = NULL;

void FillTx() {
    // The USB-CDC driver has no API for transmitting.
    // Instead, we manipulate its internal buffer variables,
    // and it will start a transfer upon incoming SOF frames.

    // One day, we could try the USB driver here:
    // https://github.com/STMicroelectronics/STM32CubeF4, or some
    // other open source driver that makes sense.

    if(!UsbConnected) {
        return;
    }

    NVIC_DisableIRQ(OTG_FS_IRQn);
    if(USB_Tx_State == USB_CDC_IDLE) {
        if(APP_Rx_ptr_out == APP_Rx_ptr_in) {
            APP_Rx_ptr_out = 0;
            APP_Rx_ptr_in = 0;
        }
        while(!TxQueue->empty() && APP_Rx_ptr_in < APP_RX_DATA_SIZE) {
            uint8_t b = TxQueue->pop();
            APP_Rx_Buffer[APP_Rx_ptr_in++] = b;
        }
    }
    NVIC_EnableIRQ(OTG_FS_IRQn);
}

void VCP_FlushTx() {
    FillTx();
}

void VCP_Reset() {
    TxQueue->clear();
}

void VCP_Setup(IProducer<uint8_t> *rx_queue, CircularBuffer<uint8_t> *tx_queue)
{
    RxQueue = rx_queue;
    TxQueue = tx_queue;

    USBD_Init(
        &USB_OTG_dev,
        USB_OTG_FS_CORE_ID,
        &USR_desc,
        &USBD_CDC_cb, // From ST cdc class driver
        &USR_cb
    );
}

extern "C" {
    static uint16_t InitCb(void) {
        UsbConnected = true;
        TxQueue->clear();
        return USBD_OK;
    }

    static uint16_t DeInitCb(void) {
        UsbConnected = false;
        return USBD_OK;
    }

    static uint16_t CtrlCb(uint32_t Cmd, uint8_t* Buf, uint32_t Len) {
        (void)Buf;
        (void)Len;
        switch (Cmd)
        {
        case SEND_ENCAPSULATED_COMMAND:
            printf("USB-CDC: SEND_ENCAPSULATED_COMMAND\n");
            break;

        case GET_ENCAPSULATED_RESPONSE:
            printf("USB-CDC: GET_ENCAPSULATED_RESPONSE\n");
            break;

        case SET_COMM_FEATURE:
            printf("USB-CDC: SET_COMM_FEATURE\n");
            break;

        case GET_COMM_FEATURE:
            printf("USB-CDC: GET_COMM_FEATURE\n");
            break;

        case CLEAR_COMM_FEATURE:
            printf("USB-CDC: CLEAR_COMM_FEATURE\n");
            break;

        case SET_LINE_CODING:
            printf("USB-CDC: SET_LINE_CODING\n");
            break;

        case GET_LINE_CODING:
            printf("USB-CDC: GET_LINE_CODING\n");
            break;

        case SET_CONTROL_LINE_STATE:
            printf("USB-CDC: SET_CONTROL_LINE_STATE\n");
            break;

        case SEND_BREAK:
            printf("USB-CDC: SEND_BREAK\n");
            break;

        default:
            break;
        }

        return USBD_OK;
    }

    static uint16_t DataRxCb(uint8_t *buf, uint32_t len)
    {
        // Copy data from working buffer to queue
        for(uint32_t i=0; i<len; i++) {
            RxQueue->push(buf[i]);
        }
        return USBD_OK;
    }

    CDC_IF_Prop_TypeDef VCP_fops = {
        InitCb,
        DeInitCb,
        CtrlCb,
        NULL,
        DataRxCb
    };
}


extern "C" {
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
  VCP_Reset();
  UsbConnected = true;
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


} // extern "C"