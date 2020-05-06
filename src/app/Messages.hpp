#pragma once

#include <cstdint>
#include <cstring>
#include "MessageFramer.hpp"

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
        return 16;
    }
    
    uint8_t values[16];
};


struct BulkCapacitanceMsg {
    static const uint8_t ID = 2;
    static const uint8_t MAX_VALUES = 64;

    BulkCapacitanceMsg() : count(0), start_index(0) {}

    static int predictSize(uint8_t *buf, uint32_t length) {
        if(length < 3) {
            return 0;
        } else {
            return buf[2] * 2 + 3;
        }
    }

    bool fill(uint8_t *buf, uint32_t length) {
        if(length == 0 || length != predictSize(buf, length)) {
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

// Message sent to device to set (writeFlag = 1) or request (writeFlag = 0)
// the value of a device. The same message is returned from the device in 
// response with the new value -- response is sent for both writes and 
// requests, so that it serves as an ACK. 
// The parameter value may be a 4-byte integer or float, depending on the 
// parameter. 
struct ParameterMsg {
    static const uint8_t ID = 6;

    static int predictSize(uint8_t *buf, uint32_t length) {
        return 10;
    }

    void fill(uint8_t *buf, uint32_t length) {
        paramIdx = *((uint32_t*)&buf[1]);
        paramValue.s32 = *((int32_t*)&buf[5]);
        writeFlag = buf[9];
    }

    void serialize(Serializer &s) {
        uint8_t *p;
        s.push(ID);
        s.push(paramIdx);
        s.push(paramValue.s32);
        s.push(writeFlag);

        p = (uint8_t *)&paramIdx;
        
        s.finish();
    }

    uint32_t paramIdx;
    union {
        float f32;
        int32_t s32;
    } paramValue;
    // If set, this message is setting a param. If clear, this is requesting the current value.
    uint8_t writeFlag;
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
            // case ElectrodeEnableMsg::ID:
            //     return ElectrodeEnableMsg::predictSize(buf, length); 
            // case BulkCapacitanceMsg::ID:
            //     return BulkCapacitanceMsg::predictSize(buf, length);
            default:
                return -1;
        }
    }    
};
