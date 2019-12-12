/* Nick Nagy
Setting GPIO clocks using BCM2835 ARM Peripherals Guide.
*/

#include "gpio_ctrl.h"

bool clocksInitialized = 0;
unsigned int * gpio;
unsigned int * i2s;
unsigned int * clk_ctrl;

bool clockIsBusy(int clock_ctrl_reg) {
    return (((clock_ctrl_reg)>>7) & 1);
}

// assumes mash=1
int setDiv(unsigned int freq) {
    int divi = (OSC_FREQ / freq) << 12;
    if (MASH) 
        return divi | (4096 * (freq - divi));
    return divi;
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
    i2s[CTRL_STATUS_REG] |= 0x19; // enable interface, and assert TXCLR & RXCLR
    while((i2s[CTRL_STATUS_REG] & 0x1000000)); // wait 2 clk cycles for SYNC
    i2s[CTRL_STATUS_REG] |= 512;
    i2s[DREQ_LVL_REG] = 0; // set RX & TX request levels TODO
    i2s[CTRL_STATUS_REG] |= 6; // set TXON and RXON to begin operation
}

// uses source 1 (oscillator, 19.2MHz)
void setClockFreqs(unsigned int mclk_freq) {
    // master clock
    if (!clockIsBusy(clk_ctrl[CLK0_CTRL_REG])) {
        clk_ctrl[CLK0_CTRL_REG + 1] = CLK_PASSWD | setDiv(mclk_freq);
    } else {
        printf("Failure: master clock is busy.\n");
        return;
    }
    // slave clock
    if (!clockIsBusy(clk_ctrl[CLK1_CTRL_REG])) {
        clk_ctrl[CLK1_CTRL_REG + 1] = CLK_PASSWD | setDiv(mclk_freq>>6);
    } else {
        printf("Failure: slave clock is busy.\n");
        return;
    }
    // left-right clock
    if (!clockIsBusy(clk_ctrl[CLK2_CTRL_REG])) {
        clk_ctrl[CLK2_CTRL_REG+ 1] = CLK_PASSWD | setDiv(mclk_freq>>9);
    } else {
        printf("Failure: left-right clock is busy.\n");
        return;
    }
}

void initClocks() {
    if (!clocksInitialized) {
        gpio[0] |= CLK0_FSEL_BITS;
        // disable clock first
        while(clk_ctrl[CLK0_CTRL_REG] & BUSY) {
            clk_ctrl[CLK0_CTRL_REG] = (CLK_PASSWD | KILL); //& ~ENABLE);
        }
        printf("clock disabled.\n");
        clk_ctrl[CLK0_DIV_REG] = (CLK_PASSWD | setDiv(MCLK_FREQ));
        usleep(10);
        clk_ctrl[CLK0_CTRL_REG] = CLK_CTRL_INIT_BITS;
        usleep(10);
        clk_ctrl[CLK0_CTRL_REG] |= (CLK_PASSWD | ENABLE);
        printf("clock 0 is set to run @ %d Hz. ctrl reg @ %p = %x, div reg @ %p = %x\n", MCLK_FREQ, &clk_ctrl[CLK0_CTRL_REG], clk_ctrl[CLK0_CTRL_REG], &clk_ctrl[CLK0_DIV_REG], clk_ctrl[CLK0_DIV_REG]);
        clocksInitialized = 1;
    }
}

void LEDTest(unsigned char pin, unsigned char numBlinks, unsigned int delay_seconds) {
    // set pin to output mode (001)
    unsigned int delay_us = delay_seconds * 1000000;
    gpio[(int)pin/10] |= 1 << (3*(pin % 10));
    printf("LEDTest: FSEL0 reg @ address %p = %x\n", gpio, gpio[0]);
    for (int i = 0; i < numBlinks; i++) {
        printf("i=%d\n",i);
        setPinHigh(pin);
        usleep(delay_us);//delay(1);
        setPinLow(pin);
        usleep(delay_us);
    }
}

int main() {
    unsigned int bcm_base = bcm_host_get_peripheral_address();
    unsigned int pg_size = (unsigned)sysconf(_SC_PAGE_SIZE);
    printf("page size = %d\n", pg_size);
    int fdgpio = open("/dev/gpiomem", O_RDWR);
    if (fdgpio < 0) {
        printf("Failure to access /dev/gpiomem.\n"); 
    }
    clk_ctrl = (unsigned int *)mmap((unsigned int*)(bcm_base + CLK_CTRL_BASE_OFFSET), CLK_CTRL_BASE_PAGESIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fdgpio, 0);
    gpio = (unsigned int *)mmap((unsigned int*)(bcm_base + GPIO_BASE_OFFSET), GPIO_BASE_PAGESIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fdgpio, 0);
    initClocks();
    printf("clocks initialized.\n");
    //LEDTest(8, 3, 1);
    munmap(gpio, GPIO_BASE_PAGESIZE);
    munmap(clk_ctrl, CLK_CTRL_BASE_PAGESIZE);
    return 0;
}
