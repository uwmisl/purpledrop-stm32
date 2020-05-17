#include "gtest/gtest.h"
#include "MessageFramer.hpp"
#include "Messages.hpp"

struct MessagesTest : public ::testing::Test {
    void SetUp() {
        returnBuf = NULL;
        returnLength = 0;
        serializer.setSink(&fifo);
    }

    void parseData() {
        while(!fifo.empty()) {
            framer.push(fifo.pop(), returnBuf, returnLength);
        }
    }
    MessageFramer<Messages> framer;
    StaticCircularBuffer<uint8_t, 128> fifo;
    Serializer serializer;
    static uint8_t *returnBuf;
    static uint16_t returnLength;
};
uint8_t *MessagesTest::returnBuf = NULL;
uint16_t MessagesTest::returnLength = 0;

TEST_F(MessagesTest, BulkCapacitanceRoundTrip) {
    BulkCapacitanceMsg msg;
    msg.start_index = 11;
    msg.count = 5;
    for(int i=0; i<msg.count; i++) {
        msg.values[i] = i * 3;
    }

    msg.serialize(serializer);
    parseData();
    
    ASSERT_NE(returnLength, 0);
    ASSERT_NE(returnBuf, (uint8_t*)NULL);

    BulkCapacitanceMsg rxMsg;
    rxMsg.fill(returnBuf, returnLength);
    ASSERT_EQ(rxMsg.start_index, msg.start_index);
    ASSERT_EQ(rxMsg.count, msg.count);
    for(int i=0; i<msg.count; i++) {
        ASSERT_EQ(rxMsg.values[i], msg.values[i]);
    }
}

TEST_F(MessagesTest, ParameterMsgRoundTrip) {
    ParameterMsg msg;
    msg.paramIdx = 11;
    msg.paramValue.f32 = 5.1;
    msg.writeFlag = 1;
    
    msg.serialize(serializer);
    parseData();
    
    ASSERT_NE(returnLength, 0);
    ASSERT_NE(returnBuf, (uint8_t*)NULL);

    ParameterMsg rxMsg;
    rxMsg.fill(returnBuf, returnLength);
    ASSERT_EQ(rxMsg.paramIdx, msg.paramIdx);
    ASSERT_EQ(rxMsg.paramValue.f32, msg.paramValue.f32);
}
