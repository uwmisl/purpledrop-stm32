#include <cstdint>
#include <chrono>
#include <functional>

#include "modm/platform.hpp"
#include "Analog.hpp"
#include "AppConfig.hpp"
#include "CallbackTimer.hpp"
#include "EventEx.hpp"
#include "Events.hpp"
#include "ScanGroups.hpp"
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

using SchedulingTimer = CallbackTimer<Timer5>;

// Cycles between capacitance scans
static const uint32_t SCAN_PERIOD = 500;
// Duration of each voltage pulse
static const uint32_t DRIVE_PERIOD_US = 1000;

enum CalibrateStep : uint8_t {
    CALSTEP_NONE = 0,
    CALSTEP_REQUEST = 2,
    CALSTEP_SETTLE = 3
};

class HV507 {
public:
    static const uint32_t N_PINS = AppConfig::N_PINS;
    static const uint32_t N_BYTES = AppConfig::N_BYTES;
    typedef std::array<uint8_t, N_BYTES> PinMask;

    enum GainSetting {
        HIGH = 0,
        LOW = 1
    };

    struct SampleData {
        uint16_t sample0; // starting value
        uint16_t sample1; // final value
    };

    HV507() {
        // Save singleton reference
        mSingleton = this;
    }

    void init(EventEx::EventBroker *broker, Analog *analog);

    void setElectrodes(uint8_t electrodes[N_BYTES]) {
        for(uint32_t i=0; i<N_BYTES; i++) {
            mShiftRegA[i] = electrodes[i];
        }
        mShiftRegDirty = true;
    }

    static void timerIrqHandler() {
        if(mSingleton) {
            mSingleton->callback();
        }
    }

private:

    enum TopState_e {
        DriveN = 0,
        DriveP,
        Scan,
        MeasureGroups,
    };

    enum DriveState_e {
        Start,
        WaitIntermediate,
        EndPulse
    };

    enum GroupState_e {
        Init,
        Load,
        Measure
    };

    struct FSM {
        TopState_e top = TopState_e::DriveN;
        DriveState_e drive = DriveState_e::Start;
        uint32_t count = 0;
    };

    FSM mFsm;

    PinMask mShiftRegA;
    PinMask mShiftRegB;
    uint8_t mDutyCycleA;
    uint8_t mDutyCycleB;

    ScanGroups<N_PINS> mScanGroups;

    /* These must be cached to ensure changes are made only when starting a new
    cycle */
    PinMask mShadowShiftReg;
    PinMask mIntermediateShiftReg;
    bool mWriteIntermediate = false;

    bool mShiftRegDirty = false;
    uint32_t mCyclesSinceScan;
    uint16_t mScanData[N_PINS];
    uint16_t mOffsetCalibration;
    uint16_t mOffsetCalibrationLowGain;
    uint8_t mLowGainFlags[N_BYTES];
    uint8_t mCalibrateStep;
    uint16_t mGroupScanData[ScanGroups<N_PINS>::MaxGroups];
    EventEx::EventHandlerFunction<events::SetElectrodes> mSetElectrodesHandler;
    EventEx::EventHandlerFunction<events::SetGain> mSetGainHandler;
    EventEx::EventHandlerFunction<events::CapOffsetCalibrationRequest> mCapOffsetCalibrationRequestHandler;
    EventEx::EventBroker *mBroker;
    Analog *mAnalog;

    static HV507 *mSingleton;

    void callback();
    /* Perform capacitance scan of all electrodes*/
    void scan();
    void loadShiftRegister(PinMask shift_reg);
    void latchShiftRegister();
    void calibrateOffset();
    SampleData sampleCapacitance(uint32_t sample_delay_ns, bool fire_sync_pulse);
    void handleSetGain(events::SetGain &e);
    GainSetting getGain(uint32_t channel);
    void setPolarity(bool pol);
    bool getPolarity();
    void blank() { BL::setOutput(false); };
    void unblank() { BL::setOutput(true); };
    bool driveFsm();
    void groupScan();
};
