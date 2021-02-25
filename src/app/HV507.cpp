#include "HV507.hpp"


static const uint32_t TIMER_IRQ_PRIO = 1;

MODM_ISR(TIM5) {
    SchedulingTimer::irqHandler();
    HV507::timerIrqHandler();
}

HV507* HV507::mSingleton = nullptr;

void HV507::init(EventEx::EventBroker *broker, Analog *analog) {
    mBroker = broker;
    mAnalog = analog;
    mCyclesSinceScan = 0;
    mCalibrateStep = CALSTEP_NONE;
    mDutyCycleA = 255;
    mDutyCycleB = 255;
    for(uint32_t i=0; i<N_BYTES; i++) {
        mShiftRegA[i] = 0;
        mShiftRegB[i] = 0;
        mLowGainFlags[i] = 0;
    }

    SPI::connect<SCK::Sck, MOSI::Mosi, GpioUnused::Miso>();
    SPI::initialize<SystemClock, 6000000>();
    SPI::setDataOrder(SPI::DataOrder::LsbFirst);
    // modm SPI driver doesn't support IRQ setup
    NVIC_SetPriority(TIM5_IRQn, TIMER_IRQ_PRIO);
    NVIC_EnableIRQ(TIM5_IRQn);

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

    mSetElectrodesHandler.setFunction([this](auto &e) { handleSetElectrodes(e); });
    mBroker->registerHandler(&mSetElectrodesHandler);
    mSetGainHandler.setFunction([this](auto &e) { handleSetGain(e); });
    mBroker->registerHandler(&mSetGainHandler);
    mCapOffsetCalibrationRequestHandler.setFunction([this](auto &) { this->mCalibrateStep = CALSTEP_REQUEST; });
    mBroker->registerHandler(&mCapOffsetCalibrationRequestHandler);
    mSetDutyCycleHandler.setFunction([this](auto &e){ handleSetDutyCycle(e); });
    mBroker->registerHandler(&mSetDutyCycleHandler);

    // Kick off asynchronous drive
    SchedulingTimer::init();
    SchedulingTimer::reset();
    SchedulingTimer::schedule(DRIVE_PERIOD_US);
}

