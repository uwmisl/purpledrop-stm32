#pragma once

#include "SystemClock.hpp"

/** Uses a hardware timer to provide interrupt callbacks and timing */
template <typename TIM>
class CallbackTimer {
public:
    /* Initialize the timer hardware */
    static void init();

    /* Reset the timer counter to zero */
    static void reset();

    /* Get the current timer value in microseconds */
    static uint32_t time_us();

    /* Schedule an IRQ callback `delay_us` microseconds from now */
    static void schedule(uint32_t delay_us);

    /* Timer IRQ handler: must be connected by user, and interrupt vector must be enabled.
    For example, call `Timer5::enableInterruptVector(true, priority)`;
     */
    static void irqHandler();
};

template <typename TIM>
void CallbackTimer<TIM>::init() {
    TIM::enable();
    TIM::setMode(TIM::Mode::UpCounter);
    TIM::configureOutputChannel(1, TIM::OutputCompareMode::Inactive, 0, TIM::PinState::Disable, false);
}

template <typename TIM>
void CallbackTimer<TIM>::reset() {
    TIM::setValue(0);
}

template <typename TIM>
uint32_t CallbackTimer<TIM>::time_us() {
    return (uint32_t)(TIM::getValue() * 1e6f / (float)TIM::template getTickFrequency<SystemClock>());
}

template <typename TIM>
void CallbackTimer<TIM>::schedule(uint32_t delay_us) {
    // Pause timer to avoid race condition if counter ticks past compare while setting it
    TIM::pause();
    uint32_t current = TIM::getValue();
    uint32_t freq = TIM::template getTickFrequency<SystemClock>();
    uint32_t compare = current + (uint32_t)((float)delay_us * freq / 1e6);
    TIM::setCompareValue(1, compare);
    TIM::enableInterrupt(TIM::Interrupt::CaptureCompare1);
    TIM::start();
}

template <typename TIM>
void CallbackTimer<TIM>::irqHandler() {
    TIM::acknowledgeInterruptFlags(TIM::getInterruptFlags());
}