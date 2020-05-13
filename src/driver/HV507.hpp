#include <cstdint>
#include "modm/platform.hpp"
#include "AppConfig.hpp"

// template<
//     class SPI, 
//     class SCK, 
//     class MOSI, 
//     class POL, 
//     class LE, 
//     class BL,
//     class INT_RESET,
//     uint32_t N_CHIPS = 2>

using namespace modm::platform;

using SPI = SpiMaster3;
using SCK = GpioC10;
using MOSI = GpioC12;
using POL = GpioB5;
using BL = GpioB4;
using LE = GpioC13;

// Cycles between capacitance scans; must be even
static const uint32_t SCAN_PERIOD = 500;

template<uint32_t N_CHIPS = 2>
struct HV507 {
    void init() {
        mCyclesSinceScan = 0;
        for(int i=0; i<N_CHIPS * 8; i++) {
            mShiftReg = 0;
        }
    }
    void drive() {
        mCyclesSinceScan++;

    }

    /* Perform capacitance scan */
    void scan() {
        // Clear all bits in the shift register except the first
        for(int i=0; i < N_CHIPS * 8 - 1; i++) {
            SPI::transferBlocking(0);
        }
        SPI::transferBlocking(1);

        // Convert SCK and MOSI pins from alternate function to outputs
        MOSI::setOutput(0);
        SCK::setOutput(0);

        POL::setLow();
        BL::setLow();

        // TODO: delay configurable time

        for(int i=0; i < N_CHIPS * 8; i++) {
            if(i == AppConfig::CommonPin()) { 
                // Skip the common top plate electrode
                SCK::setHigh();
                // TODO: Delay 80ns
                SCK::setLow();
                continue;
            }

            INT_RESET::setLow();

            
        }
    }   
private:
    uint8_t mShiftReg[N_CHIPS * 8];
    uint32_t mCyclesSinceScan;
};