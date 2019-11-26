/* Nick Nagy
Setting GPIO clocks using BCM2835 ARM Peripherals Guide.
*/

#ifndef GPCLK_SETUP_H
#define GPCLK_SETUP_H

#include <stdio.h>
#include <stdbool.h>

// clock function select register for GPCLK0, GPCLK1, GPCLK2
#define GPFSEL_REG 0x7E200000

// input for *GPFSEL_REG. Sets all 3 to clock function
#define CLOCKS_FSEL 0x049000

// clock control
#define CM_GP0CTL 0x7e101070
#define CM_GP1CTL 0x7e101078
#define CM_GP2CTL 0x73101080

// clock divisors
#define CM_GP0DIV 0x7e101074
#define CM_GP1DIV 0x7e10107c
#define CM_GP2DIV 0x7e101084

#define PASSWD 0x5A000000
#define MASH 512
#define ENABLE 16

#define OSC_FREQ 19200000
#define MCLK_FREQ 11289000

// globals, registers
bool clocks_initialized = 0;
int * mclk_ctrl_reg = (int *)CM_GP0CTL; 
int * mclk_div_reg = (int *)CM_GP0DIV;
int * sclk_ctrl_reg = (int *)CM_GP1CTL;
int * sclk_div_reg = (int *)CM_GP1DIV;
int * lrclk_ctrl_reg = (int *)CM_GP2CTL;
int * lrclk_div_reg = (int *)CM_GP2DIV;

bool clock_is_busy(int * clock_ctrl_reg);

int set_div(unsigned int freq);

void set_clock_freqs(unsigned int mclk_freq);

void init_clocks();

#endif