extern "C" {
    #include "stm32f4xx.h"
}

constexpr GPIO_TypeDef* PortA() {
    return GPIOA;
}

template<GPIO_TypeDef *PORT(), uint8_t PIN>
struct STM32_Gpio : public GpioPin {

    static void enableAlternate(uint8_t af) {
        setMode(GPIO_Mode_AF);
        GPIO_PinAFConfig(PORT(), PIN, af);
    }

    bool read() {
        return (PORT()->IDR & (1<<PIN)) != 0;
    }

    void write(bool value) {
        if(value) {
            PORT()->BRSSL = (1<<PIN);
        } else {
            PORT()->BRSSH = (1<<PIN);
        }
    }

    bool toggle() {
        write( (PORT()->ODR & (1<<PIN)) != 0);
    }

    void enableOutput(bool enable) {
        uint32_t mode = enable ? GPIO_Mode_Out : GPIO_Mode_IN;
        setMode(mode);
    }

    void setMode(uint8_t mode) {
        uint32_t tmp = PORT()->MODER &= 3 << (2*PIN);
        PORT()->MODER = tmp | (mode << 2*PIN);
    }
};

struct GpioPin {

    void setHigh() { write(true); }
    void setLow() { write(false); }

    virtual bool read() = 0;
    virtual void write(bool value) = 0;
    virtual void toggle() = 0;
    virtual void enableOutput(bool enable) = 0;    
};