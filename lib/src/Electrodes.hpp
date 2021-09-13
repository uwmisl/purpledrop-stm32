#include <cstdint>
#include <chrono>
#include <functional>

#include "modm/platform.hpp"
#include "modm/math/utils/bit_operation.hpp"
#include "AppConfig.hpp"
#include "CircularBuffer.hpp"
#include "EventEx.hpp"
#include "Events.hpp"
#include "ScanGroups.hpp"

using namespace modm::platform;
using namespace modm::literals;
using namespace std::chrono_literals;

// Cycles between capacitance scans
static const uint32_t SCAN_PERIOD = 500;
// Duration of each voltage pulse
static const uint32_t DRIVE_PERIOD_US = 1000;

enum CalibrateStep : uint8_t {
    CALSTEP_NONE = 0,
    CALSTEP_REQUEST = 2,
    CALSTEP_SETTLE = 3
};

// Utility to measure the change in signal over some number of samples
template<uint32_t Depth, typename T_Meas, typename T_Ticks>
struct Differencer {
    struct Result_t {
        T_Meas dx;
        T_Ticks dt;
    };
    T_Meas values[Depth];
    T_Ticks times[Depth];
    uint16_t ptr = 0;
    uint16_t count = 0;

    inline void push(T_Meas x, T_Ticks t) {
        ptr = (ptr + 1) % Depth;
        values[ptr] = x;
        times[ptr] = t;
        if(count < Depth) {
            count++;
        }
    }

    inline bool valid() {
        return count == Depth;
    }

    inline Result_t delta() {
        static_assert(!std::numeric_limits<T_Ticks>::is_signed);

        uint16_t oldest = (ptr + 1) % Depth;

        return Result_t{
            (T_Meas)(values[ptr] - values[oldest]),
            (T_Ticks)(times[ptr] - times[oldest])
        };
    }
};

template<typename HV507, typename SchedulingTimer, typename TimingTimer>
class Electrodes {
public:
    using GainSetting = HV507::GainSetting;

    struct ElectrodeCalibrationData {
        float voltage; // Voltage at which measurement is taken
        uint16_t offsets[HV507::N_PINS];
    };

    struct SampleData {
        uint16_t sample0; // starting value
        uint16_t sample1; // final value
    };

    Electrodes() {
        // Save singleton reference
        mSingleton = this;
    }

    template<typename SystemClock>
    void init(EventEx::EventBroker *broker) {
        mBroker = broker;
        mCyclesSinceScan = 0;
        mCalibrateStep = CALSTEP_NONE;
        mDutyCycleA = 255;
        mDutyCycleB = 255;
        for(uint32_t i=0; i<HV507::N_BYTES; i++) {
            mShiftRegA[i] = 0;
            mShiftRegB[i] = 0;
            mLowGainFlags[i] = 0;
        }

        HV507::template init<SystemClock>();

        calibrateOffset();

        mSetElectrodesHandler.setFunction([this](auto &e) { handleSetElectrodes(e); });
        mBroker->registerHandler(&mSetElectrodesHandler);
        mSetGainHandler.setFunction([this](auto &e) { handleSetGain(e); });
        mBroker->registerHandler(&mSetGainHandler);
        mCapOffsetCalibrationRequestHandler.setFunction([this](auto &) { this->mCalibrateStep = CALSTEP_REQUEST; });
        mBroker->registerHandler(&mCapOffsetCalibrationRequestHandler);
        mSetDutyCycleHandler.setFunction([this](auto &e){ handleSetDutyCycle(e); });
        mBroker->registerHandler(&mSetDutyCycleHandler);
        mUpdateElectrodeCalibrationHandler.setFunction([this](auto &e){ handleUpdateElectrodeCalibration(e); });
        mBroker->registerHandler(&mUpdateElectrodeCalibrationHandler);

        TimingTimer::init();
        // Kick off asynchronous drive
        SchedulingTimer::init();
        SchedulingTimer::reset();
        SchedulingTimer::schedule(DRIVE_PERIOD_US);

    }

