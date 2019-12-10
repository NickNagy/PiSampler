/* Nick Nagy
Setting GPIO clocks using BCM2835 ARM Peripherals Guide.
*/

#include "gpio_ctrl.h"

bool clocksInitialized = 0;
unsigned int * gpio;
unsigned int * i2s;
unsigned int * clk_ctrl;

unsigned int addressDiff = 0; // for debugging

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
    i2s[CTRL_STATUS_REG] |= 0x19; // enable interface, and assert TXCLR & RXCLR
    while((i2s[CTRL_STATUS_REG] & 0x1000000)); // wait 2 clk cycles for SYNC
    i2s[CTRL_STATUS_REG] |= 512;
    i2s[DREQ_LVL_REG] = 0; // set RX & TX request levels TODO
    i2s[CTRL_STATUS_REG] |= 6; // set TXON and RXON to begin operation
}

// uses source 1 (oscillator, 19.2MHz)
void setClockFreqs(unsigned int mclk_freq) {
    // master clock
    if (!clockIsBusy(clk_ctrl[CLK0_IDX])) {
        clk_ctrl[CLK0_IDX + 1] = CLK_PASSWD | setDiv(mclk_freq);
    } else {
        printf("Failure: master clock is busy.\n");
        return;
    }
    // slave clock
    if (!clockIsBusy(clk_ctrl[CLK1_IDX])) {
        clk_ctrl[CLK1_IDX + 1] = CLK_PASSWD | setDiv(mclk_freq>>6);
    } else {
        printf("Failure: slave clock is busy.\n");
        return;
    }
    // left-right clock
    if (!clockIsBusy(clk_ctrl[CLK2_IDX])) {
        clk_ctrl[CLK2_IDX + 1] = CLK_PASSWD | setDiv(mclk_freq>>9);
    } else {
        printf("Failure: left-right clock is busy.\n");
        return;
    }
}

void initClocks() {
    if (!clocksInitialized) {
        gpio[0] |= CLOCK_FSEL_BITS;
        printf("initClocks: FSEL0 reg @ address %p = %x\n", gpio - addressDiff, gpio[0]);
        // set src to 1 (oscillator). Set enable AFTER src, password and mash
        for (int i = CLK0_IDX; i < CLK1_IDX; i+=2) {//i <= CLK2_IDX; i+=2) {
            clk_ctrl[i] |= CLK_PASSWD | MASH | SRC;
            printf("clock ctrl reg @ address%p = %x\n", &clk_ctrl[i], clk_ctrl[i]);
            //clk_ctrl[i] |= ENABLE;
            clk_ctrl[i+1] |= CLK_PASSWD;
            printf("clock div reg @ address%p = %x\n", &clk_ctrl[i+1], clk_ctrl[i + 1]);
        }
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

void clockTest(unsigned int freq) {
    gpio[0] |= CLOCK_FSEL_BITS;
    clk_ctrl[0] |= CLK_CTRL_INIT_BITS;
    clk_ctrl[1] |= CLK_PASSWD | setDiv(freq);
    clk_ctrl[0] |= ENABLE;
}

int main() {
    int fdgpio = open("/dev/gpiomem", O_RDWR);
    if (fdgpio < 0) {
        printf("Failure to access /dev/gpiomem.\n"); 
    }
    gpio = mmap((int *)GPIO_BASE, GPIO_BASE_PAGESIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fdgpio, 0);
    clk_ctrl = mmap((int *)CLK_CTRL_BASE, CLK_CTRL_BASE_PAGESIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fdgpio, 0);
    printf("address of clk_ctrl = %p\n", clk_ctrl);
    //i2s = (unsigned int *)((char *)gpio + 170); // GPIO_BASE + 0x3000 bytes //mmap((int *)I2S_BASE, I2S_PAGESIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fdgpio, 0);
    //initClocks();
    //setClockFreqs(MCLK_FREQ);
    // testing LED @ pin 8
    clockTest(MCLK_FREQ);
    LEDTest(8, 20, 1);
    clk_ctrl[0] ^= ENABLE; //disable
    munmap(gpio, GPIO_BASE_PAGESIZE);
    munmap(clk_ctrl, CLK_CTRL_BASE_PAGESIZE);
    return 0;
}
