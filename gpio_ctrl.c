/* Nick Nagy
Setting GPIO clocks using BCM2835 ARM Peripherals Guide.
*/

#include "gpio_ctrl.h"

static char clocksInitialized;
static char clocksRunning;
static char clockSource;
static unsigned clockSourceFreq;

static volatile unsigned int bcm_base = 0;
static volatile unsigned int * gpio = 0;
static volatile unsigned int * clk_ctrl = 0;

static int getSourceFrequency(char src) {
    switch (src) {
        case OSC: return OSC_FREQ;
        case PLLC: return PLLC_FREQ;
        case PLLD: return PLLD_FREQ;
        default: return 0;
    }
}

static bool setPinMode(char pin, char mode) {
    if (mode > 7) {
        printf("ERROR: INVALID MODE.\n");
        return 0;
    }
    char fsel_reg = pin/10;
    char pinR = pin % 10;
    gpio[fsel_reg] = (gpio[fsel_reg] & FSEL_CLEAR_PIN_BITS(pinR)) | (mode << FSEL_SHIFT(pinR));
    return 1;
}

static bool clockIsBusy(int clock_ctrl_reg) {
    return (((clock_ctrl_reg)>>7) & 1);
}

// assumes mash = 0 or mash = 1
int setDiv(unsigned freq, bool mash) {
    float diviFloat = (float)clockSourceFreq / freq;
    int divi = (int)diviFloat;
    if (mash) 
        return (divi << 12) | (int)(4096 * (diviFloat - divi));
    return divi << 12;
}

void setPinHigh(char n) {
    gpio[GPIO_SET_REG] |= (1 << n);
}

void setPinLow(char n) {
    gpio[GPIO_CLR_REG] |= (1 << n);
}

static bool isValidClockSelection(char clockNum) {
    switch(clockNum) {
        case 0: {
            printf("Clock 0 is available for user\n");
            return 1;
        }
        case 1: { // TODO: add model dependencies
            printf("Clock 1 is not available for user.\n");
            return 0;
        }
        case 2: { // TODO: add model dependencies
            printf("Clock 2 is available for user\n"); 
            return 1;
        }
        default: {
            printf("Invalid clock.\n");
            return 0;
        }
    }
}

void disableClock(char clockNum) {
    if (clk_ctrl[CLK_CTRL_REG(clockNum)] & BUSY) {
        do {
            printf("test...\n");
            clk_ctrl[CLK_CTRL_REG(clockNum)] &= CLK_DISABLE_BITS;
        }
        while(clk_ctrl[CLK_CTRL_REG(clockNum)] & BUSY);
    }
    clocksRunning &= ~(1 << clockNum);
}

static int initClock(char clockNum, unsigned frequency, bool mash) {
    if (!isValidClockSelection(clockNum)) return 1;
    if((clocksRunning >> clockNum) & 1) 
        disableClock(clockNum);
    clk_ctrl[CLK_DIV_REG(clockNum)] = (CLK_PASSWD | setDiv(frequency, mash));
    usleep(10);
    clk_ctrl[CLK_CTRL_REG(clockNum)] = (CLK_PASSWD | MASH(mash) | clockSource);
    usleep(10);
    printf("clock %d is set to run @ %d Hz.\n", clockNum, frequency);
    PRINT_REG("CTL", clk_ctrl[CLK_CTRL_REG(clockNum)]);
    PRINT_REG("DIV", clk_ctrl[CLK_DIV_REG(clockNum)]);
    clocksInitialized |= 1 << clockNum;
    if (VERBOSE) {
        printf("Clocks running = %d, clocks initialized = %d\n", clocksRunning, clocksInitialized);
    }
    return 0;
}

static int startClock(char clockNum) {
    if ((clocksRunning >> clockNum) & 1) {
        printf("Clock %d is already running. You will have to disable it first.\n", clockNum);
        return 1;
    }
    if (!((clocksInitialized >> clockNum) & 1)) {
        printf("ERROR: clock %d hasn't been initialized.\n", clockNum);
        return 1;
    }
    if (VERBOSE) {
        unsigned divi = (clk_ctrl[CLK_DIV_REG(clockNum)] >> 12) & 0xFFF;
        unsigned divf = clk_ctrl[CLK_DIV_REG(clockNum)] & 0xFFF; 
        unsigned clockFreq = (unsigned) clockSourceFreq / (divi + (divf/4096.0));
        printf("Starting clock %d @ %d Hz...\n", clockNum, clockFreq);
    }
    clocksRunning |= (1 << clockNum);
    clk_ctrl[CLK_CTRL_REG(clockNum)] |= (CLK_PASSWD | ENABLE);
    setPinMode(clockNum + 4, 4);
    return 0;
}

void LEDTest(char pin, unsigned char numBlinks, unsigned delay_seconds) {
    unsigned delay_us = 1000000 * delay_seconds;
    int fsel_reg = pin/10;
    int pinR = pin % 10;
    setPinMode(pin, 1);
    if (VERBOSE) PRINT_REG("FSEL", gpio[fsel_reg]);
    for (char i = 0; i < numBlinks; i++) {
        printf("%d\n",i);
        setPinHigh(pin);
        usleep(delay_us);
        setPinLow(pin);
        usleep(delay_us);
    }
}

static bool initMemMap() {
    unsigned bcm_base = bcm_host_get_peripheral_address();
    int fd;
    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        printf("Failure to access /dev/mem\n"); 
        return 0;
    }
    gpio = (unsigned *)mmap(0, GPIO_BASE_MAPSIZE, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED|MAP_LOCKED, fd, GPIO_BASE);
    printf("GPIO loaded from /dev/mem beginning at address %p.\n", gpio);
    clk_ctrl = (unsigned *)mmap(0, CLK_CTRL_BASE_MAPSIZE, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED|MAP_LOCKED, fd, CLK_CTRL_BASE);
    printf("clock control and div registers loaded from /dev/mem beginning @ address %p.\n", clk_ctrl);
    return 1;
}

static void clearMemMap() {
    munmap((void*)gpio, GPIO_BASE_MAPSIZE);
    munmap((void*)clk_ctrl, CLK_CTRL_BASE_MAPSIZE);
}

void disableAllClocks() {
    if (VERBOSE)
        printf("Disabling all clocks...\n");
    for (int i = 0; i < 3; i++) {
        if ((clocksRunning >> i) & 1)
            disableClock(i);
    }
    if (VERBOSE)
        printf("Clocks running = %d\n", clocksRunning);
    clocksInitialized = 0;
}

int main(int argc, char** argv) {
    if(!initMemMap()) return 1;
    clockSource = PLLD;//OSC;
    clockSourceFreq = getSourceFrequency(clockSource);
    printf("running system off of source #%d at frequency %d Hz\n", clockSource, clockSourceFreq);
    clocksRunning = 7; // 0b111, init all 3 gpclks to assumed running
    initClock(0, 11289000, 1);
    startClock(0);
    LEDTest(8, 10, 1);
    disableAllClocks();
    PRINT_REG("Ctrl", clk_ctrl[CLK_CTRL_REG(0)]);
    PRINT_REG("Div", clk_ctrl[CLK_DIV_REG(0)]);
    clearMemMap();
    return 0;
}