    inline static SampleData
    sampleCapacitance(bool low_gain, bool fire_sync_pulse) {
        static const uint32_t N_SAMPLE = 1;
        static const uint32_t DIFF_DELAY = 3;
        SampleData ret;

        uint32_t accum0 = 0;
        uint32_t accum1 = 0;
        Differencer<DIFF_DELAY, uint16_t, typename SchedulingTimer::CountType> diff;
        uint32_t sample_count = 0;
        HV507::setupAnalog();

        // Pre-compute threshold in counts per timer tick to save cycles in loop
        // Config threshold is counts / us.
        float threshold = AppConfig::AutoSampleThreshold();

        // Performed with interrupts disabled for consistent timing
        {
            modm::atomic::Lock lck;

            // Release the current integrator reset to begin measuring charge transfer
            HV507::enableIntegrator();
            modm::delay(std::chrono::nanoseconds(AppConfig::IntegratorResetDelay()));
            // Assert GPIO for test / debug
            // Allows scope to trigger on desired channel
            if(fire_sync_pulse) {
                HV507::setScanSync();
            }
            // Take an initial reading of integrator output -- the integrator does not
            // reset fully to 0V
            for(uint32_t i=0; i<N_SAMPLE; i++) {
                accum0 += HV507::readIntVout();
            }
            // Release the blanking signal, charging active electrodes
            HV507::unblank();

            int16_t prev_sample = -1;
            if(AppConfig::AutoSampleDelay()) {
                // Wait for dV/dT to fall below threshold
                while(sample_count++ < (uint32_t)AppConfig::AutoSampleTimeout()) {
                    uint16_t x = HV507::readIntVout();
                    if((prev_sample > -1) && (x - prev_sample < threshold)) {
                        break;
                    }
                    prev_sample = x;
                }

                // Wait for an additional configurable delay after meeting
                // threshold or timing out
                modm::delay(std::chrono::nanoseconds(AppConfig::AutoSampleHoldoff()));
            } else {
                uint32_t sample_delay_ns;
                if(low_gain) {
                    sample_delay_ns = AppConfig::SampleDelayLowGain();
                } else {
                    sample_delay_ns = AppConfig::SampleDelay();
                }
                modm::delay(std::chrono::nanoseconds(sample_delay_ns));
            }

            // Read final value from integrator
            for(uint32_t i=0; i<N_SAMPLE; i++) {
                accum1 += HV507::readIntVout();
            }

            HV507::clearScanSync();
            HV507::resetIntegrator();
        }

        ret.sample0 = accum0/N_SAMPLE;
        ret.sample1 = accum1/N_SAMPLE;
        return ret;
    }

    // To be called frequently on main task
    void poll() {
        while(mAsyncEventQ.count() > 0) {
            AsyncEvent_e e = mAsyncEventQ.pop();
            if(e == AsyncEvent_e::SendActiveCap) {
                // Send active capacitance message
                auto &sample = mLastActiveSample;
                events::CapActive event(
                    sample.sample0 + mOffsetCalibration + mActiveElectrodeOffset,
                    sample.sample1,
                    AppConfig::ActiveCapLowGain() ? 1 : 0
                );
                mBroker->publish(event);
            } else if(e == AsyncEvent_e::SendGroupCap) {
                events::CapGroups event;
                event.measurements = mGroupScanData;
                event.scanGroups = mScanGroups;
                mBroker->publish(event);
            } else if(e == AsyncEvent_e::SendScanCap) {
                events::CapScan event;
                event.measurements = mScanData;
                mBroker->publish(event);
            } else if(e == AsyncEvent_e::SendElectrodeAck) {
                events::ElectrodesUpdated event;
                mBroker->publish(event);
            }
        }
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
    };

    enum AsyncEvent_e : uint8_t {
        SendActiveCap,
        SendScanCap,
        SendGroupCap,
        SendElectrodeAck
    };

    // Store signals to send event in the main task
    // We can't send messages from the IRQs because message serialization is not
    // thread safe. Queue ensures events are transmitted in the correct order.
    static const uint32_t EVENT_Q_SIZE = 16;
    StaticCircularBuffer<AsyncEvent_e, EVENT_Q_SIZE> mAsyncEventQ;

    FSM mFsm;

    HV507::PinMask mShiftRegA;
    HV507::PinMask mShiftRegB;
    uint8_t mDutyCycleA;
    uint8_t mDutyCycleB;

    ScanGroups<HV507::N_PINS, AppConfig::N_CAP_GROUPS> mScanGroups;

