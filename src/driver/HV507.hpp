#include <cstdint>
#include <chrono>
#include <functional>

#include "modm/platform.hpp"
#include "Analog.hpp"
#include "AppConfig.hpp"
#include "EventEx.hpp"
#include "Events.hpp"
#include "SystemClock.hpp"

using namespace modm::platform;
using namespace modm::literals;
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
// Debug IO, useful for syncing scope capacitance scan
using SCAN_SYNC = GpioC1;
using AUGMENT_ENABLE = GpioA3;

// Cycles between capacitance scans; must be even
static const uint32_t SCAN_PERIOD = 500;
// static const auto BLANKING_DELAY = 14us;
// static const auto RESET_DELAY = 1000ns;
// static const auto SAMPLE_DELAY = 10us;

enum CalibrateStep : uint8_t {
    CALSTEP_NONE = 0,
    CALSTEP_REQUEST = 2,
    CALSTEP_SETTLE = 3
};

template<uint32_t N_CHIPS = 2>
class HV507 {
public:
    static const uint32_t N_PINS = N_CHIPS * 64;
    static const uint32_t N_BYTES = N_CHIPS * 8;

    enum GainSetting {
        HIGH = 0,
        LOW = 1
    };

    struct SampleData {
        uint16_t sample0; // starting value
        uint16_t sample1; // final value
    };

