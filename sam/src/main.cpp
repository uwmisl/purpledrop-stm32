#include <cstdio>
#include <cstring>

#include "modm/platform.hpp"
#include "modm/processing.hpp"
#include "modm/architecture/interface/assert.hpp"
#include "modm/math/utils/bit_operation.hpp"

#include "Analog.hpp"
#include "AppConfigController.hpp"
#include "AuxGpios.hpp"
#include "CircularBuffer.hpp"
#include "Comms.hpp"
#include "Electrodes.hpp"
#include "EventEx.hpp"
#include "FeedbackControl.hpp"
#include "HV507.hpp"
#include "HvRegulator.hpp"
#include "InvertableGpio.hpp"
#include "Max31865.hpp"
#include "PwmDac.hpp"
#include "PwmOutput.hpp"
#include "SamCallbackTimer.hpp"
#include "SamFlash.hpp"
#include "ScanGroups.hpp"
#include "SystemClock.hpp"
#include "TempSensors.hpp"

#include "i2c_master_1.hpp"
#include "i2c_master_6.hpp" // TODO: drop me

using namespace modm::platform;
using namespace modm::literals;

typedef struct __attribute__((packed)) ContextStateFrame {
  uint32_t r0;
  uint32_t r1;
  uint32_t r2;
  uint32_t r3;
  uint32_t r12;
  uint32_t lr;
  uint32_t return_address;
  uint32_t xpsr;
} sContextStateFrame;

using AuxGpioArray = GpioArray<
    GpioB13,
    GpioA27,
    GpioA26
>;

struct Hv507_Pins {
    using SCK = InvertableGpio<GpioB1>;
    using MOSI = InvertableGpio<GpioB8>;
    using POL = InvertableGpio<GpioA2>;
    using BL = InvertableGpio<GpioB0>;
    using LE = InvertableGpio<GpioB4>;
    using AUGMENT_ENABLE = InvertableGpio<GpioA25>;
    using INT_RESET = GpioA3;

    using GAIN_SEL = GpioA4;
    // Debug IO, useful for syncing scope capacitance scan
    using SCAN_SYNC = GpioB11;
};
using Hv507_SPI = SpiMaster4;

struct TempSensor_Pins {
    using SCK = GpioA15;
    using MOSI = GpioA6;
    using MISO = GpioA5;
    using CS0 = GpioA29;
    using CS1 = GpioA30;
    using CS2 = GpioA16;
    using CS3 = GpioA31;
};
using TempSensor_SPI = SpiMaster2;

// Analog input pins
using VHV_FB_P = GpioA17;
using VHV_FB_N = GpioA18;
using INT_VOUT = GpioA19;
using ISENSE = GpioA20;

// The right pins for purpledrop board
using I2C = purpledrop::I2cMaster1;
using SDA_PIN = modm::platform::GpioB3;
using SCL_PIN = modm::platform::GpioB2;

using ElectrodeSchedulingTimer = SamCallbackTimer<TimerChannel3>;
using ElectrodeTimingTimer = SamCallbackTimer<TimerChannel4>;

using VHV_TARGET = GpioA0;
using Dac = PwmDac<TimerChannel0, VHV_TARGET>;
using AnalogImpl = Analog<modm::platform::Adc, INT_VOUT, ISENSE, VHV_FB_P, VHV_FB_N>;

AppConfigController<SamFlash, 0, 1> appConfigController;
AuxGpios<AuxGpioArray> auxGpios;
Electrodes<HV507<Hv507_Pins, Hv507_SPI, AnalogImpl>, ElectrodeSchedulingTimer, ElectrodeTimingTimer> hvControl;
FeedbackControl feedbackControl;
EventEx::EventBroker broker;
Comms comms;
HvRegulator<Dac, AnalogImpl> hvRegulator;
TempSensors<TempSensor_Pins, TempSensor_SPI> tempSensors;
//PwmOutput<I2C> pwmOutput;

//using LoopTimingPin = GpioB11;
using SwitchGreenPin = GpioA14;
using SwitchRedPin = GpioA13;

// Provides a delayed signal for rebooting to DFU mode
static modm::Timeout dfuDetachTimeout{20ms};

#define GPIO_AF_OTG1_FS 10

