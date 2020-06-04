#include <vector>
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

std::vector<uint8_t>
serialize_message(
    uint8_t* buf, 
    uint32_t length,
    bool wrong_crc = false) 
{
    Checksum cs;
    std::vector<uint8_t> ret;
    ret.push_back(0x7e);
    for(uint32_t i=0; i<length; i++) {
        cs.push(buf[i]);
        if(buf[i] == 0x7d || buf[i] == 0x7e) {
            // Escape control characters
            ret.push_back(0x7d);
            ret.push_back(buf[i] ^ 0x20);
        } else {
            ret.push_back(buf[i]);
        }
    }
    if(wrong_crc) {
        cs.a += 1;
    }
    ret.push_back(cs.a);
    ret.push_back(cs.b);

    return ret;
}

struct FramerTest : public ::testing::Test {
    void SetUp() {}

    MessageFramer<MockParser> framer;
};

TEST_F(FramerTest, ignore_bad_crc) {
    uint8_t message[16];
    for(int i=0; i < 16; i++) {
        message[i] = i+1;
    }
    auto tx_bytes = serialize_message(message, sizeof(message), true);
    uint8_t *returnBuf;
    uint16_t returnLength;
    bool msgFound;
    for(int i=0; i < tx_bytes.size(); i++) {
        msgFound = framer.push(tx_bytes[i], returnBuf, returnLength);
    }
    ASSERT_FALSE(msgFound);
}

TEST_F(FramerTest, parse_proper_message) {
    uint8_t message[16];
    for(int i=0; i < 16; i++) {
        message[i] = i+1;
    }
    auto tx_bytes = serialize_message(message, sizeof(message));
    uint8_t *returnBuf;
    uint16_t returnLength;
    bool msgFound;
    for(int i=0; i < tx_bytes.size(); i++) {
        msgFound = framer.push(tx_bytes[i], returnBuf, returnLength);
    }
    ASSERT_TRUE(msgFound);
    ASSERT_EQ(returnLength, 16);
    ASSERT_NE(returnBuf, (uint8_t*)NULL);
    for(int i=0; i<16; i++) {
        ASSERT_EQ(returnBuf[i], i+1);
    }
}

