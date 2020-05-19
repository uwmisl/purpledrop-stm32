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
    #include "usbd_cdc_core_loopback.h"
    #include "usbd_usr.h"
}

#include "CircularBuffer.hpp"
#include "modm/platform.hpp"

#define RX_BUF_SIZE CDC_DATA_MAX_PACKET_SIZE
#define TX_BUF_SIZE CDC_DATA_MAX_PACKET_SIZE
__ALIGN_BEGIN USB_OTG_CORE_HANDLE USB_OTG_dev __ALIGN_END;
static volatile bool UsbConnected = false;
static volatile bool TxActiveFlag = 0;
static uint8_t Rxbuffer[RX_BUF_SIZE];
static uint8_t Txbuffer[TX_BUF_SIZE];

static IProducer<uint8_t> *RxQueue = NULL;
static CircularBuffer<uint8_t> *TxQueue = NULL;

void FillTx() {
    if(!UsbConnected) {
        return;
    }
    uint32_t count = 0;
    while(!TxQueue->empty() && count < TX_BUF_SIZE) {
        Txbuffer[count++] = TxQueue->pop();
    }
    if(count > 0) {
        TxActiveFlag = true;
        DCD_EP_Tx(&USB_OTG_dev, CDC_IN_EP, Txbuffer, count);
        printf("Tx %ld bytes\n", count);
    }
}

void VCP_FlushTx() {
    if(!TxActiveFlag) {
        FillTx();
    }
}

void VCP_Reset() {
    TxQueue->clear();
    TxActiveFlag = false;
}

/* send data function */
uint32_t VCP_Send(uint8_t * pbuf, uint32_t buf_len, bool flush)
{
    uint32_t tx_count = 0;
    while(tx_count < buf_len) {
        if(!TxQueue->push(pbuf[tx_count])) {
            break;
        }
        tx_count++;
    }

    // if(flush) {
    //     VCP_FlushTx();
    // }
    return tx_count;
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

    DCD_DevConnect(&USB_OTG_dev);
    // Prepare first receive
    DCD_EP_PrepareRx(&USB_OTG_dev, CDC_OUT_EP, Rxbuffer, RX_BUF_SIZE);
}

extern "C" {
    static uint16_t DataTxCb(void)
    {
        printf("DataTxCB\n");
        TxActiveFlag = false;
        // Fill TX if there's data available, or do nothing if not
        FillTx();
        return USBD_OK;
    }

    static uint16_t DataRxCb(uint32_t len)
    {   
        // Copy data from working buffer to queue
        for(uint32_t i=0; i<len; i++) {
            RxQueue->push(Rxbuffer[i]);
        }
        // Prepare for next receive
        DCD_EP_PrepareRx(&USB_OTG_dev, CDC_OUT_EP, Rxbuffer, RX_BUF_SIZE);
        return USBD_OK;
    }

    CDC_IF_Prop_TypeDef VCP_fops = {
        DataTxCb,
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