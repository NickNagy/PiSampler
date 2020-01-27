#include "gpio.h"

static volatile uint32_t * gpioMap = 0;

// please call this function in your main() before everything else
void initGPIO() {
    printf("Initializing GPIO...");
    gpioMap = initMemMap(GPIO_BASE_OFFSET, GPIO_BASE_MAPSIZE);
    printf("done.\n");
}

// not static because needed by clk.h ?
void setPinMode(char pin, char mode) {
    if (!gpioMap)
        initGPIO();
    if (mode > 7)
        FATAL_ERROR("invalid pin mode for GPIO.");
    char fsel_reg = pin/10;
    char pinR = pin % 10;
    gpioMap[fsel_reg] = (gpioMap[fsel_reg] & FSEL_CLEAR_PIN_BITS(pinR)) | (mode << FSEL_SHIFT(pinR));
}

void setPinHigh(char n) {
    if (!gpioMap)
        initGPIO();
    gpioMap[GPIO_SET_REG] |= (1 << n);
}

void setPinLow(char n) {
    if (!gpioMap)
        initGPIO();
    gpioMap[GPIO_CLR_REG] |= (1 << n);
}

void LEDTest(char pin, uint8_t numBlinks, uint32_t delay_seconds) {
    uint32_t delay_us = 1000000 * delay_seconds;
    uint8_t fsel_reg = pin/10;
    uint8_t pinR = pin % 10;
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
