#include <cstdint>
#include <chrono>
#include <functional>

#include "modm/platform.hpp"
#include "Analog.hpp"
#include "AppConfig.hpp"
#include "CallbackTimer.hpp"
#include "CircularBuffer.hpp"
#include "EventEx.hpp"
#include "Events.hpp"
#include "InvertableGpio.hpp"
#include "ScanGroups.hpp"
#include "SystemClock.hpp"

using namespace modm::platform;
using namespace modm::literals;
using namespace std::chrono_literals;

using SPI = SpiMaster3;
using SCK = InvertableGpio<GpioC10>;
using MOSI = InvertableGpio<GpioC12>;
using POL = InvertableGpio<GpioB5>;
using BL = InvertableGpio<GpioB4>;
using LE = InvertableGpio<GpioC13>;
using INT_RESET = GpioC2;
using INT_VOUT = GpioA2;
using GAIN_SEL = GpioC3;
// Debug IO, useful for syncing scope capacitance scan
using SCAN_SYNC = GpioC1;
using AUGMENT_ENABLE = InvertableGpio<GpioA3>;

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

    // To be called frequently (as fast as possible) by main task
    void poll();

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
        EndPulse,
        EndCycle
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

    enum AsyncEvent_e : uint8_t {
        SendActiveCap,
        SendScanCap,
        SendGroupCap,
        SendElectrodeAck
    };
    static const uint32_t EVENT_Q_SIZE = 16;

    // Store signals to send event in the main task
    // We can't send messages from the IRQs because message serialization is not
    // thread safe. Queue ensures events are transmitted in the correct order.
    StaticCircularBuffer<AsyncEvent_e, EVENT_Q_SIZE> mAsyncEventQ;

    FSM mFsm;

    PinMask mShiftRegA;
    PinMask mShiftRegB;
    uint8_t mDutyCycleA;
    uint8_t mDutyCycleB;

    ScanGroups<N_PINS, AppConfig::N_CAP_GROUPS> mScanGroups;

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
    std::array<uint16_t, AppConfig::N_CAP_GROUPS> mGroupScanData;
    SampleData mLastActiveSample;
    EventEx::EventHandlerFunction<events::SetElectrodes> mSetElectrodesHandler;
    EventEx::EventHandlerFunction<events::SetGain> mSetGainHandler;
    EventEx::EventHandlerFunction<events::CapOffsetCalibrationRequest> mCapOffsetCalibrationRequestHandler;
    EventEx::EventHandlerFunction<events::SetDutyCycle> mSetDutyCycleHandler;

    EventEx::EventBroker *mBroker;
    Analog *mAnalog;

    // Pointer to the singleton class instance for static methods
    static HV507 *mSingleton;

    void callback();
    /* Perform capacitance scan of all electrodes*/
    void scan();
    void loadShiftRegister(PinMask shift_reg);
    void latchShiftRegister();
    void calibrateOffset();
    SampleData sampleCapacitance(uint32_t sample_delay_ns, bool fire_sync_pulse);

    GainSetting getGain(uint32_t channel);
    void setPolarity(bool pol);
    bool getPolarity();
    void blank() { BL::setOutput(false); };
    void unblank() { BL::setOutput(true); };
    bool driveFsm();
    void groupScan();

    void handleSetDutyCycle(events::SetDutyCycle &e);
    void handleSetElectrodes(events::SetElectrodes &e);
    void handleSetGain(events::SetGain &e);
};
