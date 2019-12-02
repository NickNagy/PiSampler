/*
Nick Nagy

When PMOD I2S2 is in master mode, it internally determines its LRCLK and SCLK frequencies based on the MCLK input.
With MCLK = 11.289MHz, LRCLK = 44.1kHz and SCLK = 32*LRCLK (for 16b data)

Because LRCLK completes 1 period every time it reads from both channels, whereas PCM_FS (on the Pi) reads from both channels
each time it changes states, PCM_FS should be set to half of the frequency of LRCLK. For the example above, this would be 22.05kHz


*/

#ifndef I2S_CTRL_H
#define I2S_CTRL_H

#include "gpio_ctrl.h"

#define I2S_BASE BCM_BASE + 0x203000
#define I2S_PAGESIZE 24

// register indices in I2S memory map array (FIFO_REG @ I2S_BASE + 0x4, MODE_REG @ I2S_BASE + 0x8, etc)
#define CTRL_STATUS 0
#define FIFO_REG 1
#define MODE_REG 2
#define RX_CONFIG 3
#define TX_CONFIG 4
#define DREQ_LVL 5
#define INTERRUPT_EN 6
#define INTERRUPT_STATUS 7
#define GRAY_MODE_REG 8

// input for FSEL, set GPIO 18 - 21 to I2S function
#define PCM_CLK_FSEL_BITS 0x24000000 // for FSEL0
#define PCM_DATA_FSEL_BITS 0x24 // for FSEL1 

// PCM_FS
#define FRAME_SYNC_MODE 1 // 0 = master (we generate frame sync), 1 = slave (frame sync is an input) 
// FSLEN & FLEN are not used when FRAM_SYNC_MODE == 1
#define FSLEN 32
#define FLEN 2*FSLEN

// TX & RX
# define CH1POS 1 // PMOD MSB bit is read after first complete SCLK cycle when LRCLK changes states
# define CH1WID 0 // total bits = 8 + CHWID + (CHWEX * 16)
# define CH2WID CH1WID
# define CH2POS CH1POS + CH1WID
// reg[30] & reg[14] are enable bits
# define RX_REG_BITS (1<<30) | (CH1POS << 20) | (CH1WID << 16) | (1 << 14) | (CH2POS << 4) | (CH2WID)
# define TX_REG_BITS RX_REG_BITS

void initI2S();

#endif