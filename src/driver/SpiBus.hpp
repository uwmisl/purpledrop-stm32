#pragma once

extern "C" {
#include "stm32f4xx.h"
}

struct SpiTransfer {
    SpiTransfer() {
        txBuf = NULL;
        rxBuf = NULL;
        xferLength = 0;
        callback = NULL;
    }

    uint8_t *txBuf;
    uint8_t *rxBuf;

    uint32_t xferLength;

    void (*callback)();
};



struct SpiBus {
    // Initialize the SPI bus.
    // This controller only supports DMA transfers. You can only use certain DMA stream/
    // channel combinations with certain SPIx peripherals. No checking is done here, so 
    // user is responsible to check the datasheet and choose wisely.
    void init(
        SPI_TypeDef *spi_dev,
        DMA_Stream_TypeDef *tx_dma_stream,
        DMA_Stream_TypeDef *rx_dma_stream,
        uint8_t tx_dma_chan,
        uint8_t rx_dma_chan,
        bool cpol,
        bool cpha,
        uint32_t speed_hz,
    ) {
        RCC_ClocksTypeDef clocks;
        SPI_InitTypeDef spiInit;
        DMA_InitTypeDef dmaaInit;

        uint32_t base_clk = 0;
        switch(spi_dev) {
            case SPI2:
            case SPI3:
                base_clk = clocks.PCLK1_Frequency;
                break;
            default:
                base_clk = clocks.PCLK2_Frequency;
        }

        uint8_t br;
        uint32_t div = 2;
        while(base_clk / div > speed_hz) {
            div *= 2;
            br += 1;
        }


        SPI_StructInit(&spiInit);
        spiInit.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
        spiInit.SPI_Mode = SPI_Mode_Master;
        spiInit.SPI_DataSize = SPI_DataSize_8b;
        spiInit.SPI_CPOL = cpol ? SPI_CPOL_High : SPI_CPOL_Low;
        spiInit.SPI_CHPA = cpha ? SPI_CPHA_2Edge : SPI_CPHA_1Edg;
        spiInit.SPI_NSS = 0;
        spiInit.SPI_BaudRatePrescaler = br << 3;
        SPI_Init(SPI(), &spiInit);
        
        SPI_Cmd(SPI(), ENABLE);

        // Initialize TX dma
        DMA_StructInit()
        dmaInit.DMA_PeripheralBaseAddr = &SPI()->DR;
        dmaInit.DMA_Memory0BaseAddr = 
        dmaInit.DMA_DIR = 
        dmaInit.DMA_BufferSize =
        dmaInit.DMA_PeripheralInc = 
        dmaInit.DMA_MemoryInc = 
        dmaInit.DMA_PeripheralDataSize =
        dmaInit.DMA_MemoryDataSize =
        dmaInit.DMA_Mode =
        dmaInit.DMA_Priority = 
        dmaInit.DMA_FIFOMode =
        dmaInit.DMA_FIFOThreshold = 
        dmaInit.DMA_MemoryBurst =
        dmaInit.DMA_PeripheralBurst =
        dmaInit.DMA_Channel = channel;

    }

    void startTransfer(SpiTransfer *xfer) {

    }

    void doTransfer(SpiTransfer *xfer) {

    }
};