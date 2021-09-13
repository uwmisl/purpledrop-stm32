#pragma once

#include <cstdint>

template<
    typename AdcDev,
    typename INT_VOUT,
    typename ISENSE,
    typename VHV_FB_P,
    typename VHV_FB_N>
struct Analog {
    /** Setup ADC to read IntVout()
    Separated from the read to allow tighter timing of repeated reads
    */
    static void setupIntVout() {
        AdcDev::setChannel(AdcDev::template getPinChannel<INT_VOUT>()); //, AdcDev::SampleTime::Cycles3);
        // Empirically, this is required -- on the samg anyway -- to get good samples when changing channels
        readIntVout();
    }

    /** Sample the INT_VOUT pin
    setIntVout must be called first
    */
    static uint16_t readIntVout() {
        AdcDev::startConversion();
        while(!AdcDev::isConversionFinished());
        return AdcDev::getValue();
    }

    /** Return VHV_FB_P - VHV_FB_N */
    static inline int16_t readVhvDiff() {
        int16_t p_read, n_read;
        // Disable interrupts because readIntVout may be called in IRQ
        modm::atomic::Lock lck;

        p_read = AdcDev::readChannel(AdcDev::template getPinChannel<VHV_FB_P>());
        n_read = AdcDev::readChannel(AdcDev::template getPinChannel<VHV_FB_N>());
        return p_read - n_read;
    }
};
