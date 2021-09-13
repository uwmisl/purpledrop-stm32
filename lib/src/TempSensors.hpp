#pragma once
#include "modm/platform.hpp"
#include "AppConfig.hpp"
#include "Events.hpp"
#include "Max31865.hpp"
#include "PeriodicPollingTimer.hpp"


template <typename PINS, class SPI>
struct TempSensors {
    TempSensors() : mTimer(AppConfig::TEMP_READ_PERIOD) {}

    template<typename SystemClock>
    void init(EventEx::EventBroker *broker);

    void poll();

    static float getLatest(uint8_t ch);

private:
    EventEx::EventBroker *mBroker;
    PeriodicPollingTimer mTimer;
    static uint16_t mReadings[AppConfig::N_TEMP_SENSOR];
    //static IMax31865 *mDrivers[AppConfig::N_TEMP_SENSOR];

    static Max31865<SPI, typename PINS::CS0> Sensor0;
    static Max31865<SPI, typename PINS::CS1> Sensor1;
    static Max31865<SPI, typename PINS::CS2> Sensor2;
    static Max31865<SPI, typename PINS::CS3> Sensor3;

    static constexpr IMax31865 *mDrivers[] = {
        &Sensor0,
        &Sensor1,
        &Sensor2,
        &Sensor3
    };

    static constexpr float RESIST_REF = 4000.0;

    
};

template<typename PINS, class SPI>
uint16_t TempSensors<PINS, SPI>::mReadings[AppConfig::N_TEMP_SENSOR] = {0};

template<typename PINS, class SPI>
Max31865<SPI, typename PINS::CS0> TempSensors<PINS, SPI>::Sensor0;
template<typename PINS, class SPI>
Max31865<SPI, typename PINS::CS1> TempSensors<PINS, SPI>::Sensor1;
template<typename PINS, class SPI>
Max31865<SPI, typename PINS::CS2> TempSensors<PINS, SPI>::Sensor2;
template<typename PINS, class SPI>
Max31865<SPI, typename PINS::CS3> TempSensors<PINS, SPI>::Sensor3;
// IMax31865 *TempSensors::mDrivers[] = {
//     &Sensor0,
//     &Sensor1,
//     &Sensor2,
//     &Sensor3
// };


template <typename PINS, class SPI>
void TempSensors<PINS, SPI>::poll() {
    if(mTimer.poll()) {
        events::TemperatureMeasurement event;
        for(uint32_t i=0; i<AppConfig::N_TEMP_SENSOR; i++) {
            mReadings[i] = mDrivers[i]->read_temperature() * 100;
            event.measurements[i] = mReadings[i];
            uint8_t faults = mDrivers[i]->fault_flags();
            if(faults != 0) {
                printf("Fault %x on sensor %ld\n", faults, i);
            }
        }

        mBroker->publish(event);
    }
}

template<typename PINS, class SPI> 
template<typename SystemClock>
void TempSensors<PINS, SPI>::init(EventEx::EventBroker *broker) {
    mBroker = broker;
    SPI::template initialize<SystemClock, 3_MHz>();
    SPI::setDataMode(SPI::DataMode::Mode1);
    for(uint32_t i=0; i<AppConfig::N_TEMP_SENSOR; i++) {
        mDrivers[i]->init(RESIST_REF);
    }
}