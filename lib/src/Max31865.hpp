#pragma once
#include <cstdint>
#include <cstdlib>
#include <cmath>

using namespace std::chrono_literals;

struct IMax31865 {
    virtual void init(float resist_ref) = 0;
    virtual float read_resistance() = 0;
    virtual float read_temperature() = 0;
    virtual uint8_t fault_flags() = 0;
};

template<class SPI, class CS> 
struct Max31865 : IMax31865 {
    enum Regs {
        Configuration = 0,
        RtdHigh = 1,
        RtdLow = 2,
        HighFaultThresholdMsb = 3,
        HighFaultThresholdLsb = 4,
        LowFaultThresholdMsb = 5,
        LowFaultThresholdLsb = 6,
        FaultStatus = 7
    };

    enum ConfigBits {
        /// 1 = 50Hz, 0 = 60Hz
        Filter50Hz           = 0b00000001,
        /// 1 = clear (auto-clear)
        FaultStatusClear     = 0b00000010,
        /// See table 3
        FaultDetectionCycle2 = 0b00000100,
        /// See table 3
        FaultDetectionCycle3 = 0b00001000,
        /// 1 = 3-wire, 0 = 2-wire or 4-wire
        ThreeWire            = 0b00010000,
        /// 1 = one-shot (auto-clear)
        OneShot              = 0b00100000,
        /// 1 = auto, 0 = normally off
        ConversionMode       = 0b01000000,
        /// 1 = on, 0 = off
        VBias                = 0b10000000,
    };

    static constexpr uint8_t configRegValue() {
        return VBias | ConversionMode;
    }
    
    void writeByte(uint8_t addr, uint8_t value) {
        uint8_t txBuf[2];
        txBuf[0] = addr | (1<<7);
        txBuf[1] = value;
        transfer(txBuf, NULL, 2);
    }

    uint8_t readByte(uint8_t addr) {
        uint8_t txBuf[2];
        uint8_t rxBuf[2];
        txBuf[0] = addr;
        txBuf[1] = 0;
        transfer(txBuf, rxBuf, 2);
        return rxBuf[1];
    }

    void init(float resist_ref) {
        mResistRef = resist_ref;
        static const uint16_t LOW_THRESH = 0;
        static const uint16_t HIGH_THRESH = 0xfffe;

        CS::setOutput(true);
        writeByte(Configuration, configRegValue());
        writeByte(LowFaultThresholdMsb, (LOW_THRESH >> 8));
        writeByte(LowFaultThresholdLsb, LOW_THRESH & 0xff);
        writeByte(HighFaultThresholdMsb, (HIGH_THRESH >> 8));
        writeByte(HighFaultThresholdLsb, HIGH_THRESH & 0xff);
    }

    float read_resistance() {
        uint8_t txBuf[3];
        uint8_t rxBuf[3];
        txBuf[0] = RtdHigh;

        transfer(txBuf, rxBuf, 3);
        bool fault = (rxBuf[2] & 0x1) != 0;
        float resistance = 0.0;
        if(!fault) {
            mFaultFlags = 0;
        } else {
            mFaultFlags = readByte(FaultStatus);
            writeByte(Configuration, configRegValue() | FaultStatusClear);
        }
        uint16_t rawResistance = ((uint16_t)rxBuf[1] << 7) + (rxBuf[2] >> 1);
        resistance = (float)rawResistance * mResistRef / 32768;
        return resistance;
    }

    float read_temperature() { 
        static const float rtd_a = -412.6;
        static const float rtd_b = 140.41;
        static const float rtd_c = 0.00764;
        static const float rtd_d = -6.25e-17;
        static const float rtd_e = -1.25e-24;
        float res = read_resistance();

        return rtd_a \
            + rtd_b * sqrtf(1.0 + rtd_c * res) \
            + rtd_d * powf(res, 5.0) \
            + rtd_e * powf(res, 7);
    }

    uint8_t fault_flags() {
        return mFaultFlags;
    }

private:
    float mResistRef;
    uint8_t mFaultFlags;
    void transfer(uint8_t *tx_buf, uint8_t *rx_buf, uint16_t length) {
        CS::setOutput(false);
        modm::delay(400ns);
        SPI::transferBlocking(tx_buf, rx_buf, length);
        CS::setOutput(true);
    }

};
