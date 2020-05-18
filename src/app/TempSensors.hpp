#pragma once
#include "modm/platform.hpp"
#include "AppConfig.hpp"
#include "Max31865.hpp"
#include "PeriodicPollingTimer.hpp"


struct TempSensors {
    static void poll();
    static float getLatest(uint8_t ch);
    
private:
    static PeriodicPollingTimer mTimer;
    static IMax31865 *mDrivers[AppConfig::N_TEMP_SENSOR];  
    static uint16_t mReadings[AppConfig::N_TEMP_SENSOR];
};