    /* These must be cached to ensure changes are made only when starting a new
    cycle */
    HV507::PinMask mShadowShiftReg;
    HV507::PinMask mIntermediateShiftReg;
    bool mWriteIntermediate = false;
    bool mShiftRegDirty = false;
    uint32_t mCyclesSinceScan;
    uint16_t mScanData[HV507::N_PINS];
    uint16_t mOffsetCalibration;
    uint16_t mOffsetCalibrationLowGain;
    uint8_t mLowGainFlags[HV507::N_BYTES];
    uint8_t mCalibrateStep;
    std::array<uint16_t, AppConfig::N_CAP_GROUPS> mGroupScanData;
    SampleData mLastActiveSample;
    ElectrodeCalibrationData mElectrodeCalibration;
    uint16_t mActiveElectrodeOffset;
    std::array<uint16_t, AppConfig::N_CAP_GROUPS> mGroupElectrodeOffsets;
    // Allocate storage for event handler callbacks
    EventEx::EventHandlerFunction<events::SetElectrodes> mSetElectrodesHandler;
    EventEx::EventHandlerFunction<events::SetGain> mSetGainHandler;
    EventEx::EventHandlerFunction<events::CapOffsetCalibrationRequest> mCapOffsetCalibrationRequestHandler;
    EventEx::EventHandlerFunction<events::SetDutyCycle> mSetDutyCycleHandler;
    EventEx::EventHandlerFunction<events::UpdateElectrodeCalibration> mUpdateElectrodeCalibrationHandler;

    EventEx::EventBroker *mBroker;

    // Pointer to the singleton class instance for static methods
    static Electrodes<HV507, SchedulingTimer, TimingTimer> *mSingleton;

    inline uint16_t electrodeOffset(uint8_t pin, bool low_gain=false) {
        float x = mElectrodeCalibration.offsets[pin] * AppConfig::HvControlTarget() / mElectrodeCalibration.voltage;
        if(low_gain) {
            x *= AppConfig::LowGainR() / AppConfig::HighGainR();
        }
        return x;
    }

    void callback() {
        if(mCalibrateStep == CALSTEP_REQUEST) {
            // When requested, setup the correct polarity, then allow a cycle to stabilize
            HV507::blank();
            HV507::setPolarity(true);
            mCalibrateStep = CALSTEP_SETTLE;
            SchedulingTimer::schedule(DRIVE_PERIOD_US);
        } else if(mCalibrateStep == CALSTEP_SETTLE) {
            // Previouse cycle we did setup, now measure the offset
            calibrateOffset();
            mCalibrateStep = CALSTEP_NONE;
            // Reset all state after calibration -- i.e. start over whatever stage of measurement we were in
            mFsm = FSM();
        }

        if(mFsm.top == TopState_e::DriveN) {
            if(driveFsm()) {
                // drive sub state machine finished
                mFsm.top = TopState_e::MeasureGroups;
                HV507::blank();
                HV507::setPolarity(true);
                SchedulingTimer::schedule(1);
                mCyclesSinceScan++;
            }
        } else if(mFsm.top == TopState_e::MeasureGroups) {
            groupScan();
            mFsm.top = TopState_e::DriveP;
            SchedulingTimer::schedule(1);
        } else if(mFsm.top == TopState_e::DriveP) {
            if(mCyclesSinceScan >= SCAN_PERIOD) {
                mCyclesSinceScan = 0;
                scan();
                mAsyncEventQ.push(AsyncEvent_e::SendScanCap);
            }

            if(driveFsm()) {
                mFsm.top = TopState_e::DriveN;
                SchedulingTimer::schedule(1);
            }
        }

        // Update settings for inverted opto isolator
        if(AppConfig::InvertedOpto()) {
            HV507::Spi::setDataMode(HV507::Spi::DataMode::Mode2); // Set CPOL = 1, inverted clock
            HV507::Pins::POL::setInvert(true);
            HV507::Pins::BL::setInvert(true);
            HV507::Pins::LE::setInvert(true);
            HV507::Pins::SCK::setInvert(true);
            HV507::Pins::MOSI::setInvert(true);
            HV507::Pins::AUGMENT_ENABLE::setInvert(true);
        } else {
            HV507::Spi::setDataMode(HV507::Spi::DataMode::Mode0); // Set CPOL = 0, non-inverted clock
            HV507::Pins::POL::setInvert(false);
            HV507::Pins::BL::setInvert(false);
            HV507::Pins::LE::setInvert(false);
            HV507::Pins::SCK::setInvert(false);
            HV507::Pins::MOSI::setInvert(false);
            HV507::Pins::AUGMENT_ENABLE::setInvert(false);
        }
    }

