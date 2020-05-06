#pragma once

#include <cstdlib>
#include "SpiBus.hpp"
#include "Gpio.hpp"

struct SpiDevice {
    SpiDevice() : mBus(NULL), mCs(NULL) {}

    void init(SpiBus *bus, GpioPin *cs) {
        mBus = bus;
        mCs = cs;
    }

private: 
    SpiBus *mBus;
    GpioPin *mCs;
}