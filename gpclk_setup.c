
/* Nick Nagy
Setting GPIO clocks using BCM2835 ARM Peripherals Guide.
*/

#include <fcntl.h>
#include <sys/mman.h>
#include "gpclk_setup.h"

/*bool clock_is_busy(int * clock_ctrl_reg) {
    return (((*clock_ctrl_reg)>>7) & 1);
}

// assumes mash=1
int set_div(unsigned int freq) {
    int divi = OSC_FREQ / freq;
    return (divi<<12) & (4096 * (freq - divi));
}

// uses source 1 (oscillator, 19.2MHz)
void set_clock_freqs(unsigned int mclk_freq) {
    // master clock
    if (!clock_is_busy(mclk_ctrl_reg)) {
        *mclk_div_reg = PASSWD & set_div(mclk_freq);
    } else {
        printf("Failure: master clock is busy.\n");
        return;
    }
    // slave clock
    if (!clock_is_busy(sclk_ctrl_reg)) {
        *sclk_div_reg = PASSWD & set_div(mclk_freq>>6);
    } else {
        printf("Failure: slave clock is busy.\n");
        return;
    }
    // left-right clock
    if (!clock_is_busy(lrclk_ctrl_reg)) {
        *lrclk_div_reg = PASSWD & set_div(mclk_freq>>9);
    } else {
        printf("Failure: left-right clock is busy.\n");
    }
}

void init_clocks() {//unsigned int mclk_freq) {
    if (!clocks_initialized) {
        *((int *)GPFSEL_REG) = CLOCKS_FSEL; // set the GPIO pins to function as clocks
        // set src to 1 (oscillator). Set enable AFTER src, password and mash
        *mclk_ctrl_reg = PASSWD & MASH & 1; 
        *mclk_ctrl_reg &= ENABLE;
        *mclk_div_reg = PASSWD;
        *sclk_ctrl_reg = PASSWD & MASH &1;
        *sclk_ctrl_reg = ENABLE;
        *sclk_div_reg = PASSWD;
        *lrclk_ctrl_reg = PASSWD & MASH & 1;
        *lrclk_ctrl_reg = ENABLE;
        *lrclk_div_reg = PASSWD;
        clocks_initialized = 1;
    }
}*/

int main() {
    //init_clocks();
    //set_clock_freqs(MCLK_FREQ);
    int fdgpio = open("/dev/gpiomem", O_RDWR);
    if (fdgpio < 0) {
        printf("Failure.\n"); 
    }
    unsigned int * gpio_clk_ctrl = mmap((int *)CM_GP0CTL, 20, PROT_READ|PROT_WRITE, MAP_SHARED, fdgpio, 0);
    unsigned int * gpio = mmap((int *)GPFSEL_REG, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE, fdgpio, 0);
    printf("gpio clock control is mapped at address %p\n", gpio_clk_ctrl);
    printf("gpiomem is mapped at address %p\n", gpio);
    return 0;
}


