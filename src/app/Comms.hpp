#pragma once
#include "AppConfig.hpp"
#include "CircularBuffer.hpp"
#include "EventEx.hpp"
#include "Events.hpp"
using namespace EventEx;

/** Application messaging interface
 * 
 * Provides bridge between events and serial messages
 */
struct Comms {
    static void init(
        EventBroker *broker,
        IConsumer<uint8_t> *rx_queue,
        IProducer<uint8_t> *tx_queue,
        void (*flush)()
    );

    static void HandleCapScan(CapScanEvent &e);

private:
    static EventBroker *mBroker;
    static IConsumer<uint8_t> *mRxQueue;
    static IProducer<uint8_t> *mTxQueue;
    static void(*mFlush)(void);

    static uint16_t mCapScanData[AppConfig::N_PINS];
    static uint32_t mCapScanTxPos;
    static bool mCapScanDataDirty;
    static const uint32_t CapScanMsgSize = 8;

    // Allocate storage for event handlers
    static EventHandlerFunction<CapScanEvent> mCapScanHandler;


    static void PeriodicSend();
};