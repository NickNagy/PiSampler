#ifndef GPIO_H
#define GPIO_H

#include "../globals.h"
#include "pimem.h"

#define FSEL_SHIFT(x) 3*x
#define FSEL_CLEAR_PIN_BITS(x) ~(7 << FSEL_SHIFT(x))

#define GPIO_BASE_OFFSET 0x200000
#define GPIO_BASE_MAPSIZE 0xF4

#define GPIO_SET_REG 7  // turns GPIO pins high
#define GPIO_CLR_REG 10 // turns GPIO pins low
#define GPIO_LEV_REG 13 // when in input mode, bit n reads GPIO pin n

void initGPIO();

bool setPinMode(char pin, char mode);

void setPinHigh(char n);

void setPinLow(char n);

void LEDTest(char pin, uint8_t numBlinks, uint32_t delay_seconds);

#endif
