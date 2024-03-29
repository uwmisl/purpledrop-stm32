#include "version.hpp"

#include "Comms.hpp"
#include "MessageFramer.hpp"
#include "Messages.hpp"

using namespace events;

void Comms::init(
        EventBroker *broker
) {
    mBroker = broker;

    mParamaterDescriptorTxPos = AppConfig::N_OPT_DESCRIPTOR;

    mCapScanHandler.setFunction([this](auto &e) { HandleCapScan(e); });
    mBroker->registerHandler(&mCapScanHandler);
    mCapActiveHandler.setFunction([this](auto &e) { HandleCapActive(e); });
    mBroker->registerHandler(&mCapActiveHandler);
    mCapGroupsHandler.setFunction([this](auto &e) { HandleCapGroups(e); });
    mBroker->registerHandler(&mCapGroupsHandler);
    mElectrodesUpdatedHandler.setFunction([this](auto &e) { HandleElectrodesUpdated(e); });
    mBroker->registerHandler(&mElectrodesUpdatedHandler);
    mTemperatureMeasurementHandler.setFunction([this](auto &e) { HandleTemperatureMeasurement(e); });
    mBroker->registerHandler(&mTemperatureMeasurementHandler);
    mHvRegulatorUpdateHandler.setFunction([this](auto &e) { HandleHvRegulatorUpdate(e); });
    mBroker->registerHandler(&mHvRegulatorUpdateHandler);
    mDutyCycleUpdatedHandler.setFunction([this](auto &e){ HandleDutyCycleUdpated(e); });
    mBroker->registerHandler(&mDutyCycleUpdatedHandler);
}

void Comms::poll() {
    uint8_t *msgBuf;
    uint16_t msgLen;
    uint8_t readByte;
    while(modm::platform::UsbUart0::read(readByte)) {
        if(mFramer.push(readByte, msgBuf, msgLen)) {
            ProcessMessage(msgBuf, msgLen);
        }
    }
    PeriodicSend();
}

void Comms::ProcessMessage(uint8_t *buf, uint16_t len) {
    if(len == 0) {
        return;
    }

    switch(buf[0]) {
        case CalibrateCommandMsg::ID:
            {
                CalibrateCommandMsg msg(buf, len);
                Serializer ser(&mTxQueue);
                if(msg.command == CalibrateCommandMsg::CommandType::CapacitanceOffset) {
                    events::CapOffsetCalibrationRequest event;
                    mBroker->publish(event);
                }
                SendAck(CalibrateCommandMsg::ID);
            }
            break;
        case DataBlobMsg::ID:
            {
                DataBlobMsg msg;
                msg.fill(buf, len);
                if(msg.blob_id == DataBlobId::SoftwareVersionBlob) {
                    SendBlob(DataBlobId::SoftwareVersionBlob, (uint8_t*)VERSION_STRING, strlen(VERSION_STRING));
                } else if(msg.blob_id == DataBlobId::OffsetCalibration) {
                    events::UpdateElectrodeCalibration event;
                    event.offset = msg.chunk_index;
                    event.length = msg.payload_size;
                    event.data = msg.data;
                    mBroker->publish(event);
                    SendAck(DataBlobMsg::ID);
                }
            }
            break;
        case ElectrodeEnableMsg::ID:
            {
                ElectrodeEnableMsg msg(buf, len);
                events::SetElectrodes event;
                event.groupID = msg.groupID;
                event.setting = msg.setting;
                for(uint32_t i=0; i<AppConfig::N_BYTES; i++) {
                    event.values[i] = msg.values[i];
                }
                mBroker->publish(event);
            }
            break;
        case FeedbackCommandMsg::ID:
            {
                FeedbackCommandMsg msg(buf, len);
                events::FeedbackCommand event;
                event.target = msg.target;
                event.mode = msg.mode;
                event.measureGroupsNMask = msg.measureGroupsNMask;
                event.measureGroupsPMask = msg.measureGroupsPMask;
                event.baseline = msg.baseline;
                mBroker->publish(event);
            }
            break;
        case GpioControlMsg::ID:
            {
                GpioControlMsg msg(buf, len);
                events::GpioControl event;
                event.pin = msg.pin;
                event.outputEnable = (bool)(msg.flags & GpioControlMsg::OutputFlag);
                event.value = (bool)(msg.flags & GpioControlMsg::ValueFlag);
                event.write = !(bool)(msg.flags & GpioControlMsg::ReadFlag);
                event.callback = [this](uint8_t pin, bool value) {
                    GpioControlMsg resp;
                    Serializer ser(&mTxQueue);
                    resp.pin = pin;
                    resp.flags = 0;
                    if(value) {
                        resp.flags |= GpioControlMsg::ValueFlag;
                    }
                    resp.serialize(ser);
                    //mFlush();
                };
                mBroker->publish(event);
            }
            break;
        case ParameterDescriptorMsg::ID:
            // Kick off transmission of all parameter descriptors
            mParamaterDescriptorTxPos = 0;
            break;
        case ParameterMsg::ID:
            {
                ParameterMsg msg;
                events::SetParameter event;
                msg.fill(buf, len);
                event.paramIdx = msg.paramIdx;
                event.paramValue.i32 = msg.paramValue.i32;
                event.writeFlag = msg.writeFlag;
                event.callback = [this](const uint32_t &idx, const ConfigOptionValue &value) {
                    ParameterMsg msg;
                    Serializer ser(&mTxQueue);
                    msg.paramIdx = idx;
                    msg.paramValue.i32 = value.i32;
                    msg.writeFlag = 0;
                    msg.serialize(ser);
                    //mFlush();
                };
                mBroker->publish(event);
            }
            break;
        case SetGainMsg::ID:
            {
                SetGainMsg msg;
                events::SetGain event;
                Serializer ser(&mTxQueue);
                msg.fill(buf, len);
                for(uint32_t i=0; i<msg.count; i++) {
                    event.set_channel(i, msg.get_channel(i));
                }
                mBroker->publish(event);
                SendAck(SetGainMsg::ID);
            }
            break;
        case SetPwmMsg::ID:
            {
                SetPwmMsg msg;
                events::SetPwm event;
                Serializer ser(&mTxQueue);
                msg.fill(buf, len);
                event.channel = msg.channel;
                event.duty_cycle = msg.duty_cycle;
                mBroker->publish(event);
                SendAck(SetPwmMsg::ID);
            }
            break;
    }
}

