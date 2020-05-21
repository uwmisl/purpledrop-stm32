#include "SystemClock.hpp"
#include "TempSensors.hpp"

using namespace modm::platform;

using SPI = SpiMaster2;
using SCK = GpioC7;
using MOSI = GpioB15;
using MISO = GpioB14;
using CS0 = GpioC0;
using CS1 = GpioC0;
using CS2 = GpioC0;
using CS3 = GpioC0;

static Max31865<SPI, GpioB13> Sensor0;
static Max31865<SPI, GpioB12> Sensor1;
static Max31865<SPI, GpioB10> Sensor2;
static Max31865<SPI, GpioB1> Sensor3;

static const float RESIST_REF = 4000.0;

IMax31865 *TempSensors::mDrivers[] = {
    &Sensor0,
    &Sensor1,
    &Sensor2,
    &Sensor3
};
uint16_t TempSensors::mReadings[AppConfig::N_TEMP_SENSOR] = {0};

void TempSensors::init(EventEx::EventBroker *broker) {
    mBroker = broker;
    SPI::initialize<SystemClock, 3_MHz>();
    SPI::connect<SCK::Sck, MOSI::Mosi, MISO::Miso>();
    for(uint32_t i=0; i<AppConfig::N_TEMP_SENSOR; i++) {
        mDrivers[i]->init(RESIST_REF);
    }
}

void TempSensors::poll() {
    if(mTimer.poll()) {
        events::TemperatureMeasurement event;
        for(uint32_t i=0; i<AppConfig::N_TEMP_SENSOR; i++) {
            mReadings[i] = mDrivers[i]->read_temperature();
            event.measurements[i] = mReadings[i];
        }

        mBroker->publish(event);
    }
}