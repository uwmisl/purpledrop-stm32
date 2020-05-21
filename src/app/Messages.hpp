#pragma once

#include <cstdint>
#include <cstring>
#include "MessageFramer.hpp"
#include "AppConfig.hpp"

/** For each message type, a struct is defined here. The string does three things:
 *
 * 1. `predictSize` looks at the beginning or a paylaod in progress to
 *      determine the size of the message. For constant size packets, this can
 *      return a constant. For variable packets (e.g. packets with an array and
 *      a count field), it may not be possible to determine the size until
 *      enough of the data is received. In this case, predictSize should return 0.
 *      If the packet is not valid -- e.g. it has an invalid ID or its length is
 *      longer than the allowable size -- predictSize can return -1 to cause the
 *      framer to abandon parsing.
 *
 * 2.  `fill` takes a complete message payload and attempts to populate the
 *      message struct from the data contained there in. Messages should always
 *      copy the data, as the buffer may be re-used.
 *
 * 3.   `serialize` sends the message to the provided Serializer. No framing is
 *      done by the message. `serialize` just converts struct fields to payload
 *      bytes.
 *
 * All payload buffers include the ID field in the first byte.
 *
 * All messages must be added to the switch statement in Messages::predictSize.
 ***/

struct ElectrodeEnableMsg {
    static const uint8_t ID = 0;

    static int predictSize(uint8_t *buf, uint32_t length) {
        (void)buf;
        (void)length;
        return 16;
    }

    uint8_t values[16];
};

struct BulkCapacitanceMsg {
    static const uint8_t ID = 2;
    static const uint8_t MAX_VALUES = 16;

    BulkCapacitanceMsg() : start_index(0), count(0) {}

    static int predictSize(uint8_t *buf, uint32_t length) {
        if(length < 3) {
            return 0;
        } else {
            return buf[2] * 2 + 3;
        }
    }

    bool fill(uint8_t *buf, uint32_t length) {
        if(length == 0 || (int)length != predictSize(buf, length)) {
            return false;
        }

        start_index = buf[1];
        count = buf[2];
        if(count > MAX_VALUES) {
            return false;
        }
        for(int i=0; i<count; i++) {
            values[i] = (uint16_t)buf[3 + i*2] + (uint16_t)buf[4 + i*2] * 256;
        }
        return true;
    }

    void serialize(Serializer &ser) {
        ser.push(ID);
        ser.push(start_index);
        ser.push(count);
        for(int i=0; i<count; i++) {
            ser.push((uint8_t)(values[i] & 0xff));
            ser.push((uint8_t)(values[i] >> 8));
        }
        ser.finish();
    }

    uint8_t start_index;
    uint8_t count;
    uint16_t values[MAX_VALUES];
};

struct ActiveCapacitanceMsg {
    static const uint8_t ID = 3;

    uint16_t baseline;
    uint16_t measurement;

    void serialize(Serializer &ser) {
        ser.push(ID);
        ser.push(baseline);
        ser.push(measurement);
        ser.finish();
    }
};

struct CommandAckMsg {
    static const uint8_t ID = 4;
    static const uint8_t MAX_VALUES = 64;

    CommandAckMsg() : acked_id(0) {}

    CommandAckMsg(uint8_t id) : acked_id(id) {}

    static int predictSize(uint8_t *buf, uint32_t length) {
        (void)buf;
        (void)length;
        return 2;
    }

    bool fill(uint8_t *buf, uint32_t length) {
        if(length == 0 || (int)length != predictSize(buf, length)) {
            return false;
        }

        acked_id = buf[1];
        return true;
    }

    void serialize(Serializer &ser) {
        ser.push(ID);
        ser.push(acked_id);
        ser.finish();
    }

    uint8_t acked_id;
};

// Message sent to device to set (writeFlag = 1) or request (writeFlag = 0)
// the value of a device. The same message is returned from the device in
// response with the new value -- response is sent for both writes and
// requests, so that it serves as an ACK.
// The parameter value may be a 4-byte integer or float, depending on the
// parameter.
struct ParameterMsg {
    static const uint8_t ID = 6;

    static int predictSize(uint8_t *buf, uint32_t length) {
        (void)buf;
        (void)length;
        return 10;
    }

    void fill(uint8_t *buf, uint32_t length) {
        (void)length;
        paramIdx = *((uint32_t*)&buf[1]);
        paramValue.i32 = *((int32_t*)&buf[5]);
        writeFlag = buf[9];
    }

    void serialize(Serializer &s) {
        s.push(ID);
        s.push(paramIdx);
        s.push(paramValue.i32);
        s.push(writeFlag);

        s.finish();
    }

    uint32_t paramIdx;
    union {
        float f32;
        int32_t i32;
    } paramValue;
    // If set, this message is setting a param. If clear, this is requesting the current value.
    uint8_t writeFlag;
};

struct TemperatureMsg {
    static const uint8_t ID = 7;
    static const uint32_t MAX_COUNT = AppConfig::N_TEMP_SENSOR;
    uint8_t count; // Number
    int16_t temps[MAX_COUNT];

    void serialize(Serializer &s) {
        if(count > MAX_COUNT) {
            count = MAX_COUNT;
        }
        s.push(ID);
        s.push(count);
        for(uint32_t i=0; i<count; i++) {
            s.push(temps[i]);
        }
        s.finish();
    }
};

struct HvRegulatorMsg {
    static const uint8_t ID = 8;

    float voltage;
    uint16_t vTargetOut;

    void serialize(Serializer &s) {
        s.push(ID);
        s.push(voltage);
        s.push(vTargetOut);
        s.finish();
    }
};

#define PREDICT(msgname) case msgname::ID: \
    return msgname::predictSize(buf, length);

struct Messages {
    static int predictSize(uint8_t *buf, uint32_t length) {
        if(length < 1) {
            return 0; // No message type yet, so no idea how long
        }
        uint8_t id = buf[0];

        switch(id) {
            PREDICT(ElectrodeEnableMsg)
            PREDICT(BulkCapacitanceMsg)
            PREDICT(ParameterMsg)
            default:
                return -1;
        }
    }
};
