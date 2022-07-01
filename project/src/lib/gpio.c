#include "gpio.h"

struct gpio{
    unsigned int FSEL[6];
    unsigned int reservedA;
    unsigned int SET[2];
    unsigned int reservedB;
    unsigned int CLR[2];
    unsigned int reservedC;
    unsigned int LEV[2];
};

volatile struct gpio *gpio = (struct gpio *)0x20200000;

void gpio_init(void) {
    // no initialization required for this peripheral
}

void gpio_set_function(unsigned int pin, unsigned int function) {
    if(function > GPIO_FUNC_ALT3) {
        return; //does nothing if the function is invalid
    }
    unsigned int loc = pin/10;
    unsigned int exact_loc = 10 * loc;
    gpio->FSEL[loc] &= ~(0b111 << (pin - exact_loc) * 3); //"and"-ing bit mask at the position of FSEL
    gpio->FSEL[loc] |= (function << (pin - exact_loc) * 3); //"or"-ing bit mask at FSEL
}

unsigned int gpio_get_function(unsigned int pin) {
    if(pin > GPIO_PIN_LAST) {
        return GPIO_INVALID_REQUEST;
    }
    unsigned int loc = pin / 10;
    unsigned int exact_loc = 10 * loc;
    unsigned int reg = gpio->FSEL[loc];
    reg = reg >> (pin - exact_loc) * 3; //"cuts off" the bits previous to the current
    reg &= 0b111; //clears all bits except the most immediate 3
    return reg;
}

void gpio_set_input(unsigned int pin) {
    gpio_set_function(pin, GPIO_FUNC_INPUT);
}

void gpio_set_output(unsigned int pin) {
    gpio_set_function(pin, GPIO_FUNC_OUTPUT);
}

void gpio_write(unsigned int pin, unsigned int value) {
    unsigned int cur_pin_0 = 1 << pin;
    unsigned int cur_pin_1 = 1 << (pin - 32);

    if(value == 1 && pin < 32) {
        gpio->SET[0] = cur_pin_0;
    }
    if(value == 1 && pin >= 32 && pin <= GPIO_PIN_LAST) {
        gpio->SET[1] = cur_pin_1;
    }
    if(value == 0 && pin < 32) {
        gpio->CLR[0] = cur_pin_0;
    }
    if(value == 0 && pin >= 32 && pin <= GPIO_PIN_LAST) {
        gpio->CLR[1] = cur_pin_1;
    }
}

unsigned int gpio_read(unsigned int pin) {
    if(pin > GPIO_PIN_LAST) {
        return GPIO_INVALID_REQUEST;
    }
    if(pin < 32) {
        unsigned int result = gpio->LEV[0] >> pin;
        result &= 0b1; //clears all bits except the desired bit
        return result;
    }
    unsigned int result = gpio->LEV[1] >> (pin - 32);
    result &= 0b1; //clears all bits except the desired bit
    return result;
}
