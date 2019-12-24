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
#define FSEL_CLEAR_PIN_BITS(x) ~(7 << FSEL_SHIFT(x))
#define PRINT_REG(prefix, reg) printf("%s: reg @ %p = %x\n", prefix, &reg, reg);
#define VERBOSE 1

/* 
******************** CLOCK CONTROL ************************
Registers and relevant bit assignments for setting up GPCLK
*/

// function select register for GPCLK0, GPCLK1, GPCLK2
// clock control registers start at +0x101070, but this is not a multiple of our system page size, so we have to start from +0x101000
#define CLK_CTRL_BASE_OFFSET 0x101000 
#define CLK_CTRL_BASE (bcm_base + CLK_CTRL_BASE_OFFSET)
#define CLK_CTRL_BASE_MAPSIZE 0xA8 // need to hold up to 0x101088

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
************************** PCM *****************************
*/

#define PCM_BASE_OFFSET 0x3000
#define PCM_BASE (bcm_base + PCM_BASE_OFFSET)
#define PCM_BASE_MAPSIZE 0x24

#define PCM_CTRL_REG   0
#define PCM_FIFO_REG   1
#define PCM_MODE_REG   2
#define PCM_RXC_REG    3
#define PCM_TXC_REG    4
#define PCM_DREQ_REG   5
#define PCM_INTEN_REG  6
#define PCM_INTSTC_REG 7
#define PCM_GRAY_REG   8


/*
********************** GPIO CONTROL ************************
All other GPIO-relevant registers and bit assignments
*/

#define GPIO_BASE_OFFSET 0x200000
#define GPIO_BASE (bcm_base + GPIO_BASE_OFFSET)
#define GPIO_BASE_MAPSIZE 0xF4

#define GPIO_SET_REG 7  // turns GPIO pins high
#define GPIO_CLR_REG 10 // turns GPIO pins low
#define GPIO_LEV_REG 13 // when in input mode, bit n reads GPIO pin n


// ************************* GPIO FUNCTIONS *********************************

void setPinHigh(char n);

void setPinLow(char n);

void LEDTest(char pin, unsigned char numBlinks, unsigned delay_seconds);

// ************************** CLOCK FUNCTIONS ********************************

static int getSourceFrequency(char src);

static bool clockIsBusy(int clock_ctrl_reg);

static int setDiv(unsigned freq, bool mash);

static bool isValidClockSelection(char clockNum);

void disableClock(char clockNum);

int startClock(char clockNum);

int initClock(char clockNum, unsigned frequency, bool mash);

void disableAllClocks();

// ************************* PCM FUNCTIONS *****************************

static void initPolledMode();

static void initInterruptMode();

static void initDMAMode();

void initPCM();

// ************************* MEMORY MAPPING ****************************

static bool initMemMap();

static void clearMemMap();

#endif