    bool driveFsm() {
        if(mFsm.drive == DriveState_e::Start) {
            TimingTimer::reset();
            HV507::blank();
            if(mFsm.top == TopState_e::DriveN) {
                HV507::setPolarity(false);
            } else {
                HV507::setPolarity(true);
            }
            // Write both enable groups
            for(uint32_t b=0; b<HV507::N_BYTES; b++) {
                if(AppConfig::InvertedOpto()) {
                    mShadowShiftReg[b] = ~mShiftRegA[b] & ~mShiftRegB[b];
                } else {
                    mShadowShiftReg[b] = mShiftRegA[b] | mShiftRegB[b];
                }
            }

            HV507::loadShiftRegister(mShadowShiftReg);
            HV507::latchShiftRegister();
            if(mShiftRegDirty) {
                mShiftRegDirty = false;
                mAsyncEventQ.push(AsyncEvent_e::SendElectrodeAck);
            }

            if(mFsm.top == TopState_e::DriveN) {
                HV507::unblank();
            } else {
                bool low_gain = AppConfig::ActiveCapLowGain();
                if(low_gain) {
                    HV507::setGain(GainSetting::Low);
                } else {
                    HV507::setGain(GainSetting::High);
                }
                // Scan sync pin -1 causes sync pulse on active capacitance measurement
                bool fire_sync_pulse = AppConfig::ScanSyncPin() == -1;
                mLastActiveSample = sampleCapacitance(low_gain, fire_sync_pulse);
                mAsyncEventQ.push(AsyncEvent_e::SendActiveCap);
            }

            // Determine what intermediate steps are needed
            if(mDutyCycleA == mDutyCycleB) {
                mWriteIntermediate = false;
            } else {
                mWriteIntermediate = true;
                if(mDutyCycleA < mDutyCycleB) {
                    for(uint32_t i=0; i<HV507::N_BYTES; i++) {
                        mIntermediateShiftReg[i] = mShiftRegB[i];
                    }
                } else {
                    for(uint32_t i=0; i<HV507::N_BYTES; i++) {
                        mIntermediateShiftReg[i] = mShiftRegA[i];
                    }
                }
                if(AppConfig::InvertedOpto()) {
                    for(size_t i=0; i<mIntermediateShiftReg.size(); i++) {
                        mIntermediateShiftReg[i] = ~mIntermediateShiftReg[i];
                    }
                }
            }

            // Next step, we will either latch the new partial shift register value,
            // or we will go straight to blanking
            if(mWriteIntermediate) {
                HV507::loadShiftRegister(mIntermediateShiftReg);
                mFsm.drive = DriveState_e::WaitIntermediate;
            } else {
                mFsm.drive = DriveState_e::EndPulse;
            }

            uint8_t min_duty = std::min(mDutyCycleA, mDutyCycleB);
            int32_t wait_time = (DRIVE_PERIOD_US * min_duty) / 255 - TimingTimer::time_us();
            if(wait_time < 0) {
                wait_time = 0;
            }
            SchedulingTimer::schedule(wait_time);
            return false;
        } else if(mFsm.drive == DriveState_e::WaitIntermediate) {
            uint8_t max_duty = std::max(mDutyCycleA, mDutyCycleB);
            int32_t wait_time = DRIVE_PERIOD_US * max_duty / 255 - TimingTimer::time_us();
            if(wait_time < 0) {
                wait_time = 0;
            }
            HV507::latchShiftRegister();
            mFsm.drive = DriveState_e::EndPulse;
            SchedulingTimer::schedule(wait_time);
            return false;
        } else if(mFsm.drive == DriveState_e::EndPulse) {
            HV507::blank();
            mFsm.drive = DriveState_e::EndCycle;
            int32_t wait_time = DRIVE_PERIOD_US - TimingTimer::time_us();
            if(wait_time < 0) {
                wait_time = 0;
            }
            SchedulingTimer::schedule(wait_time);
            return false;
        } else if(mFsm.drive == DriveState_e::EndCycle) {
            mFsm.drive = DriveState_e::Start;
            return true;
        }
        mFsm.drive = DriveState_e::Start;
        return true;
    }

