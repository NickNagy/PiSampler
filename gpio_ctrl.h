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
//#include <bcm_host.h>

/*
#define PI_2_BASE 0x20000000
#define PI_3_BASE 0x3F000000


// set to proper board
#define BCM_BASE PI_3_BASE
*/

// Pi crystal oscillator
#define OSC_FREQ 19200000

#define DATA_SIZE 16

/* 
******************** CLOCK CONTROL ************************
Registers and relevant bit assignments for setting up GPCLK
*/

// function select register for GPCLK0, GPCLK1, GPCLK2
#define CLK_CTRL_BASE_OFFSET 0x101070 //CLK_CTRL_BASE (BCM_BASE + 0x101070)
#define CLK_CTRL_BASE_PAGESIZE 24

// input for *GPFSEL0. Sets all 3 GPCLKs to their clock function
#define CLOCK_FSEL_BITS 0x124000

#define CLK0_IDX 0
#define CLK1_IDX 2
#define CLK2_IDX 4

// relevant clock frequencies
#define MCLK_FREQ 11289000
#define MCLK_LRCLK_RATIO 256 // =(2^val). may have to be empirically determined
#define PCM_FS_FREQ (MCLK_FREQ / (2*MCLK_LRCLK_RATIO))
#define SCLK_FREQ PCM_FS_FREQ*DATA_SIZE

#define CLK_PASSWD 0x5A000000
#define MASH 512
#define ENABLE 16
#define SRC 1

#define CLK_CTRL_INIT_BITS (CLK_PASSWD | MASH | SRC)

/*
*********************** I2S CONTROL ************************
Registers and relevant bit assignments for setting up I2S
*/

#define I2S_BASE_OFFSET 0x203000 //BCM_BASE + 0x203000
#define I2S_PAGESIZE 24

// register indices in I2S memory map array (FIFO_REG @ I2S_BASE + 0x4, MODE_REG @ I2S_BASE + 0x8, etc)
#define CTRL_STATUS_REG 0
#define FIFO_REG 1
#define MODE_REG 2
#define RX_CONFIG_REG 3
#define TX_CONFIG_REG 4
#define DREQ_LVL_REG 5
#define INTERRUPT_EN_REG 6
#define INTERRUPT_STATUS_REG 7
#define GRAY_MODE_REG 8 

// input for FSEL, set GPIO 18 - 21 to I2S function
#define PCM_CLK_FSEL_BITS 0x24000000 // for FSEL0
#define PCM_DATA_FSEL_BITS 0x24 // for FSEL1 

// MODE REGISTER BITS
#define FRAME_SYNC_MODE 1 // 0 = master (we generate frame sync), 1 = slave (frame sync is an input) 
#define FSLEN 2*DATA_SIZE // irrelevant when FRAME_SYNC_MODE == 0
#define FLEN 2*FSLEN // irrelevant when FRAME_SYNC_MODE == 0
#define MODE_REG_BITS (FRAME_SYNC_MODE << 21) | (FLEN<<10) | FSLEN

// TX & RX BITS
# define CH1POS 1 // PMOD MSB bit is read after first complete SCLK cycle when LRCLK changes states
# define CH1WID 0 // total bits = 8 + CHWID + (CHWEX * 16)
# define CH2WID CH1WID
# define CH2POS CH1POS + CH1WID
# define RX_CONFIG_REG_BITS (1<<30) | (CH1POS << 20) | (CH1WID << 16) | (1 << 14) | (CH2POS << 4) | (CH2WID) // reg[30] & reg[14] are enable bits
# define TX_CONFIG_REG_BITS RX_CONFIG_REG_BITS

/*
********************** GPIO CONTROL ************************
All other GPIO-relevant registers and bit assignments
*/

// gpio
#define GPIO_BASE_OFFSET 0x200000//(BCM_BASE + 0x200000)// 0x7E200000
#define GPIO_BASE_PAGESIZE 4096

#define GPIO_SET_REG 7  // turns GPIO pins high
#define GPIO_CLR_REG 10 // turns GPIO pins low
#define GPIO_LEV_REG 13 // when in input mode, bit n reads GPIO pin n

bool clockIsBusy(int clock_ctrl_reg);

void setPinHigh(unsigned char n);

void setPinLow(unsigned char n);

int setDiv(unsigned int freq);

void setClockFreqs(unsigned int mclk_freq);

void initI2S(unsigned char frameSize, unsigned char channelWidth);

void initClocks();

void LEDTest(unsigned char pin, unsigned char numBlinks, unsigned int delay_seconds);

#endif
