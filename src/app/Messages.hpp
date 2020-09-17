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
 *      message struct from the data contained there in.
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

    ElectrodeEnableMsg() : values{0} {}

    static int predictSize(uint8_t *buf, uint32_t length) {
        (void)buf;
        (void)length;
        return 17;
    }

    bool fill(uint8_t* buf, uint32_t length) {
        if(length == 0 || (int)length != predictSize(buf, length)) {
            return false;
        }

        for(int i=0; i<16; i++) {
            values[i] = buf[i+1];
        }

        return true;
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
    int16_t temps[MAX_COUNT]; // degC * 100

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

struct SetPwmMsg {
    static const uint8_t ID = 9;

    static int predictSize(uint8_t *buf, uint32_t length) {
        (void)buf;
        (void)length;
        return 4;
    }

    void fill(uint8_t *buf, uint32_t length) {
        if(length >= 4) {
            channel = buf[1];
            duty_cycle = *((uint16_t*)&buf[2]);
        }
    }

    uint8_t channel;
    uint16_t duty_cycle;
};

/** Used to request and return a string of bytes from the purpledrop
 */
enum DataBlobId : uint16_t {
    SoftwareVersionBlob = 0
};

struct DataBlobMsg {
    static const uint8_t ID = 10;
    static const uint32_t MAX_SIZE = 250;

    uint8_t blob_id;
    uint8_t payload_size;
    uint16_t chunk_index;
    const uint8_t *data;

    static int predictSize(uint8_t *buf, uint32_t length) {
        if(length < 3) {
            return 0;
        }
        return buf[2] + 5;
    }

    void fill(uint8_t *buf, uint32_t length) {
        int32_t predicted_size = predictSize(buf, length);
        if(predicted_size <= 0 || (int)length != predicted_size) {
            blob_id = 0;
            chunk_index = 0;
            payload_size = 0;
            data = 0;
            return;
        }
        blob_id = buf[1];
        payload_size = buf[2];
        chunk_index = *((uint16_t*)&buf[3]);
        data = &buf[5];
    }

    void serialize(Serializer &ser) {
        uint8_t size = payload_size;
        if(size > MAX_SIZE) {
            size = MAX_SIZE;
        }
        ser.push(ID);
        ser.push(blob_id);
        ser.push(size);
        ser.push(chunk_index);
        for(uint32_t i=0; i<size; i++) {
            ser.push(data[i]);
        }
        ser.finish();
    }
};

struct SetGainMsg {
    static const uint8_t ID = 11;
    static const uint8_t MAX_COUNT = 128;
    static const uint8_t MAX_BYTES = (MAX_COUNT+3) / 4; // round up
    uint8_t count;
    uint8_t data[MAX_BYTES];
    SetGainMsg() : count(0), data{0} {}

    static int predictSize(uint8_t *buf, uint32_t length) {
        (void)buf;
        if(length < 2) {
            return 0;
        } else {
            return 2 + (buf[1] + 3) / 4;
        }
    }

    bool fill(uint8_t* buf, uint32_t length) {
        if(length == 0 || (int)length != predictSize(buf, length)) {
            return false;
        }

        count = buf[1];
        for(uint32_t i=0; i<count && i <MAX_COUNT; i+=4) {
            data[i/4] = buf[i/4+2];
        }
        return true;
    }

    uint8_t get_channel(uint8_t channel) {
        uint32_t offset = channel / 4;
        uint32_t shift = (channel % 4) * 2;
        return (data[offset] >> shift) & 0x3;
    }
};

struct ParameterDescriptorMsg {
    static const uint8_t ID = 12;

    uint32_t param_id;
    uint8_t defaultValue[4]; // could be int or float
    uint16_t sequence_number; // The position of this parameter in the list
    uint16_t sequence_total;  // The total number of parameters in the list

    const char *name;
    const char *description;
    const char *type; // one of: "float", "int", "bool"

    static int predictSize(uint8_t *buf, uint32_t length) {
        (void)buf;
        (void)length;
        // Received messages are always empty and indicate request for parameter descriptor dump
        return 1; 
    }

    bool fill(uint8_t *buf, uint32_t length) {
        (void)buf;
        if(length > 0) {
            return true;
        } else {
            return false;
        }
    }    

    void serialize(Serializer &ser) {
        uint32_t len;
        // Total size of name + description + type  + the two null terminators separating them
        uint16_t str_size = strlen(name) + strlen(description) + strlen(type) + 2;
        
        ser.push(ID);
        ser.push(str_size);
        ser.push(param_id);
        ser.push(*((uint32_t*)defaultValue));
        ser.push(sequence_number);
        ser.push(sequence_total);
        len = strlen(name);
        for(uint32_t i=0; i<len; i++) {
            ser.push(name[i]);
        }
        ser.push((uint8_t)0); // null separator
        len = strlen(description);
        for(uint32_t i=0; i<len; i++) {
            ser.push(description[i]);
        }
        ser.push((uint8_t)0); // null separator
        len = strlen(type);
        for(uint32_t i=0; i<len; i++) {
            ser.push(type[i]);
        }
        ser.finish();
    }
};

struct CalibrateCommandMsg {
    static const uint8_t ID = 13;

    enum CommandType : uint8_t {
        CapacitanceOffset = 0
    };
    
    uint8_t command;

    static int predictSize(uint8_t *buf, uint32_t length) {
        (void)buf;
        (void)length;
        return 2;
    }

    bool fill(uint8_t *buf, uint32_t length) {
        if(length < 2) {
            return false;
        } else {
            command = buf[1];
            return true;
        }
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
            PREDICT(BulkCapacitanceMsg)
            PREDICT(CalibrateCommandMsg)
            PREDICT(DataBlobMsg)
            PREDICT(ElectrodeEnableMsg)
            PREDICT(ParameterDescriptorMsg)
            PREDICT(ParameterMsg)
            PREDICT(SetGainMsg)
            PREDICT(SetPwmMsg)
            default:
                return -1;
        }
    }
};
