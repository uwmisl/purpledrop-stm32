#include "SystemClock.hpp"
#include "TempSensors.hpp"

using namespace modm::platform;

using SPI = SpiMaster2;
using SCK = GpioC7;
using MOSI = GpioB15;
using MISO = GpioB14;
using CS0 = GpioB13;
using CS1 = GpioB12;
using CS2 = GpioB10;
using CS3 = GpioB1;

static Max31865<SPI, CS0> Sensor0;
static Max31865<SPI, CS1> Sensor1;
static Max31865<SPI, CS2> Sensor2;
static Max31865<SPI, CS3> Sensor3;

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
    SPI::setDataMode(SPI::DataMode::Mode1);
    for(uint32_t i=0; i<AppConfig::N_TEMP_SENSOR; i++) {
        mDrivers[i]->init(RESIST_REF);
    }
}

void TempSensors::poll() {
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