#include <tuple>
#include "Events.hpp"

/** Run-time accessible list of Gpio types
 */
template<typename... Gpios>
class GpioArray {
public:

    static void setOutput(uint32_t n, bool value) {
        if(n >= size) {
            return;
        }
        setOutputTable[n](value);
    }

    static void setInput(uint32_t n) {
        if(n >= size) {
            return;
        }
        setInputTable[n]();
    }

    static bool read(uint32_t n) {
        if(n >= size) {
            return false;
        }
        return readTable[n]();
    }

    static constexpr uint32_t size = sizeof...(Gpios);

private:
    typedef void(*setOutputFn)(bool);
    static constexpr setOutputFn setOutputTable[] = {&Gpios::setOutput...};
    typedef void(*setInputFn)();
    static constexpr setInputFn setInputTable[] = {&Gpios::setInput...};
    typedef bool(*readFn)();
    static constexpr readFn readTable[] = {&Gpios::read...};
};

template <typename GpioArrayType>
class AuxGpios {
public:
    AuxGpios() {}
    
    void init(EventBroker *broker) 
    {
        mBroker = broker;
        mGpioControlHandler.setFunction([this](auto &e) { HandleGpioControl(e); });
        mBroker->registerHandler(&mGpioControlHandler);
    }

private:
    EventBroker *mBroker;
    EventHandlerFunction<events::GpioControl> mGpioControlHandler;

    void HandleGpioControl(events::GpioControl &e) {
        if(e.pin >= GpioArrayType::size) {
            return;
        }
        if(e.write) {
            if(e.outputEnable) {
                GpioArrayType::setOutput(e.pin, e.value);
            } else {
                GpioArrayType::setInput(e.pin);
            }
        }
        bool readValue = GpioArrayType::read(e.pin);
        // Return the value even on write; it can serve as an acknowledgement
        e.callback(e.pin, readValue);
    }
    
};