    void groupScan() {
        if(!mScanGroups.isAnyGroupActive()) {
            mGroupScanData.fill(0);
            return;
        }
        for(uint32_t group=0; group<AppConfig::N_CAP_GROUPS; group++) {
            if(mScanGroups.isGroupActive(group)) {
                mShadowShiftReg = mScanGroups.getGroupMask(group);
                if(AppConfig::InvertedOpto()) {
                    for(size_t i=0; i<mShadowShiftReg.size(); i++) {
                        mShadowShiftReg[i] = ~mShadowShiftReg[i];
                    }
                }
                uint16_t calibration_offset;
                bool low_gain = mScanGroups.getGroupSetting(group) & 1;
                if(low_gain) {
                    calibration_offset = mOffsetCalibrationLowGain;
                    HV507::setGain(GainSetting::Low);
                } else {
                    calibration_offset = mOffsetCalibration;
                    HV507::setGain(GainSetting::High);
                }
                HV507::blank();
                // Presently assuming the SPI transfer time is more than enough blanking time,
                // so this is not adjustable
                HV507::loadShiftRegister(mShadowShiftReg);
                HV507::latchShiftRegister();

                // Use 10,000 + group number to trigger on a particular group measurement
                bool fire_sync_pulse = AppConfig::ScanSyncPin() == (int32_t)(10000 + group);
                auto sample = sampleCapacitance(low_gain, fire_sync_pulse);
                mGroupScanData[group] = sample.sample1 - sample.sample0 - calibration_offset - mGroupElectrodeOffsets[group];
                if(mGroupScanData[group] > 32767) {
                    mGroupScanData[group] = 0;
                }
            } else {
                mGroupScanData[group] = 0;
            }
        }
        mAsyncEventQ.push(AsyncEvent_e::SendGroupCap);
    }

    /** Perform capacitance scan of all electrodes */
    void scan() {
        // Takes over bitbang control of the SPI pins, after loading a single
        // '1' into the shift register. This 1 is shifted through all positions,
        // and the capacitance is measured for each.
        uint16_t offset_calibration;
        // Clear all bits in the shift register except the first
        for(uint32_t i=0; i < HV507::N_BYTES - 1; i++) {
            if(AppConfig::InvertedOpto()) {
                HV507::Spi::transferBlocking(0xff);
            } else {
                HV507::Spi::transferBlocking(0);
            }
        }
        if(AppConfig::InvertedOpto()) {
            HV507::Spi::transferBlocking(0xfe);
        } else {
            HV507::Spi::transferBlocking(0x01);
        }
        // Convert SCK and MOSI pins from alternate function to outputs
        HV507::Pins::MOSI::setOutput(0);
        HV507::Pins::SCK::setOutput(0);

        HV507::setPolarity(true);
        HV507::blank();

        modm::delay(std::chrono::nanoseconds(AppConfig::ScanStartDelay()));

        for(int32_t i = HV507::N_PINS-1; i >= 0; i--) {
            bool lowGain = getGain(i) == GainSetting::Low;
            if(lowGain) {
                offset_calibration = mOffsetCalibrationLowGain;
                HV507::setGain(GainSetting::Low);
            } else {
                offset_calibration = mOffsetCalibration;
                HV507::setGain(GainSetting::High);
            }
            // Skip the common top plate electrode
            if(i == (int)AppConfig::TopPlatePin()) {
                HV507::Pins::SCK::setOutput(true);
                modm::delay(80ns);
                HV507::Pins::SCK::setOutput(false);
                continue;
            }

            HV507::latchShiftRegister();
            modm::delay(std::chrono::nanoseconds(AppConfig::ScanBlankDelay()));

            // Assert sync pulse on the requested pin for scope triggering
            bool fire_sync_pulse = i == AppConfig::ScanSyncPin();
            SampleData sample = sampleCapacitance(lowGain, fire_sync_pulse);
            mScanData[i] = sample.sample1 - sample.sample0 - offset_calibration - electrodeOffset(i, lowGain);
            // It can go negative on overflow; clip it to zero when that happens
            if(mScanData[i] > 32767) {
                mScanData[i] = 0;
            }

            HV507::blank();

            // Clock in the next bit
            HV507::Pins::SCK::setOutput(true);
            modm::delay(80ns);
            HV507::Pins::SCK::setOutput(false);
        }

        // Restore GPIOs to alternate fucntion
        HV507::Spi::template connect<
            typename HV507::Pins::SCK::Sck,
            typename HV507::Pins::MOSI::Mosi>();
    }

