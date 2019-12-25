#include "gpio.h"

static unsigned * gpioMap = initMemMap(GPIO_BASE_OFFSET, GPIO_BASE_MAPSIZE);

// not static because needed by clk.h ?
bool setPinMode(char pin, char mode) {
    if (mode > 7) {
        printf("ERROR: INVALID MODE.\n");
        return 0;
    }
    char fsel_reg = pin/10;
    char pinR = pin % 10;
    gpioMap[fsel_reg] = (gpioMap[fsel_reg] & FSEL_CLEAR_PIN_BITS(pinR)) | (mode << FSEL_SHIFT(pinR));
    return 1;
}

void setPinHigh(char n) {
    gpioMap[GPIO_SET_REG] |= (1 << n);
}

void setPinLow(char n) {
    gpioMap[GPIO_CLR_REG] |= (1 << n);
}

void LEDTest(char pin, unsigned char numBlinks, unsigned delay_seconds) {
    unsigned delay_us = 1000000 * delay_seconds;
    int fsel_reg = pin/10;
    int pinR = pin % 10;
    setPinMode(pin, 1);
    if (VERBOSE) PRINT_REG("FSEL", gpioMap[fsel_reg]);
    for (char i = 0; i < numBlinks; i++) {
        printf("%d\n",i);
        setPinHigh(pin);
        usleep(delay_us);
        setPinLow(pin);
        usleep(delay_us);
    }
}