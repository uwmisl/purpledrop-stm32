
#include <modm/platform.hpp>

using std::literals::chrono_literals::operator""ns;

static uint16_t medianof3(uint16_t a, uint16_t b, uint16_t c) {
    if(a < b && a < c) {
        return std::min(b, c);
    } else if(b < a && b < c) {
        return std::min(a, c);
    } else {
        return std::min(a, b);
    }
}

/** Provides low level control of a pair of HV507 chips
 */
template<typename PINS, class SPI, class Analog>
class HV507 {
public:
    using Pins = PINS;
    using Spi = SPI;

    static const uint32_t N_PINS = AppConfig::N_PINS;
    static const uint32_t N_BYTES = AppConfig::N_BYTES;
    typedef std::array<uint8_t, N_BYTES> PinMask;

    enum class GainSetting : uint8_t{
        High = 0,
        Low
    };

    template<typename SystemClock>
    static void
    init() {
        SPI::template initialize<SystemClock, 6000000>();
        SPI::setDataOrder(SPI::DataOrder::MsbFirst);

        PINS::SCAN_SYNC::setOutput(false);
        PINS::INT_RESET::setOutput(true);
        PINS::GAIN_SEL::setOutput(false);
        PINS::AUGMENT_ENABLE::setOutput(false);
        PINS::POL::setOutput(false);
        PINS::BL::setOutput(false);
        PINS::LE::setOutput(false);
    }

    inline static void
    setPolarity(bool pol) {
        if(pol) {
            PINS::POL::setOutput(true);
            // small delay before enabling augment FET to avoid shoot through current
            // Delay always performed so that augment enable doesn't change timing
            modm::delay(50ns);
            if(AppConfig::AugmentTopPlateLowSide()) {
                PINS::AUGMENT_ENABLE::setOutput(true);
            }
        } else {
            PINS::AUGMENT_ENABLE::setOutput(false);
            // small delay to avoid shoot through current
            modm::delay(50ns);
            PINS::POL::setOutput(false);
        }
    }

    inline static bool
    getPolarity() {
        return PINS::POL::read();
    }

    inline static void
    setGain(GainSetting g) {
        if(g == GainSetting::Low) {
            PINS::GAIN_SEL::setOutput(true);
        } else {
            PINS::GAIN_SEL::setOutput(false);
        }
    }

    inline static void
    blank() {
        PINS::BL::setOutput(false);
    };

    inline static void
    unblank() {
        PINS::BL::setOutput(true);
    };

    inline static void
    resetIntegrator() {
        PINS::INT_RESET::set();
    }

    inline static void
    enableIntegrator() {
        PINS::INT_RESET::reset();
    }

    inline static void
    loadShiftRegister(PinMask shift_reg) {
        SPI::transferBlocking(&shift_reg[0], 0, N_BYTES);
    }

    inline static void
    latchShiftRegister() {
        PINS::LE::setOutput(true);
        // Per datasheet, min LE pulse width is 80ns
        modm::delay(80ns);
        PINS::LE::setOutput(false);
    }

    inline static void
    setScanSync() {
        PINS::SCAN_SYNC::set();
    }

    inline static void
    clearScanSync() {
        PINS::SCAN_SYNC::reset();
    }

    inline static void
    setupAnalog() {
        Analog::setupIntVout();
    }

    inline static uint16_t
    readIntVout() {
        return Analog::readIntVout();
    }
};

