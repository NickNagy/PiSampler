#ifndef CLK_H
#define CLK_H

#include "../globals.h"
#include "pimem.h"
#include "gpio.h"

// function select register for GPCLK0, GPCLK1, GPCLK2
// clock control registers start at +0x101070, but this is not a multiple of our system page size, so we have to start from +0x101000
#define CLK_CTRL_BASE_OFFSET 0x101000
#define CLK_CTRL_BASE_MAPSIZE 0x88 // 0xA8 // need to hold up to 0x101088

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

static int32_t setDiv(uint32_t targetFreq, uint32_t sourceFreq, bool mash);

bool clockIsBusy(int32_t clock_ctrl_reg);

bool isValidClockSelection(char clockNum);

uint32_t getSourceFrequency(char src);

void disableClock(char clockNum);

void startClock(char clockNum);

void initClock(char clockNum, uint32_t frequency, bool mash, char clockSource);

void disableAllClocks();

#endif
