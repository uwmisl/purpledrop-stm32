#pragma once
#include <cstdint>
#include <cstdlib>

#include "CircularBuffer.hpp"

struct Checksum {
    Checksum() : a(0), b(0) {}

    void push(uint8_t x) {
        a += x;
        b += a;
    }

    void reset() {
        a = 0;
        b = 0;
    }

    uint8_t a;
    uint8_t b;

    static Checksum compute_over(uint8_t *buf, uint32_t length) {
        Checksum cs;
        for(uint32_t i=0; i<length; i++) {
            cs.push(buf[i]);
        }
        return cs;
    }
};

/* Parses framed messages to provide unescaped payloads
*/
template<typename TParser, uint32_t MAX_SIZE = 256>
struct MessageFramer {

    MessageFramer() {
        mEscaping = false;
        mParsing = false;
        mCount = 0;
    }

    bool push(uint8_t b, uint8_t *&msg, uint16_t &length) {
        if(mEscaping) {
            b = b ^ 0x20;
            mEscaping = false;
        } else if(b == 0x7d) {
            mEscaping = true;
            return false;
        } else if(b == 0x7e) {
            // start of frame
            reset();
            mParsing = true;
            return false;
        }

        mBuf[mCount] = b;
        mCount++;

        int expected_size = TParser::predictSize(mBuf, mCount);
        if(expected_size == -1) {
            // It's not a valid message
            reset();
        } else if(expected_size > 0 && mCount >= expected_size + 2) {
            Checksum calc = Checksum::compute_over(mBuf, mCount - 2);
            if(calc.a == mBuf[mCount - 2] && calc.b == mBuf[mCount - 1]) {
                msg = mBuf;
                length = mCount - 2;
                reset();
                return true;
            } else {
                reset();
            }
        }
        msg = mBuf;
        length = mCount - 2;
        return false; // No message found
    }

    void reset() {
        mParsing = false;
        mEscaping = false;
        mCount = 0;
    }
private:
    bool mEscaping;
    bool mParsing;
    uint8_t mCount;
    uint8_t mBuf[MAX_SIZE];
};

struct Serializer{
    Serializer() : Serializer(nullptr) {}

    Serializer(IProducer<uint8_t> *sink) : mStarted(false), mSink(sink)  {}

    void setSink(IProducer<uint8_t>  *sink) { mSink = sink; };

    void push(uint8_t b, bool last = false)  {
        if(!mSink) {
            return;
        }
        if(!mStarted) {
            mSink->push(0x7e); // preface with start of frame code
            mStarted = true;
        }
        mCs.push(b);
        if(b == 0x7d || b == 0x7e) {
            mSink->push(0x7d);
            mSink->push(b ^ 0x20);
        } else {
            mSink->push(b);
        }
        if(last) {
            finish();
        }
    }

    template<typename T>
    void push(T value, bool last = false) {
        uint8_t *p = (uint8_t*)&value;
        for(uint32_t i=0; i<sizeof(value); i++) {
            push(p[i]);
        }
        if(last) {
            finish();
        }
    }

    void finish() {
        // Send the CRC
        mSink->push(mCs.a);
        mSink->push(mCs.b);
        // Setup to be re-used on a new packet
        mStarted = false;
        mCs.reset();
    }

private:
    bool mStarted;
    Checksum mCs;
    IProducer<uint8_t> *mSink;
};