void Comms::HandleCapActive(CapActive &e) {
    ActiveCapacitanceMsg msg;
    Serializer ser(&mTxQueue);
    msg.baseline = e.baseline;
    msg.measurement = e.measurement;
    msg.settings = e.settings;
    msg.serialize(ser);
    //mFlush();
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

void Comms::HandleCapGroups(CapGroups &e) {
    BulkCapacitanceMsg msg;
    Serializer ser(&mTxQueue);
    msg.groupScan = 1;
    msg.count = e.measurements.size();
    msg.startIndex = 0;
    for(uint32_t i=0; i<e.measurements.size() && i<msg.MAX_VALUES; i++) {
        msg.values[i] = e.measurements[i];
    }
    msg.serialize(ser);
    //mFlush();
}

void Comms::HandleElectrodesUpdated(ElectrodesUpdated &e) {
    (void)e;
    CommandAckMsg msg;
    Serializer ser(&mTxQueue);
    msg.acked_id = ElectrodeEnableMsg::ID;
    msg.serialize(ser);
    //mFlush();
}

void Comms::HandleHvRegulatorUpdate(HvRegulatorUpdate &e) {
    mHvUpdateCounter++;
    if(mHvUpdateCounter >= HvMessageDivider) {
        mHvUpdateCounter = 0;
        HvRegulatorMsg msg;
        Serializer ser(&mTxQueue);
        msg.voltage = e.voltage;
        msg.vTargetOut = e.vTargetOut;
        msg.serialize(ser);
        //mFlush();
    }
}

void Comms::HandleTemperatureMeasurement(TemperatureMeasurement &e) {
    TemperatureMsg msg;
    Serializer ser(&mTxQueue);
    msg.count = AppConfig::N_TEMP_SENSOR;
    for(uint32_t i=0; i<AppConfig::N_TEMP_SENSOR; i++) {
        msg.temps[i] = e.measurements[i];
    }
    msg.serialize(ser);
    //mFlush();
}

void Comms::HandleDutyCycleUdpated(DutyCycleUpdated &e) {
    DutyCycleUpdatedMsg msg;
    Serializer ser(&mTxQueue);
    msg.dutyCycleA = e.dutyCycleA;
    msg.dutyCycleB = e.dutyCycleB;
    msg.serialize(ser);
    //mFlush();
}

void Comms::PeriodicSend() {
    if(mCapScanTimer.poll()) {
        if((mCapScanTxPos >= AppConfig::N_PINS) && mCapScanDataDirty) {
            mCapScanDataDirty = false;
            mCapScanTxPos = 0;
        }
        if(mCapScanTxPos < AppConfig::N_PINS) {
            BulkCapacitanceMsg msg;
            Serializer ser(&mTxQueue);
            msg.groupScan = 0;
            msg.startIndex = mCapScanTxPos;
            msg.count = AppConfig::N_PINS - mCapScanTxPos;
            if(msg.count > CapScanMsgSize) {
                msg.count = CapScanMsgSize;
            }
            for(uint32_t i=0; i<msg.count; i++) {
                msg.values[i] = mCapScanData[i + msg.startIndex];
            }
            mCapScanTxPos += msg.count;
            msg.serialize(ser);
            //mFlush();
        }
    }

    if(mParameterTxTimer.poll() && mParamaterDescriptorTxPos < AppConfig::N_OPT_DESCRIPTOR) {
        ParameterDescriptorMsg msg;
        ConfigOptionDescriptor *desc = &AppConfig::optionDescriptors[mParamaterDescriptorTxPos];
        Serializer ser(&mTxQueue);
        msg.param_id = desc->id;
        memcpy(msg.defaultValue, &desc->defaultValue, sizeof(msg.defaultValue));
        msg.sequence_number = mParamaterDescriptorTxPos;
        msg.sequence_total = AppConfig::N_OPT_DESCRIPTOR;
        msg.name = desc->name;
        msg.description = desc->description;
        msg.type = desc->type;
        msg.serialize(ser);
        //mFlush();
        mParamaterDescriptorTxPos++;
    }
}

void Comms::SendBlob(uint8_t blob_id, const uint8_t *buf, uint32_t size) {
    // We don't handle multi-packet blobs yet
    // In the future, for larger data blobs (e.g. Parameter descriptions) this
    // needs to chunk them up, and queue for transmission over time.

    DataBlobMsg msg;
    Serializer ser(&mTxQueue);
    msg.blob_id = blob_id;
    msg.chunk_index = 0;
    msg.payload_size = size;
    msg.data = buf;
    msg.serialize(ser);
    //mFlush();
}

void Comms::SendAck(uint8_t acked_id) {
    CommandAckMsg ack;
    Serializer ser(&mTxQueue);
    ack.acked_id = acked_id;
    ack.serialize(ser);
    //mFlush();
}