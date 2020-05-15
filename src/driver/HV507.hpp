#include <cstdint>
#include <chrono>
#include <functional>

#include "modm/platform.hpp"
#include "AppConfig.hpp"
#include "MessageSender.hpp"
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

    struct ScanData {
        uint16_t pins[N_PINS];
    };

    struct SampleData {
        uint16_t sample0; // starting value
        uint16_t sample1; // final value
    };

    typedef std::function<void(ScanData)> CapScanCallback_f;
    typedef std::function<void(uint16_t, uint16_t)> CapCallback_f;
    typedef std::function<void()> UpdateCallback_f;

    void init(MessageSender *message_sender) {
        mSender = message_sender;
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

    void setCapScanCallback(CapScanCallback_f &cb) {
        mCapScanCallback = cb;
    }

    void setCapCallback(HV507<N_CHIPS>::CapCallback_f cb) {
        mCapCallback = cb;
    }

    void setUpdateCallback(UpdateCallback_f &cb) {
        mUpdateCallback = cb;
    }

    // To be called by applicatin at 2x polarity frequency
    void drive();


    /* Perform capacitance scan */
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
    ScanData mScanData;
    MessageSender *mSender;
    CapScanCallback_f mCapScanCallback;
    CapCallback_f mCapCallback;
    UpdateCallback_f mUpdateCallback;

    void scan();
    void loadShiftRegister();
    SampleData sampleCapacitance();
};

template <uint32_t N_CHIPS>
void HV507<N_CHIPS>::drive() {
    mCyclesSinceScan++;

    if(mCyclesSinceScan == SCAN_PERIOD) {
        scan();
        // TODO: Queue results for transmit
    }

    if(mShiftRegDirty) {
        loadShiftRegister();
        mShiftRegDirty = false;
        // TODO: Send CommandAck
    }

    if(!POL::read()) {
        BL::reset();
        POL::set();
        modm::delay(BLANKING_DELAY);
        INT_RESET::reset();
        modm::delay(RESET_DELAY);
        // TODO: ADC conversion
        uint16_t sample0 = 0;
        BL::set();
        modm::delay(SAMPLE_DELAY);
        // TODO: ADC conversion
        uint16_t sample1 = 0;
        INT_RESET::set();
        // Send active capacitance message

    } else {
        POL::reset();
    }
}

template<uint32_t N_CHIPS>
void HV507<N_CHIPS>::scan() {
    // Clear all bits in the shift register except the first
    for(int i=0; i < N_BYTES - 1; i++) {
        SPI::transferBlocking(0);
    }
    SPI::transferBlocking(1);

    // Convert SCK and MOSI pins from alternate function to outputs
    MOSI::setOutput(0);
    SCK::setOutput(0);

    POL::reset();
    BL::reset();

    // TODO: delay configurable time
    modm::delay(200us);

    for(int i=N_PINS-1; i >= 0; i--) {
        uint16_t sample0, sample1;
        if(i == AppConfig::CommonPin()) {
            // Skip the common top plate electrode
            SCK::set();
            // TODO: Delay 80ns
            SCK::reset();
            continue;
        }

        sampleCapacitance();
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
    Adc1::setChannel(INT_VOUT::AdcChannel<Adc1>, Adc1::SampleTime::Cycles3);
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