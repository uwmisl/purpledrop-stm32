#include "gtest/gtest.h"
#include "MessageFramer.hpp"

static const int Msg1Size = 16;
struct MockParser {
    static int predictSize(uint8_t *buf, uint32_t length) {
        if(length < 1) {
            return 0;
        } else if(buf[0] == 1) {
            return Msg1Size;
        } else {
            return -1;
        }
    }
};

template<typename T>
void serialize_message(
    uint8_t* buf, 
    uint32_t length, 
    MessageFramer<T> &framer, 
    bool wrong_crc = false) 
{
    Checksum cs;
    framer.push(0x7e);
    for(uint32_t i=0; i<length; i++) {
        cs.push(buf[i]);
        if(buf[i] == 0x7d || buf[i] == 0x7e) {
            // Escape control characters
            framer.push(0x7d);
            framer.push(buf[i] ^ 0x20);
        } else {
            framer.push(buf[i]);
        }
    }
    if(wrong_crc) {
        cs.a += 1;
    }
    framer.push(cs.a);
    framer.push(cs.b);
}

struct FramerTest : public ::testing::Test {
    void SetUp() {
        returnBuf = NULL;
        returnLength = 0;
        framer.registerMessageCallback(
        [](uint8_t *buf, uint32_t len) { 
            returnBuf = buf; 
            returnLength = len; 
        }
    );
    }

    MessageFramer<MockParser> framer;
    static uint8_t *returnBuf;
    static uint32_t returnLength;
};
uint8_t *FramerTest::returnBuf = NULL;
uint32_t FramerTest::returnLength = 0;

TEST_F(FramerTest, Happy1) {
    uint8_t message[16];
    for(int i=0; i < 16; i++) {
        message[i] = i+1;
    }
    serialize_message(message, sizeof(message), framer, true);

    ASSERT_EQ(returnLength, 0);
    ASSERT_EQ(returnBuf, (uint8_t*)NULL);
}

TEST_F(FramerTest, BadCrc) {
    uint8_t message[16];
    for(int i=0; i < 16; i++) {
        message[i] = i+1;
    }
    serialize_message(message, sizeof(message), framer);

    ASSERT_EQ(returnLength, 16);
    ASSERT_NE(returnBuf, (uint8_t*)NULL);
    for(int i=0; i<16; i++) {
        ASSERT_EQ(returnBuf[i], i+1);
    }
}

