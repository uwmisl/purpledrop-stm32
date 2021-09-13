#pragma once

#include "SystemClock.hpp"

/** Uses a hardware timer to provide interrupt callbacks and timing */
template <typename TIM>
class SamCallbackTimer {
public:
    using CountType = uint16_t;

    /* Initialize the timer hardware */
    static inline void init();

    /* Reset the timer counter to zero */
    static inline void reset();

    /* Get the current timer value in microseconds */
    static inline uint32_t time_us();

    /* Get the current timer value in counts */
    static inline CountType time_counts();

    /* Get the counter frequency in Hz */
    static inline uint32_t tick_frequency();

    /* Schedule an IRQ callback `delay_us` microseconds from now */
    static inline  void schedule(uint32_t delay_us);

    /* Timer IRQ handler: must be connected by user, and interrupt vector must be enabled.
    For example, call `Timer5::enableInterruptVector(true, priority)`;
     */
    static inline void irqHandler();
};

template <typename TIM>
void SamCallbackTimer<TIM>::init() {
    TIM::initialize();
    TIM::setClockSource(TIM::ClockSource::MckDiv8);
    TIM::setWaveformSelection(TIM::WavSel::Up);
    TIM::setWaveformMode(true);
    TIM::enable();
    TIM::enableInterruptVector(true);
}

template <typename TIM>
void SamCallbackTimer<TIM>::reset() {
    TIM::start();
}

template <typename TIM>
uint32_t SamCallbackTimer<TIM>::time_us() {
    return (uint32_t)(TIM::getValue() * 1e6f / (float)TIM::template getTickFrequency<SystemClock>());
}

template <typename TIM>
SamCallbackTimer<TIM>::CountType SamCallbackTimer<TIM>::time_counts() {
    return TIM::getValue();
}

template <typename TIM>
uint32_t SamCallbackTimer<TIM>::tick_frequency() {
    return TIM::template getTickFrequency<SystemClock>();
}


template <typename TIM>
void SamCallbackTimer<TIM>::schedule(uint32_t delay_us) {
    // Pause timer to avoid race condition if counter ticks past compare while setting it
    TIM::disable();
    uint32_t freq = TIM::template getTickFrequency<SystemClock>();
    uint32_t compare = (uint32_t)((float)delay_us * freq / 1e6);
    // Setting compare to zero won't trigger until the counter wraps; min delay is 1 tick
    if(compare < 1) {
        compare = 1;
    }
    TIM::setRegA(compare);
    TIM::enableInterrupt(TIM::Interrupt::RaCompare);
    TIM::enable();
    TIM::start();
}

template <typename TIM>
void SamCallbackTimer<TIM>::irqHandler() {
    TIM::getInterruptFlags();
    TIM::disableInterrupt(TIM::Interrupt::RaCompare);
}