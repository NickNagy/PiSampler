/* Nick Nagy
Setting GPIO clocks using BCM2835 ARM Peripherals Guide.
*/

#include "gpio_ctrl.h"

bool clocks_initialized = 0;
unsigned int * gpio;
unsigned int * clk_ctrl;

bool clock_is_busy(int clock_ctrl_reg) {
    return (((clock_ctrl_reg)>>7) & 1);
}

// assumes mash=1
int set_div(unsigned int freq) {
    int divi = OSC_FREQ / freq;
    return (divi<<12) | (4096 * (freq - divi));
}

void set_pin_high(unsigned char n) {
    gpio[GPIO_SET_REG] |= (1 << n);
}

void set_pin_low(unsigned char n) {
    gpio[GPIO_CLR_REG] |= (1 << n);
}

// uses source 1 (oscillator, 19.2MHz)
void set_clock_freqs(unsigned int mclk_freq) {
    // master clock
    if (!clock_is_busy(clk_ctrl[MCLK_IDX])) {
        clk_ctrl[MCLK_IDX + 1] = PASSWD | set_div(mclk_freq);
    } else {
        printf("Failure: master clock is busy.\n");
        return;
    }
    // slave clock
    if (!clock_is_busy(clk_ctrl[SCLK_IDX])) {
        clk_ctrl[SCLK_IDX + 1] = PASSWD | set_div(mclk_freq>>6);
    } else {
        printf("Failure: slave clock is busy.\n");
        return;
    }
    // left-right clock
    if (!clock_is_busy(clk_ctrl[LRCLK_IDX])) {
        clk_ctrl[LRCLK_IDX + 1] = PASSWD | set_div(mclk_freq>>9);
    	printf("lef-right clock @ address %p running at %dHz.\n", &clk_ctrl[LRCLK_IDX], mclk_freq>>9);
    } else {
        printf("Failure: left-right clock is busy.\n");
    }
}

void init_clocks() {
    if (!clocks_initialized) {
        gpio[0] |= CLOCK_FSEL_BITS;
        // set src to 1 (oscillator). Set enable AFTER src, password and mash
        for (int i = MCLK_IDX; i <= LRCLK_IDX; i+=2) {
            clk_ctrl[i] = PASSWD | MASH | 1;
            clk_ctrl[i] |= ENABLE;
            clk_ctrl[i+1] = PASSWD;
        }
        clocks_initialized = 1;
    }
}

void led_test(unsigned char n, unsigned int delay_seconds) {
    // set pin to output mode (001)
    unsigned int delay_us = delay_seconds * 1000000;
    gpio[(int)n/10] |= 1 << (3*(n % 10));
    for (int i = 0; i < 20; i++) {
        set_pin_high(n);
        usleep(delay_us);//delay(1);
        set_pin_low(n);
        usleep(delay_us);
    }
}

int main() {
    int fdgpio = open("/dev/gpiomem", O_RDWR);
    if (fdgpio < 0) {
        printf("Failure to access /dev/gpiomem.\n"); 
    }
    gpio = mmap((int *)GPIO_BASE, GPIO_BASE_PAGESIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fdgpio, 0);
    clk_ctrl = mmap((int *)CLK_CTRL_BASE, CLK_CTRL_BASE_PAGESIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fdgpio, 0);
    init_clocks();
    set_clock_freqs(MCLK_FREQ);
    // testing LED @ pin 8
    led_test(8, 1);
    munmap(gpio, GPIO_BASE_PAGESIZE);
    munmap(clk_ctrl, CLK_CTRL_BASE_PAGESIZE);
    return 0;
}


