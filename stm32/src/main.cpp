#include <cstdio>
#include <cstring>

extern "C" {
    #include "stm32f4xx_ip_dbg.h"
}

#include "stm32f4xx.h"
#include "modm/platform.hpp"
#include "modm/platform/clock/rcc.hpp"
#include "modm/architecture/interface/clock.hpp"

#include "Analog.hpp"
#include "AuxGpios.hpp"
#include "StmCallbackTimer.hpp"
#include "CircularBuffer.hpp"
#include "AppConfigController.hpp"
#include "Comms.hpp"
#include "Electrodes.hpp"
#include "EventEx.hpp"
#include "FeedbackControl.hpp"
#include "HV507.hpp"
#include "HvRegulator.hpp"
#include "InvertableGpio.hpp"
#include "Max31865.hpp"
#include "PwmOutput.hpp"
#include "ScanGroups.hpp"
#include "Stm32Flash.hpp"
#include "SystemClock.hpp"
#include "TempSensors.hpp"

using namespace modm::platform;
using namespace modm::literals;

// StaticCircularBuffer<uint8_t, 4096> USBTxBuffer;
// StaticCircularBuffer<uint8_t, 512> USBRxBuffer;

using AuxGpioArray = GpioArray<
    GpioB0,
    GpioB2,
    GpioC5
>;

struct Hv507_Pins {
    using SCK = InvertableGpio<GpioC10>;
    using MOSI = InvertableGpio<GpioC12>;
    using POL = InvertableGpio<GpioB5>;
    using BL = InvertableGpio<GpioB4>;
    using LE = InvertableGpio<GpioC13>;
    using INT_RESET = GpioC2;
    using GAIN_SEL = GpioC3;
    // Debug IO, useful for syncing scope capacitance scan
    using SCAN_SYNC = GpioC1;
    using V_HV_TARGET = GpioA4; // Dac output pin
    using AUGMENT_ENABLE = InvertableGpio<GpioA3>;
};
using Hv507_SPI = SpiMaster3;

struct AnalogPins {
    using INT_VOUT = modm::platform::GpioA2;
    using VHV_FB_P = modm::platform::GpioA0;
    using VHV_FB_N = modm::platform::GpioA1;
    using ISENSE = void;
};

struct TempSensor_Pins {
    using SCK = GpioC7;
    using MOSI = GpioB15;
    using MISO = GpioB14;
    using CS0 = GpioB13;
    using CS1 = GpioB12;
    using CS2 = GpioB10;
    using CS3 = GpioB1;
};
using TempSensor_SPI = SpiMaster2;

using I2C = modm::platform::I2cMaster1;
using SDA_PIN = modm::platform::GpioB7;
using SCL_PIN = modm::platform::GpioB6;

// Simple wrapper to map `setOutput` to `setOutput1`
struct DacWrapper {
    static inline void setOutput(uint16_t x) {
        modm::platform::Dac::setOutput1(x);
    }
};

using AnalogImpl = Analog<
    Adc1,
    AnalogPins::INT_VOUT,
    AnalogPins::ISENSE,
    AnalogPins::VHV_FB_P,
    AnalogPins::VHV_FB_N>;
// On SAMG, limitations of the timer require two timers be used for scheduling callbacks
// On STM32, the same callback timer can be used for both interfaces
using ElectrodesSchedulingTimer = StmCallbackTimer<Timer5>;
using ElectrodesImpl = Electrodes<
    HV507<Hv507_Pins, Hv507_SPI, AnalogImpl>,
    ElectrodesSchedulingTimer,
    ElectrodesSchedulingTimer>;

AppConfigController<Stm32Flash, 0, 1> appConfigController;
AuxGpios<AuxGpioArray> auxGpios;

ElectrodesImpl hvControl;
FeedbackControl feedbackControl;
EventEx::EventBroker broker;
Comms comms;
HvRegulator<DacWrapper, AnalogImpl> hvRegulator;
TempSensors<TempSensor_Pins, TempSensor_SPI> tempSensors;
PwmOutput<I2C> pwmOutput;

