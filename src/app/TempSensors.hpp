#pragma once
#include "modm/platform.hpp"
#include "AppConfig.hpp"
#include "Events.hpp"
#include "Max31865.hpp"
#include "PeriodicPollingTimer.hpp"


struct TempSensors {
    TempSensors() : mTimer(AppConfig::TEMP_READ_PERIOD) {}

    void init(EventEx::EventBroker *broker);
    void poll();
    static float getLatest(uint8_t ch);

private:
    EventEx::EventBroker *mBroker;
    PeriodicPollingTimer mTimer;
    static uint16_t mReadings[AppConfig::N_TEMP_SENSOR];
    static IMax31865 *mDrivers[AppConfig::N_TEMP_SENSOR];

};