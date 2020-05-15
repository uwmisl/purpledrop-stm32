#include "Comms.hpp"
#include "MessageFramer.hpp"
#include "Messages.hpp"


void Comms::init(
        EventBroker *broker,
        IConsumer<uint8_t> *rx_queue,
        IProducer<uint8_t> *tx_queue,
        void (*flush)()
) {
    mBroker = broker;
    mRxQueue = rx_queue;
    mTxQueue = tx_queue;
    mFlush = flush;

    mCapScanHandler.setFunction([](CapScanEvent &e) { Comms::HandleCapScan(e); });
}

void Comms::HandleCapScan(CapScanEvent &e) {
    // Capacitance scan data is large, and is sent out in chunks to prevent 
    // blocking the communications channel for too long
    // This is a hold-over from when this was done on a slower serial port, 
    // and is probably less of an issue with USB but it's more flexible this
    // way. 
    for(uint32_t i=0; i<AppConfig::N_PINS; i++) {
        mCapScanData[i] = e.measurements[i];
    }
    mCapScanDataDirty = true;
}

void Comms::PeriodicSend() {
    if(mCapScanTxPos >= AppConfig::N_PINS && mCapScanDataDirty) {
        mCapScanDataDirty = false;
        mCapScanTxPos = 0;
    }
    if(mCapScanTxPos < AppConfig::N_PINS) {
        BulkCapacitanceMsg msg;
        Serializer ser(mTxQueue);
        msg.start_index = mCapScanTxPos;
        msg.count = std::min(CapScanMsgSize, AppConfig::N_PINS - mCapScanTxPos);
        for(uint32_t i=0; i<msg.count; i++) {
            msg.values[i] = mCapScanData[i + msg.start_index];
        }
        mCapScanTxPos += msg.count;
        msg.serialize(ser);
    }
}