#pragma once
#include "AppConfig.hpp"
#include "CircularBuffer.hpp"
#include "EventEx.hpp"
#include "Events.hpp"
#include "Messages.hpp"
#include "PeriodicPollingTimer.hpp"

using namespace EventEx;

/** Application messaging interface
 *
 * Provides bridge between events and serial messages
 */
struct Comms {

    Comms() : mCapScanTimer(CapScanTxPeriod * CapScanMsgSize / AppConfig::N_PINS) {}

    void init(
        EventBroker *broker,
        IConsumer<uint8_t> *rx_queue,
        IProducer<uint8_t> *tx_queue,
        void (*flush)()
    );

    /** To be called periodically by app to process any data in rx queue */
    void poll();

private:
    EventBroker *mBroker;
    IConsumer<uint8_t> *mRxQueue;
    IProducer<uint8_t> *mTxQueue;
    void(*mFlush)(void);

    MessageFramer<Messages> mFramer;

    PeriodicPollingTimer mCapScanTimer;
    uint16_t mCapScanData[AppConfig::N_PINS];
    uint32_t mCapScanTxPos;
    bool mCapScanDataDirty;
    static const uint32_t CapScanMsgSize = 8;
    static const uint32_t CapScanTxPeriod = 100000;

    uint16_t mHvUpdateCounter;
    static const uint16_t HvMessageDivider = 10;

    // Allocate storage for event handlers
    EventHandlerFunction<events::CapScan> mCapScanHandler;
    EventHandlerFunction<events::CapActive> mCapActiveHandler;
    EventHandlerFunction<events::ElectrodesUpdated> mElectrodesUpdatedHandler;
    EventHandlerFunction<events::SetParameterAck> mSetParameterAckHandler;
    EventHandlerFunction<events::TemperatureMeasurement> mTemperatureMeasurementHandler;
    EventHandlerFunction<events::HvRegulatorUpdate> mHvRegulatorUpdateHandler;

    void ProcessMessage(uint8_t *buf, uint16_t len);
    void HandleCapActive(events::CapActive &e);
    void HandleCapScan(events::CapScan &e);
    void HandleElectrodesUpdated(events::ElectrodesUpdated &e);
    void HandleSetParameterAck(events::SetParameterAck &e);
    void HandleTemperatureMeasurement(events::TemperatureMeasurement &e);
    void HandleHvRegulatorUpdate(events::HvRegulatorUpdate &e);

    void PeriodicSend();
};