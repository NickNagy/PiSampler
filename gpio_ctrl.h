/* 
Nick Nagy
Setting GPIO clocks using BCM2835 ARM Peripherals Guide.
*/

#ifndef GPCLK_SETUP_H
#define GPCLK_SETUP_H

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <bcm_host.h>

#define FSEL_SHIFT(x) 3*x
#define FSEL_CLEAR_PIN_BITS(x) ((7 << FSEL_SHIFT(x)) ^ 0xFFFFFFFF)

#define VERBOSE 1

/* 
******************** CLOCK CONTROL ************************
Registers and relevant bit assignments for setting up GPCLK
*/

// function select register for GPCLK0, GPCLK1, GPCLK2
// clock control registers start at +0x101070, but this is not a multiple of our system page size, so we have to start from +0x101000
#define CLK_CTRL_BASE_OFFSET 0x101000 
#define CLK_CTRL_BASE_MAPSIZE 168 //136 // need to hold up to 0x101088

// function select
#define CLK_ALT_SHIFT(x) 12 + FSEL_SHIFT(x)
#define CLK_FSEL_BITS(x) 4 << CLK_ALT_SHIFT(x)

// ctrl and div
#define CLK_CTRL_REG(x) 28 + (x<<1)
#define CLK_DIV_REG(x) 29 + (x<<1)

#define CLK_PASSWD (0x5A << 24)
#define MASH(x) (x << 9)
#define BUSY (1 << 7)
#define KILL (1 << 5)
#define ENABLE (1 << 4)

// source frequencies
#define OSC_FREQ 19200000
#define PLLC_FREQ 1000000000
#define PLLD_FREQ 500000000
#define HDMI_FREQ 216000000

// sources
#define OSC 1
#define PLLA 4
#define PLLC 5
#define PLLD 6

/*
********************** GPIO CONTROL ************************
All other GPIO-relevant registers and bit assignments
*/

#define GPIO_BASE_OFFSET 0x200000//(BCM_BASE + 0x200000)// 0x7E200000
#define GPIO_BASE_MAPSIZE 244

#define GPIO_SET_REG 7  // turns GPIO pins high
#define GPIO_CLR_REG 10 // turns GPIO pins low
#define GPIO_LEV_REG 13 // when in input mode, bit n reads GPIO pin n

static int getSourceFrequency(char src);

static bool clockIsBusy(int clock_ctrl_reg);

void setPinHigh(char n);

void setPinLow(char n);

int setDiv(unsigned freq, bool mash);

static bool isValidClockSelection(char clockNum);

void disableClock(char clockNum);

static int startClock(char clockNum);

static int initClock(char clockNum, unsigned frequency, bool mash);

void disableAllClocks();

void LEDTest(char pin, unsigned char numBlinks, unsigned delay_seconds);

static bool initMemMap();

static void clearMemMap();

#endif