void USB_Init()
{
  // Briefly pull DP low to indicate disconnect to host
  MATRIX->CCFG_SYSIO |= (CCFG_SYSIO_SYSIO11 | CCFG_SYSIO_SYSIO10);
  GpioA22::setOutput(false);
  modm::delay(10ms);
  GpioA22::setInput();

  SystemClock::enableUsb();
  modm::platform::Usb::initialize<SystemClock>();
}

void set_bootloader_flag(bool flag) {
    if(flag) {
        GPBR->SYS_GPBR[7] = 0x4D49534C;
    } else {
        GPBR->SYS_GPBR[7] = 0;
    }
}

void reboot_to_bootloader() {
    // Set a flag to signal bootloader to remain in DFU mode
    set_bootloader_flag(true);
    // Reset
    RSTC->RSTC_CR = RSTC_CR_PROCRST_Msk | RSTC_CR_PERRST_Msk | RSTC_CR_KEY_PASSWD;
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

MODM_ISR(TC3) {
    ElectrodeSchedulingTimer::irqHandler();
    hvControl.timerIrqHandler();
}

int main() {
    // Turn off the watchdog
    WDT->WDT_MR = (WDT_MR_WDDIS_Msk);

    // Clear bootloader signal
    set_bootloader_flag(false);
    dfuDetachTimeout.stop();

    // Select PB4
    MATRIX->CCFG_SYSIO |= CCFG_SYSIO_SYSIO4;

    SystemClock::enable();
    SystemClock::enableUsb();
    SysTickTimer::initialize<SystemClock>();
    modm::platform::Usb::initialize<SystemClock>();

    printf("Hello World!\n");

    Hv507_SPI::connect<
            Hv507_Pins::SCK::Sck,
            Hv507_Pins::MOSI::Mosi>();
    TempSensor_SPI::connect<
            TempSensor_Pins::SCK::Sck,
            TempSensor_Pins::MOSI::Mosi,
            TempSensor_Pins::MISO::Miso>();


    I2C::connect<SDA_PIN::Twd, SCL_PIN::Twck>();
    I2C::initialize<SystemClock, 100000>();

    modm::platform::Adc::initialize<SystemClock>();
    Dac::init();
    ElectrodeSchedulingTimer::init();

    appConfigController.init(&broker);
    auxGpios.init(&broker);
    hvControl.init<SystemClock>(&broker);
    feedbackControl.init(&broker);
    hvRegulator.init(&broker);
    tempSensors.init<SystemClock>(&broker);
    // TODO: Need driver for new PWM chip
    //pwmOutput.init<SystemClock>(&broker);

    USB_Init();
    tusb_init();

    broker.registerHandler(&switchUpdateHandler);

    comms.init(&broker);

    while(1) {
        tud_task();
        if(dfuDetachTimeout.execute()) {
            reboot_to_bootloader();
        }
        comms.poll();
        tempSensors.poll();
        hvRegulator.poll();
        // pwmOutput.poll();
        hvControl.poll();
    }
}

void assert_failed(uint8_t* file, uint32_t line) {
    (void)file;
    (void)line;
    printf("ASSERT FAILURE\n");
    while(true) {
        SwitchRedPin::toggle();
        modm::delay(250ms);
    }
}

void modm_abandon(const modm::AssertionInfo &info) {
    (void)info;
    while(true) {
        SwitchRedPin::toggle();
        modm::delay(250ms);
    }
}

extern "C" void HardFault_Handler() {
    while(true) {
        SwitchRedPin::toggle();
        modm::delay(250ms);
    }
}

// Redirect stdio/printf output
extern "C" int _write(int handle, char *data, int size )
{
    int count ;

    handle = handle ; // unused

    for( count = 0; count < size; count++)
    {
        //ITM_SendChar( data[count] );
        //Uart7::write(data[count]);
        (void)data;
    }

    return count;
}

// Callback for the tinyusb DFU runtime driver, called when DFU_DETACH command
// is recieved.
extern "C" void tud_dfu_runtime_reboot_to_dfu_cb(void)
{
    // Will trigger reboot after some delay
    // The delay is to allow the USB transaction to complete
    dfuDetachTimeout.restart();
}