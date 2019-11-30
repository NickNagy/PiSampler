/* Nick Nagy
Setting GPIO clocks using BCM2835 ARM Peripherals Guide.
*/

#ifndef GPCLK_SETUP_H
#define GPCLK_SETUP_H

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define PI_2_BASE 0x20000000
#define PI_3_BASE 0xF2000000

// BCM base
#define BCM_BASE PI_3_BASE

// I2S base
#define I2S_BASE (BCM_BASE + 203000)

// I2S
#define CTRL_STATUS 0
#define FIFO_REG 1
#define MODE_REG 2
#define RX_CONFIG 3
#define TX_CONFIG 4
#define DMA_REQ_LVL 5
#define INTERRUPT_EN 6
#define INTERRUPT_STATUS 7
#define GRAY_MODE_REG 8
#define I2S_PAGESIZE 24

// clock function select register for GPCLK0, GPCLK1, GPCLK2
#define CLK_CTRL_BASE (BCM_BASE + 0x101070)
#define CLK_CTRL_BASE_PAGESIZE 20

// input for *GPFSEL0. Sets all 3 GPCLKs to their clock function
#define CLOCK_FSEL_BITS (4<<18) + (4<<15) + (4<<12)//0x124000

#define MCLK_IDX 0
#define SCLK_IDX 2
#define LRCLK_IDX 4

// gpio
#define GPIO_BASE (BCM_BASE + 0x200000)// 0x7E200000
#define GPIO_BASE_PAGESIZE 4096

#define GPIO_SET_REG 7  // turns GPIO pins high
#define GPIO_CLR_REG 10 // turns GPIO pins low
#define GPIO_LEV_REG 13 // when in input mode, bit n reads GPIO pin n

#define PASSWD 0x5A000000
#define MASH 512
#define ENABLE 16

#define OSC_FREQ 19200000
#define MCLK_FREQ 11289000

bool clock_is_busy(int clock_ctrl_reg);

void set_pin_high(unsigned char n);

void set_pin_low(unsigned char n);

int set_div(unsigned int freq);

void set_clock_freqs(unsigned int mclk_freq);

void init_clocks();

#endif
