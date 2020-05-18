#include <cstdint>
#include <chrono>
#include <functional>

#include "modm/platform.hpp"
#include "AppConfig.hpp"
#include "EventEx.hpp"
#include "Events.hpp"
#include "SystemClock.hpp"

using namespace modm::platform;
using namespace std::chrono_literals;

using SPI = SpiMaster3;
using SCK = GpioC10;
using MOSI = GpioC12;
using POL = GpioB5;
using BL = GpioB4;
using LE = GpioC13;
using INT_RESET = GpioC2;
using INT_VOUT = GpioA2;
using GAIN_SEL = GpioC3;


// Cycles between capacitance scans; must be even
static const uint32_t SCAN_PERIOD = 500;
static const auto BLANKING_DELAY = 0ns;
static const auto RESET_DELAY = 0ns;
static const auto SAMPLE_DELAY = 0ns;

template<uint32_t N_CHIPS = 2>
class HV507 {
public:
    static const uint32_t N_PINS = N_CHIPS * 64;
    static const uint32_t N_BYTES = N_CHIPS * 8;

    struct SampleData {
        uint16_t sample0; // starting value
        uint16_t sample1; // final value
    };

    void init(EventEx::EventBroker *broker) {
        mBroker = broker;
        mCyclesSinceScan = 0;
        for(uint32_t i=0; i<N_BYTES; i++) {
            mShiftReg[i] = 0;
        }

        SPI::connect<SCK::Sck, MOSI::Mosi, GpioUnused::Miso>();
        SpiMaster3::initialize<SystemClock, 6000000>();
        Adc1::connect<INT_VOUT::In2>();
        INT_RESET::set();
        GAIN_SEL::reset();
    }

    // To be called by applicatin at 2x polarity frequency
    void drive();

    void setElectrodes(uint8_t electrodes[N_BYTES]) {
        for(uint32_t i=0; i<N_BYTES; i++) {
            mShiftReg[i] = electrodes[i];
        }
        mShiftRegDirty = true;
    }

private:
    uint8_t mShiftReg[N_BYTES];
    bool mShiftRegDirty = false;
    uint32_t mCyclesSinceScan;
    uint16_t mScanData[N_PINS];
    EventEx::EventBroker *mBroker;

    /* Perform capacitance scan of all electrodes*/
    void scan();
    void loadShiftRegister();
    SampleData sampleCapacitance();
};

template <uint32_t N_CHIPS>
void HV507<N_CHIPS>::drive() {
    mCyclesSinceScan++;

    if(mCyclesSinceScan == SCAN_PERIOD) {
        scan();
        events::CapScan event;
        event.measurements = mScanData;
        mBroker->publish(event);
    }

    if(mShiftRegDirty) {
        loadShiftRegister();
        mShiftRegDirty = false;
        events::ElectrodesUpdated event;
        mBroker->publish(event);
    }

    if(!POL::read()) {
        BL::reset();
        POL::set();
        modm::delay(BLANKING_DELAY);
        INT_RESET::reset();
        modm::delay(RESET_DELAY);

        SampleData sample = sampleCapacitance();
        // Send active capacitance message
        events::CapActive event(sample.sample0, sample.sample1);
        mBroker->publish(event);

    } else {
        POL::reset();
    }
}

template<uint32_t N_CHIPS>
void HV507<N_CHIPS>::scan() {
    // Clear all bits in the shift register except the first
    for(uint32_t i=0; i < N_BYTES - 1; i++) {
        SPI::transferBlocking(0);
    }
    SPI::transferBlocking(1);

    // Convert SCK and MOSI pins from alternate function to outputs
    MOSI::setOutput(0);
    SCK::setOutput(0);

    POL::reset();
    BL::reset();

    modm::delay(200us);

    for(uint32_t i=N_PINS-1; i >= 0; i--) {
        if(i == AppConfig::CommonPin()) {
            // Skip the common top plate electrode
            SCK::set();
            modm::delay(80ns);
            SCK::reset();
            continue;
        }

        SampleData sample = sampleCapacitance();
        mScanData[i] = sample.sample1 - sample.sample1;
        BL::reset();
        SCK::set();
        INT_RESET::set();
        modm::delay(80ns);
        SCK::reset();
        LE::reset();
        modm::delay(80ns);
        LE::set();
        modm::delay(4000ns);
    }

    // Restore GPIOs to alternate fucntion
    SPI::connect<SCK::Sck, MOSI::Mosi, GpioUnused::Miso>();
    // Restore shift register values
    loadShiftRegister();

}

template <uint32_t N_CHIPS>
void HV507<N_CHIPS>::loadShiftRegister() {
    SPI::transferBlocking(mShiftReg, NULL, N_BYTES);
    LE::reset();
    // Per datasheet, min LE pulse width is 80ns
    modm::delay(80ns);
    LE::set();
}

template <uint32_t N_CHIPS>
typename HV507<N_CHIPS>::SampleData HV507<N_CHIPS>::sampleCapacitance() {
    SampleData ret;
    Adc1::setChannel(Adc1::getPinChannel<INT_VOUT>(), Adc1::SampleTime::Cycles3);
    // Performed with interrupts disabled for consistent timing
    {
        modm::atomic::Lock lck;
        // Release the current integrator reset to begin measuring charge transfer
        INT_RESET::reset();
        modm::delay(RESET_DELAY);
        // Take an initial reading of integrator output -- the integrator does not
        // reset fully to 0V
        Adc1::startConversion();
        while(!Adc1::isConversionFinished());
        ret.sample0 = Adc1::getValue();
        // Release the blanking signal, and allow time for active electrodes to
        // charge and current to settle back to zero
        BL::set();
        modm::delay(SAMPLE_DELAY);
        // Read voltage integrator
        Adc1::startConversion();
        while(!Adc1::isConversionFinished());
        ret.sample1 = Adc1::getValue();
    }
    return ret;
}