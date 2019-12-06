/* Nick Nagy
Setting GPIO clocks using BCM2835 ARM Peripherals Guide.
*/

#include "gpio_ctrl.h"
#include "i2s_ctrl.h"

bool clocksInitialized = 0;
unsigned int * gpio;
unsigned int * i2s;
unsigned int * clk_ctrl;

bool clockIsBusy(int clock_ctrl_reg) {
    return (((clock_ctrl_reg)>>7) & 1);
}

// assumes mash=1
int setDiv(unsigned int freq) {
    int divi = OSC_FREQ / freq;
    return (divi<<12) | (4096 * (freq - divi));
}

void setPinHigh(unsigned char n) {
    gpio[GPIO_SET_REG] |= (1 << n);
}

void setPinLow(unsigned char n) {
    gpio[GPIO_CLR_REG] |= (1 << n);
}

/*
DMA Mode
1) set EN bit. Assert RXCLR & TXCLR, wait 2 clk cycles for FIFOs to reset
2) set DMAEN to enable DMA DREQ generation, set RXREQ/TXREQ to dermine DREQ FIFO thresholds
3) set DREQ channels, one for TX and one for RX. Start the DMA
4) set TXON and RXON to begin operation
*/
void initI2S(unsigned char frameSize, unsigned char channelWidth) {
    // set GPIO 18-21 to I2S mode
    gpio[0] |= PCM_CLK_FSEL_BITS;
    gpio[1] |= PCM_DATA_FSEL_BITS;
    // setup PCM_FS & channel width bits
    switch(channelWidth) {
        case 16: break;
        default: break;
    }
    // DMA setup
    i2s[CTRL_STATUS] |= 0x19; // enable interface, and assert TXCLR & RXCLR
    while((i2s[CTRL_STATUS] & 0x1000000)); // wait 2 clk cycles for SYNC
    i2s[CTRL_STATUS] |= 512;
    i2s[DREQ_LVL] = // set RX & TX request levels
    
    i2s[CTRL_STATUS] |= 6; // set TXON and RXON to begin operation
}

// uses source 1 (oscillator, 19.2MHz)
void setClockFreqs(unsigned int mclk_freq) {
    // master clock
    if (!clockIsBusy(clk_ctrl[MCLK_IDX])) {
        clk_ctrl[MCLK_IDX + 1] = PASSWD | setDiv(mclk_freq);
    } else {
        printf("Failure: master clock is busy.\n");
        return;
    }
    // slave clock
    if (!clockIsBusy(clk_ctrl[SCLK_IDX])) {
        clk_ctrl[SCLK_IDX + 1] = PASSWD | setDiv(mclk_freq>>6);
    } else {
        printf("Failure: slave clock is busy.\n");
        return;
    }
    // left-right clock
    if (!clock_is_busy(clk_ctrl[LRCLK_IDX])) {
        clk_ctrl[LRCLK_IDX + 1] = PASSWD | set_div(mclk_freq>>9);
    } else {
        printf("Failure: left-right clock is busy.\n");
    }
}

void initClocks() {
    if (!clocksInitialized) {
        gpio[0] |= CLOCK_FSEL_BITS;
        // set src to 1 (oscillator). Set enable AFTER src, password and mash
        for (int i = MCLK_IDX; i <= LRCLK_IDX; i+=2) {
            clk_ctrl[i] = PASSWD | MASH | 1;
            clk_ctrl[i] |= ENABLE;
            clk_ctrl[i+1] = PASSWD;
        }
        clocksInitialized = 1;
    }
}

void LEDTest(unsigned char n, unsigned int delay_seconds) {
    // set pin to output mode (001)
    unsigned int delay_us = delay_seconds * 1000000;
    gpio[(int)n/10] |= 1 << (3*(n % 10));
    for (int i = 0; i < 20; i++) {
<<<<<<< HEAD
	printf("i=%d\n",i);
        set_pin_high(n);
=======
        setPinHigh(n);
>>>>>>> ffde285b8bc226ad3a3eb4043d3c3bd671bd6be9
        usleep(delay_us);//delay(1);
        setPinLow(n);
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
    i2s = (int *)((char *)gpio + 170); // GPIO_BASE + 0x3000 bytes //mmap((int *)I2S_BASE, I2S_PAGESIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fdgpio, 0);
    initClocks();
    setClockFreqs(MCLK_FREQ);
    // testing LED @ pin 8
    LEDTest(8, 1);
    munmap(gpio, GPIO_BASE_PAGESIZE);
    munmap(i2s, I2S_PAGESIZE);
    munmap(clk_ctrl, CLK_CTRL_BASE_PAGESIZE);
    return 0;
}


