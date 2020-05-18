#include "Comms.hpp"
#include "MessageFramer.hpp"
#include "Messages.hpp"

using namespace events;

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

    mCapScanHandler.setFunction([this](auto &e) { HandleCapScan(e); });
    mBroker->registerHandler(&mCapScanHandler);
    mCapActiveHandler.setFunction([this](auto &e) { HandleCapActive(e); });
    mBroker->registerHandler(&mCapActiveHandler);
    mElectrodesUpdatedHandler.setFunction([this](auto &e) { HandleElectrodesUpdated(e); });
    mBroker->registerHandler(&mElectrodesUpdatedHandler);
    mSetParameterAckHandler.setFunction([this](auto &e) { HandleSetParameterAck(e); });
    mBroker->registerHandler(&mSetParameterAckHandler);
}

void Comms::poll() {
    uint8_t *msgBuf;
    uint16_t msgLen;
    while(!mRxQueue->empty()) {
        if(mFramer.push(mRxQueue->pop(), msgBuf, msgLen)) {
            printf("Got a message\n");
            ProcessMessage(msgBuf, msgLen);
        }
    }
}

void Comms::ProcessMessage(uint8_t *buf, uint16_t len) {
    if(len == 0) {
        return;
    }

    switch(buf[0]) {
        case ParameterMsg::ID:
            ParameterMsg msg;
            events::SetParameter event;
            msg.fill(buf, len);
            event.paramIdx = msg.paramIdx;
            event.paramValue.i32 = msg.paramValue.i32;
            event.writeFlag = msg.writeFlag;
            mBroker->publish(event);
            break;
    }
}

void Comms::HandleCapActive(CapActive &e) {
    ActiveCapacitanceMsg msg;
    Serializer ser(mTxQueue);
    msg.baseline = e.baseline;
    msg.measurement = e.measurement;
    msg.serialize(ser);
    mFlush();
}

void Comms::HandleCapScan(CapScan &e) {
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

void Comms::HandleElectrodesUpdated(ElectrodesUpdated &e) {
    CommandAckMsg msg;
    Serializer ser(mTxQueue);
    msg.acked_id = ElectrodeEnableMsg::ID;
    msg.serialize(ser);
    mFlush();
}

void Comms::HandleSetParameterAck(SetParameterAck &e) {
    ParameterMsg msg;
    Serializer ser(mTxQueue);
    msg.paramIdx = e.paramIdx;
    msg.paramValue.i32 = e.paramValue.i32;
    msg.writeFlag = 0;
    msg.serialize(ser);
    mFlush();
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
        mFlush();
    }
}