    void init(EventEx::EventBroker *broker, Analog *analog) {
        mBroker = broker;
        mAnalog = analog;
        mCyclesSinceScan = 0;
        mCalibrateStep = CALSTEP_NONE;
        for(uint32_t i=0; i<N_BYTES; i++) {
            mShiftReg[i] = 0;
            mLowGainFlags[i] = 0;
        }

        SPI::connect<SCK::Sck, MOSI::Mosi, GpioUnused::Miso>();
        SPI::initialize<SystemClock, 6000000>();
        SPI::setDataOrder(SPI::DataOrder::LsbFirst);
        INT_RESET::setOutput(true);
        GAIN_SEL::setOutput(false);
        AUGMENT_ENABLE::setOutput(false);
        POL::setOutput(false);
        BL::setOutput(false);
        LE::setOutput(true);
        POL::configure(Gpio::OutputType::PushPull);
        BL::configure(Gpio::OutputType::PushPull);
        LE::configure(Gpio::OutputType::PushPull);
        INT_RESET::configure(Gpio::OutputType::PushPull);
        GAIN_SEL::configure(Gpio::OutputType::PushPull);

        calibrateOffset();

        mSetElectrodesHandler.setFunction([this](auto &e) { setElectrodes(e.values); });
        mBroker->registerHandler(&mSetElectrodesHandler);
        mSetGainHandler.setFunction([this](auto &e) { handleSetGain(e); });
        mBroker->registerHandler(&mSetGainHandler);
        mCapOffsetCalibrationRequestHandler.setFunction([this](auto &) { this->mCalibrateStep = CALSTEP_REQUEST; });
        mBroker->registerHandler(&mCapOffsetCalibrationRequestHandler);
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
    uint16_t mOffsetCalibration;
    uint16_t mOffsetCalibrationLowGain;
    uint8_t mLowGainFlags[N_BYTES];
    uint8_t mCalibrateStep;
    EventEx::EventHandlerFunction<events::SetElectrodes> mSetElectrodesHandler;
    EventEx::EventHandlerFunction<events::SetGain> mSetGainHandler;
    EventEx::EventHandlerFunction<events::CapOffsetCalibrationRequest> mCapOffsetCalibrationRequestHandler;
    EventEx::EventBroker *mBroker;
    Analog *mAnalog;

    /* Perform capacitance scan of all electrodes*/
    void scan();
    void loadShiftRegister();
    void calibrateOffset();
    SampleData sampleCapacitance(uint32_t sample_delay_ns, bool fire_sync_pulse);
    void handleSetGain(events::SetGain &e);
    GainSetting getGain(uint32_t channel);
    void setPolarity(bool pol);
};

template <uint32_t N_CHIPS>
void HV507<N_CHIPS>::drive() {
    mCyclesSinceScan++;

    if(mCalibrateStep == CALSTEP_REQUEST) {
        // When requested, setup the correct polarity, then allow a cycle to stabilize
        setPolarity(true);
        BL::setOutput(false);
        mCalibrateStep = CALSTEP_SETTLE;
        return;
    } else if(mCalibrateStep == CALSTEP_SETTLE) {
        // Previouse cycle we did setup, now measure the offset
        calibrateOffset();
        mCalibrateStep = CALSTEP_NONE;
    }

    if(mCyclesSinceScan >= SCAN_PERIOD) {
        mCyclesSinceScan = 0;
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
        BL::setOutput(false);
        setPolarity(true);
        modm::delay(std::chrono::nanoseconds(AppConfig::BlankingDelay()));
        // Scan sync pin -1 causes sync pulse on active capacitance measurement
        bool fire_sync_pulse = AppConfig::ScanSyncPin() == -1;
        SampleData sample = sampleCapacitance(AppConfig::SampleDelay(), fire_sync_pulse);
        // Send active capacitance message
        events::CapActive event(sample.sample0 + mOffsetCalibration, sample.sample1);
        mBroker->publish(event);
    } else {
        setPolarity(false);
    }
}

template<uint32_t N_CHIPS>
void HV507<N_CHIPS>::scan() {
    uint32_t sample_delay;
    uint16_t offset_calibration;
    // Clear all bits in the shift register except the first
    for(uint32_t i=0; i < N_BYTES - 1; i++) {
        SPI::transferBlocking(0);
    }
    SPI::transferBlocking(0x80);

    // Convert SCK and MOSI pins from alternate function to outputs
    MOSI::setOutput(0);
    SCK::setOutput(0);

    setPolarity(true);
    BL::setOutput(false);

    modm::delay(std::chrono::nanoseconds(AppConfig::ScanStartDelay()));

    for(int32_t i=N_PINS-1; i >= 0; i--) {
        if(getGain(i) == GainSetting::LOW) {
            sample_delay = AppConfig::SampleDelayLowGain();
            offset_calibration = mOffsetCalibrationLowGain;
            GAIN_SEL::setOutput(true);
        } else {
            sample_delay = AppConfig::SampleDelay();
            offset_calibration = mOffsetCalibration;
            GAIN_SEL::setOutput(false);
        }
        // Skip the common top plate electrode
        if(i == (int)AppConfig::TopPlatePin()) {
            SCK::setOutput(true);
            modm::delay(80ns);
            SCK::setOutput(false);
            continue;
        }
        LE::setOutput(false);
        modm::delay(80ns);
        LE::setOutput(true);
        modm::delay(std::chrono::nanoseconds(AppConfig::ScanBlankDelay()));
        
        // Assert sync pulse on the requested pin for scope triggering
        bool fire_sync_pulse = i == AppConfig::ScanSyncPin();
        SampleData sample = sampleCapacitance(sample_delay, fire_sync_pulse);
        SCAN_SYNC::setOutput(false);
        mScanData[i] = sample.sample1 - sample.sample0 - offset_calibration;
        // It can go negative an overflow; clip it to zero when that happens
        if(mScanData[i] > 32767) {
            mScanData[i] = 0;
        }

        BL::setOutput(false);
        SCK::setOutput(true);
        modm::delay(80ns);
        SCK::setOutput(false);
    }

    // Restore gain setting to normally high gain
    GAIN_SEL::setOutput(false);
    // Restore blank signal to inactive state
    BL::setOutput(true);
    // Restore GPIOs to alternate fucntion
    SPI::connect<SCK::Sck, MOSI::Mosi, GpioUnused::Miso>();
    // Restore shift register values
    loadShiftRegister();
}

template <uint32_t N_CHIPS>
void HV507<N_CHIPS>::loadShiftRegister() {
    SPI::transferBlocking(mShiftReg, 0, N_BYTES);
    LE::setOutput(false);
    // Per datasheet, min LE pulse width is 80ns
    modm::delay(80ns);
    LE::setOutput(true);
}

template <uint32_t N_CHIPS>
void HV507<N_CHIPS>::calibrateOffset() {
    static const uint32_t nSample = 100;
    uint32_t accum = 0;

    /* Offset calibration is calibrated for both normal and low gain because 
    a different sample delay is used. The offset is essentially proportional
    to the sample delay, so one could probably be calculated from the other,
    but it's also pretty quick to just measure both */
    for(uint32_t i=0; i<nSample; i++) {
        auto sample = sampleCapacitance(AppConfig::SampleDelay(), false);
        accum += sample.sample1 - sample.sample0;
    }
    mOffsetCalibration = accum / nSample;

    accum = 0;
    for(uint32_t i=0; i<nSample; i++) {
        auto sample = sampleCapacitance(AppConfig::SampleDelayLowGain(), false);
        accum += sample.sample1 - sample.sample0;
    }
    mOffsetCalibrationLowGain = accum / nSample;
}

template <uint32_t N_CHIPS>
typename HV507<N_CHIPS>::SampleData HV507<N_CHIPS>::sampleCapacitance(uint32_t sample_delay_ns, bool fire_sync_pulse) {
    SampleData ret;

    mAnalog->setupIntVout();
    // Performed with interrupts disabled for consistent timing
    {
        modm::atomic::Lock lck;
        // Release the current integrator reset to begin measuring charge transfer
        INT_RESET::setOutput(false);
        modm::delay(std::chrono::nanoseconds(AppConfig::IntegratorResetDelay()));
        // Assert GPIO for test / debug
        // Allows scope to trigger on desired channel
        if(fire_sync_pulse) {        
            SCAN_SYNC::setOutput(true);        
        }
        // Take an initial reading of integrator output -- the integrator does not
        // reset fully to 0V
        ret.sample0 = mAnalog->readIntVout();
        // Release the blanking signal, and allow time for active electrodes to
        // charge and current to settle back to zero
        BL::setOutput(true);
        modm::delay(std::chrono::nanoseconds(sample_delay_ns));
        // Read voltage integrator
        ret.sample1 = mAnalog->readIntVout();
        SCAN_SYNC::setOutput(false);
        INT_RESET::setOutput(true);
    }
    return ret;
}

template <uint32_t N_CHIPS>
void HV507<N_CHIPS>::handleSetGain(events::SetGain &e) {
    for(uint32_t i=0; i<N_PINS; i++) {
        uint32_t offset = i / 8;
        uint32_t bit = i % 8;
        uint8_t gain = e.get_channel(i);
        if(gain == GainSetting::LOW) {
            mLowGainFlags[offset] |= 1<<bit;
        } else {
            mLowGainFlags[offset] &= ~(1<<bit);
        }
    }
}

template <uint32_t N_CHIPS>
typename HV507<N_CHIPS>::GainSetting
HV507<N_CHIPS>::getGain(uint32_t chan) {
    uint32_t offset = chan / 8;
    uint32_t bit = chan % 8;
    if((mLowGainFlags[offset] & (1<<bit)) != 0) {
        return GainSetting::LOW;
    } else {
        return GainSetting::HIGH;
    }
} 

template <uint32_t N_CHIPS>
void HV507<N_CHIPS>::setPolarity(bool pol) {
    if(pol) {
        POL::setOutput(true);
        // small delay before enabling augment FET to avoid shoot through current
        // Delay always performed so that augment enable doesn't change timing
        modm::delay(50ns);
        if(AppConfig::AugmentTopPlateLowSide()) {
            AUGMENT_ENABLE::setOutput(true);
        }
    } else {
        AUGMENT_ENABLE::setOutput(false);
        // small delay to avoid shoot through current
        modm::delay(50ns);
        POL::setOutput(false);
    }
}

