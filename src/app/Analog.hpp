#pragma once

#include <cstdint>

struct Analog {
    void init();
    
    /** Setup ADC to read IntVout() 
    Separated from the read to allow tighter timing of repeated reads
    */
    void setupIntVout();

    /** Sample the INT_VOUT pin 
    setIntVout must be called first
    */
    uint16_t readIntVout();

    /** Return VHV_FB_P - VHV_FB_N */
    int16_t readVhvDiff();
};