    void calibrateOffset() {
        static const uint32_t nSample = 100;
        int32_t accum = 0;

        /* Offset calibration is calibrated for both normal and low gain because
        a different sample delay is used. The offset is essentially proportional
        to the sample delay, so one could probably be calculated from the other,
        but it's also pretty quick to just measure both */
        HV507::setGain(GainSetting::High);
        for(uint32_t i=0; i<nSample; i++) {
            auto sample = sampleCapacitance(false, false);
            accum += sample.sample1 - sample.sample0;
        }
        if(accum < 0) {
            accum = 0;
        }
        mOffsetCalibration = accum / nSample;

        HV507::setGain(GainSetting::Low);
        accum = 0;
        for(uint32_t i=0; i<nSample; i++) {
            auto sample = sampleCapacitance(true, false);
            accum += sample.sample1 - sample.sample0;
        }
        if(accum < 0) {
            accum = 0;
        }
        mOffsetCalibrationLowGain = accum / nSample;
    }

    GainSetting getGain(uint32_t channel) {
        uint32_t offset = channel / 8;
        uint32_t bit = channel % 8;
        if((mLowGainFlags[offset] & (1<<bit)) != 0) {
            return GainSetting::Low;
        } else {
            return GainSetting::High;
        }
    }

    void handleSetDutyCycle(events::SetDutyCycle &e) {
        if(e.updateA) {
            mDutyCycleA = e.dutyCycleA;
        }
        if(e.updateB) {
            mDutyCycleB = e.dutyCycleB;
        }
        events::DutyCycleUpdated update_event;
        update_event.dutyCycleA = mDutyCycleA;
        update_event.dutyCycleB = mDutyCycleB;
        mBroker->publish(update_event);
    }

    void handleSetElectrodes(events::SetElectrodes &e) {
        if(e.groupID >= 100) {
            uint8_t scanGroup = e.groupID - 100;
            if(scanGroup >= AppConfig::N_CAP_GROUPS) {
                return;
            }
            // Compute the total electrode compenation offset for the scan group
            mGroupElectrodeOffsets[scanGroup] = 0;
            bool lowGain = e.setting & 1;
            for(uint32_t i=0; i<HV507::N_PINS; i++) {
                if(mScanGroups.isPinActive(scanGroup, i)) {
                    mGroupElectrodeOffsets[scanGroup] += electrodeOffset(i, lowGain);
                }
            }
            std::array<uint8_t, sizeof(e.values)> reversedValues;
            for(uint32_t i=0; i<reversedValues.size(); i++) {
                reversedValues[i] = modm::bitReverse(e.values[i]);
            }
            mScanGroups.setGroup(scanGroup, e.setting, &reversedValues[0]);
        } else if(e.groupID == 0) {
            mActiveElectrodeOffset = 0;
            // Compute the total electrode compensation offset for the active measurement
            for(uint32_t i=0; i<HV507::N_PINS; i++) {
                uint8_t byteidx = i/8;
                uint8_t bit = i%8;
                if(e.values[byteidx] & (1<<bit)) {
                    mActiveElectrodeOffset += electrodeOffset(i, AppConfig::ActiveCapLowGain());
                }
            }
            for(uint32_t i=0; i<HV507::N_BYTES; i++) {
                mShiftRegA[i] = modm::bitReverse(e.values[i]);
            }
            mDutyCycleA = e.setting;
            mShiftRegDirty = true;
        } else if(e.groupID == 1) {
            for(uint32_t i=0; i<HV507::N_BYTES; i++) {
                mShiftRegB[i] = modm::bitReverse(e.values[i]);
            }
            mDutyCycleB = e.setting;
            mShiftRegDirty = true;
        }
    }

    void handleSetGain(events::SetGain &e) {
        for(uint32_t i=0; i<HV507::N_PINS; i++) {
            uint32_t offset = i / 8;
            uint32_t bit = i % 8;
            uint8_t gain = e.get_channel(i);
        if(gain == (uint8_t)GainSetting::Low) {
                mLowGainFlags[offset] |= 1<<bit;
            } else {
                mLowGainFlags[offset] &= ~(1<<bit);
            }
        }
    }

    void handleUpdateElectrodeCalibration(events::UpdateElectrodeCalibration &e) {
        if(e.offset + e.length > sizeof(mElectrodeCalibration)) {
            // Refuse to overrun the allocated buffer
            return;
        }
        memcpy((uint8_t*)&mElectrodeCalibration + e.offset, e.data, e.length);
    }
};

template<typename HV507, typename SchedulingTimer, typename TimingTimer>
Electrodes<HV507, SchedulingTimer, TimingTimer>* Electrodes<HV507, SchedulingTimer, TimingTimer>::mSingleton = nullptr;