using LoopTimingPin = GpioB11;
using SwitchGreenPin = GpioC8;
using SwitchRedPin = GpioA8;

#define GPIO_AF_OTG1_FS 10

void USB_OTG_Init()
{
  // Briefly pull DP low to indicate disconnect to host
  GpioA12::setOutput(Gpio::OutputType::PushPull);
  GpioA12::reset();
  modm::delay(10ms);
  GpioA12::setInput();
  GpioA11::setAlternateFunction(GPIO_AF_OTG1_FS);
  GpioA12::setAlternateFunction(GPIO_AF_OTG1_FS);
  GpioA9::setInput();
  // GpioA9 is used as VBUS sense input, but is left in default input state

  RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN; __DSB();
  RCC->AHB2RSTR |= RCC_AHB2RSTR_OTGFSRST; __DSB();
  RCC->AHB2RSTR &= ~RCC_AHB2RSTR_OTGFSRST;
  // Use vbus detection on PA9
  USB_OTG_FS->GCCFG |= USB_OTG_GCCFG_VBDEN;
  // Force device mode
  USB_OTG_FS->GUSBCFG |= USB_OTG_GUSBCFG_FDMOD;


  NVIC_SetPriority(OTG_FS_IRQn, NVIC_EncodePriority(1, 1, 3));
  NVIC_EnableIRQ(OTG_FS_IRQn);
}

void update_switch_led(events::HvRegulatorUpdate &e) {
    if(e.voltage < 15.0) {
        SwitchRedPin::setOutput(true);
    } else {
        SwitchRedPin::setOutput(false);
    }
}
EventEx::EventHandlerFunction<events::HvRegulatorUpdate>
switchUpdateHandler([](auto &e) { update_switch_led(e); });


// Pass the TIMER5 irq onto the electrodes module
MODM_ISR(TIM5) {
    ElectrodesSchedulingTimer::irqHandler();
    hvControl.timerIrqHandler();
}


int main() {
    // Setup global peripheral register structs for use in debugger
    IP_Debug();

    SystemClock::enable();
	SysTickTimer::initialize<SystemClock>();

    Hv507_SPI::connect<
            Hv507_Pins::SCK::Sck,
            Hv507_Pins::MOSI::Mosi>();
    TempSensor_SPI::connect<
            TempSensor_Pins::SCK::Sck,
            TempSensor_Pins::MOSI::Mosi,
            TempSensor_Pins::MISO::Miso>();

    I2C::connect<SDA_PIN::Sda, SCL_PIN::Scl>();

    modm::platform::Dac::initialize();
    modm::platform::Dac::enableChannel(modm::platform::Dac::Channel::Channel1);
    modm::platform::Dac::connect<Hv507_Pins::V_HV_TARGET::Out1>();
    modm::platform::Dac::enableOutputBuffer(modm::platform::Dac::Channel::Channel1, true);

    Adc1::initialize<SystemClock, 24_MHz>();
    Adc1::connect<AnalogPins::INT_VOUT::In2, AnalogPins::VHV_FB_N::In1, AnalogPins::VHV_FB_P::In0>();

    appConfigController.init(&broker);
    auxGpios.init(&broker);
    hvControl.init<SystemClock>(&broker);
    feedbackControl.init(&broker);
    hvRegulator.init(&broker);
    tempSensors.init<SystemClock>(&broker);
    pwmOutput.init<SystemClock>(&broker);

    USB_OTG_Init();
    tusb_init();

    broker.registerHandler(&switchUpdateHandler);

    comms.init(&broker);

    LoopTimingPin::setOutput(Gpio::OutputType::PushPull);
    while(1) {
        tud_task();
        LoopTimingPin::set();
        comms.poll();
        tempSensors.poll();
        hvRegulator.poll();
        pwmOutput.poll();
        hvControl.poll();
        LoopTimingPin::reset();
    }
}

void assert_failed(uint8_t* file, uint32_t line) {
    (void)file;
    (void)line;
    printf("ASSERT FAILURE\n");
    while(1) {}
}