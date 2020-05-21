#include "Analog.hpp"
#include "SystemClock.hpp"


#include "modm/platform.hpp"

using INT_VOUT = modm::platform::GpioA2;
using VHV_FB_P = modm::platform::GpioA0;
using VHV_FB_N = modm::platform::GpioA1;

using namespace modm::platform;

void Analog::init() {
    // Setup ADC
    Adc1::connect<INT_VOUT::In2>();
    Adc1::connect<VHV_FB_P::In0>();
    Adc1::connect<VHV_FB_N::In1>();
    Adc1::initialize<SystemClock, 24_MHz>();
}

void Analog::setupIntVout() {
    Adc1::setChannel(Adc1::getPinChannel<INT_VOUT>(), Adc1::SampleTime::Cycles3);
}

uint16_t Analog::readIntVout() {
    Adc1::startConversion();
    while(!Adc1::isConversionFinished());
    return Adc1::getValue();
}

int16_t Analog::readVhvDiff() {
    int16_t p_read, n_read;

    modm::platform::Adc1::setSampleTime(Adc1::getPinChannel<VHV_FB_P>(), Adc1::SampleTime::Cycles3);
    modm::platform::Adc1::setSampleTime(Adc1::getPinChannel<VHV_FB_N>(), Adc1::SampleTime::Cycles3);
    p_read = Adc1::readChannel(Adc1::getPinChannel<VHV_FB_P>());
    n_read = Adc1::readChannel(Adc1::getPinChannel<VHV_FB_N>());
    return p_read - n_read;
}