void HV507::poll() {
    while(mAsyncEventQ.count() > 0) {
        AsyncEvent_e e = mAsyncEventQ.pop();
        if(e == AsyncEvent_e::SendActiveCap) {
            // Send active capacitance message
            auto &sample = mLastActiveSample;
            events::CapActive event(sample.sample0 + mOffsetCalibration, sample.sample1);
            mBroker->publish(event);
        } else if(e == AsyncEvent_e::SendGroupCap) {
            events::CapGroups event;
            event.measurements = mGroupScanData;
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

void HV507::groupScan() {
    if(!mScanGroups.isAnyGroupActive()) {
        mGroupScanData.fill(0);
        return;
    }
    for(uint32_t group=0; group<AppConfig::N_CAP_GROUPS; group++) {
        if(mScanGroups.isGroupActive(group)) {
            mShadowShiftReg = mScanGroups.getGroupMask(group);
            uint16_t calibration_offset;
            uint32_t sample_delay;
            if((mScanGroups.getGroupSetting(group) & 1) == GainSetting::LOW) {
                calibration_offset = mOffsetCalibrationLowGain;
                sample_delay = AppConfig::SampleDelayLowGain();
                GAIN_SEL::setOutput(true);
            } else {
                sample_delay = AppConfig::SampleDelay();
                calibration_offset = mOffsetCalibration;
                GAIN_SEL::setOutput(false);
            }
            blank();
            // Presently assuming the SPI transfer time is more than enough blanking time,
            // so this is not adjustable 
            loadShiftRegister(mShadowShiftReg);
            latchShiftRegister();

            // Use 10,000 + group number to trigger on a particular group measurement
            bool fire_sync_pulse = AppConfig::ScanSyncPin() == (int32_t)(10000 + group);
            SampleData sample = sampleCapacitance(sample_delay, fire_sync_pulse);
            if(mFsm.count < AppConfig::N_CAP_GROUPS) {
                mGroupScanData[group] = sample.sample1 - sample.sample0 - calibration_offset;
                if(mGroupScanData[group] > 32767) {
                    mGroupScanData[group] = 0;
                }
            }
        } else {
            mGroupScanData[group] = 0;
        }
    }
    mAsyncEventQ.push(AsyncEvent_e::SendGroupCap);
}

bool HV507::driveFsm() {
    if(mFsm.drive == DriveState_e::Start) {
        SchedulingTimer::reset();
        blank();
        if(mFsm.top == TopState_e::DriveN) {
            blank();
            setPolarity(false);
        } else {
            blank();
            setPolarity(true);
        }
        // Write both enable groups
        for(uint32_t b=0; b<N_BYTES; b++) {
            mShadowShiftReg[b] = mShiftRegA[b] | mShiftRegB[b];
        }
        loadShiftRegister(mShadowShiftReg);
        latchShiftRegister();
        if(mShiftRegDirty) {
            mShiftRegDirty = false;
            mAsyncEventQ.push(AsyncEvent_e::SendElectrodeAck);
        }

        if(mFsm.top == TopState_e::DriveN) {
            unblank();
        } else {
            // Active capacitance is always done at high gain
            GAIN_SEL::setOutput(false);
            // Scan sync pin -1 causes sync pulse on active capacitance measurement
            bool fire_sync_pulse = AppConfig::ScanSyncPin() == -1;
            mLastActiveSample = sampleCapacitance(AppConfig::SampleDelay(), fire_sync_pulse);
            mAsyncEventQ.push(AsyncEvent_e::SendActiveCap);
        }

        // Determine what intermediate steps are needed
        if(mDutyCycleA == mDutyCycleB) {
            mWriteIntermediate = false;
        } else {
            mWriteIntermediate = true;
            if(mDutyCycleA < mDutyCycleB) {
                for(uint32_t i=0; i<N_BYTES; i++) {
                    mIntermediateShiftReg[i] = mShiftRegB[i];
                }
            } else {
                for(uint32_t i=0; i<N_BYTES; i++) {
                    mIntermediateShiftReg[i] = mShiftRegA[i];
                }
            }
        }

        // Next step, we will either latch the new partial shift register value, 
        // or we will go straight to blanking
        if(mWriteIntermediate) {
            loadShiftRegister(mIntermediateShiftReg);
            mFsm.drive = DriveState_e::WaitIntermediate;
        } else {
            mFsm.drive = DriveState_e::EndPulse;
        }

        uint8_t min_duty = std::min(mDutyCycleA, mDutyCycleB);
        int32_t wait_time = (DRIVE_PERIOD_US * min_duty) / 255 - SchedulingTimer::time_us();
        if(wait_time < 0) {
            wait_time = 0;
        }
        SchedulingTimer::schedule(wait_time);
        return false;
    } else if(mFsm.drive == DriveState_e::WaitIntermediate) {
        uint8_t max_duty = std::max(mDutyCycleA, mDutyCycleB);
        int32_t wait_time = DRIVE_PERIOD_US * max_duty / 255 - SchedulingTimer::time_us();
        if(wait_time < 0) {
            wait_time = 0;
        }
        latchShiftRegister();
        mFsm.drive = DriveState_e::EndPulse;
        SchedulingTimer::schedule(wait_time);
        return false;
    } else if(mFsm.drive == DriveState_e::EndPulse) {
        blank();
        mFsm.drive = DriveState_e::EndCycle;
        int32_t wait_time = DRIVE_PERIOD_US - SchedulingTimer::time_us();
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

void HV507::callback() {
    if(mCalibrateStep == CALSTEP_REQUEST) {
        // When requested, setup the correct polarity, then allow a cycle to stabilize
        blank();
        setPolarity(true);
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
            blank();
            setPolarity(true);
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

}

void HV507::scan() {
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
}

void HV507::loadShiftRegister(PinMask shift_reg) {
    SPI::transferBlocking((uint8_t *)&shift_reg, 0, N_BYTES);

}

void HV507::latchShiftRegister() {
    LE::setOutput(true);
    // Per datasheet, min LE pulse width is 80ns
    modm::delay(80ns);
    LE::setOutput(false);
}

void HV507::calibrateOffset() {
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

typename HV507::SampleData HV507::sampleCapacitance(uint32_t sample_delay_ns, bool fire_sync_pulse) {
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

void HV507::handleSetDutyCycle(events::SetDutyCycle &e) {
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

void HV507::handleSetElectrodes(events::SetElectrodes &e) {
    if(e.groupID >= 100) {
        uint8_t scanGroup = e.groupID - 100;
        mScanGroups.setGroup(scanGroup, e.setting, e.values);
    } else if(e.groupID == 0) {
        for(uint32_t i=0; i<N_BYTES; i++) {
            mShiftRegA[i] = e.values[i];
        }
        mDutyCycleA = e.setting;
        mShiftRegDirty = true;
    } else if(e.groupID == 1) {
        for(uint32_t i=0; i<N_BYTES; i++) {
            mShiftRegB[i] = e.values[i];
        }
        mDutyCycleB = e.setting;
        mShiftRegDirty = true;
    }
}

void HV507::handleSetGain(events::SetGain &e) {
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

HV507::GainSetting HV507::getGain(uint32_t chan) {
    uint32_t offset = chan / 8;
    uint32_t bit = chan % 8;
    if((mLowGainFlags[offset] & (1<<bit)) != 0) {
        return GainSetting::LOW;
    } else {
        return GainSetting::HIGH;
    }
} 

void HV507::setPolarity(bool pol) {
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

bool HV507::getPolarity() {
    return POL::read();
}
