#pragma once

#include <modm/platform.hpp>

using TC = TimerChannel0;

template<typename TC, typename Pin>
class PwmDac {
public:
    // Timer overflow period
    static const uint32_t Period = 512;
    static inline void init() {
        TC::initialize();
        TC::setClockSource(TC::ClockSource::MckDiv2);
        TC::setWaveformMode(true);
        TC::setWaveformSelection(TC::WavSel::Up_Rc);
        TC::setRegC(Period);
        // Set on RA match, clear on RC match
        TC::setTioaEffects(TC::TioEffect::Set, TC::TioEffect::Clear);
        TC::template connect<typename Pin::Tioa>();
        TC::enable();
        TC::start();
    }

    static inline void setOutput(uint16_t value) {
        TC::setRegA(Period - value / 8);
    }
};
