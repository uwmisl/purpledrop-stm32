#include "TempSensors.hpp"

using namespace modm::platform;

using SPI = SpiMaster2;
using CS0 = GpioC0;
using CS1 = GpioC0;
using CS2 = GpioC0;
using CS3 = GpioC0;

static Max31865<SPI, GpioB13> Sensor0;
static Max31865<SPI, GpioB12> Sensor1;
static Max31865<SPI, GpioB10> Sensor2;
static Max31865<SPI, GpioB1> Sensor3;

IMax31865 *TempSensors::mDrivers[] = {
    &Sensor0,
    &Sensor1,
    &Sensor2,
    &Sensor3
};

PeriodicPollingTimer TempSensors::mTimer = PeriodicPollingTimer(AppConfig::TEMP_READ_PERIOD);
uint16_t TempSensors::mReadings[AppConfig::N_TEMP_SENSOR] = {0};

void TempSensors::poll() {
    if(mTimer.poll()) {
        for(uint32_t i=0; i<AppConfig::N_TEMP_SENSOR; i++) {
            mReadings[i] = mDrivers[i]->read_temperature();
        }
    }
}