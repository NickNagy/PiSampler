#include "clk.h"

//static bool clockMapInitialized = 0;
static unsigned * clkMap = initMemMap(CLK_CTRL_BASE_OFFSET, CLK_CTRL_BASE_MAPSIZE);
static char clocksInitialized;
static char clocksRunning;

// assumes mash = 0 or mash = 1
static int setDiv(unsigned freq, bool mash) {
    float diviFloat = (float)clockSourceFreq / freq;
    int divi = (int)diviFloat;
    if (mash) 
        return (divi << 12) | (int)(4096 * (diviFloat - divi));
    return divi << 12;
}

int getSourceFrequency(char src) {
    switch (src) {
        case OSC: return OSC_FREQ;
        case PLLC: return PLLC_FREQ;
        case PLLD: return PLLD_FREQ;
        default: return 0;
    }
}

bool clockIsBusy(int clock_ctrl_reg) {
    return (((clock_ctrl_reg)>>7) & 1);
}

bool isValidClockSelection(char clockNum) {
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
    if (clkMap[CLK_CTRL_REG(clockNum)] & BUSY) {
        do {
            printf("test...\n");
            clkMap[CLK_CTRL_REG(clockNum)] &= ~ENABLE;
        }
        while(clkMap[CLK_CTRL_REG(clockNum)] & BUSY);
    }
    clocksRunning &= ~(1 << clockNum);
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

int initClock(char clockNum, unsigned frequency, bool mash, char clockSource) {
    //if (!clockMapInitialized) {
    //    
    //    clockMapInitialized = 1;
    //}
    if (!isValidClockSelection(clockNum)) {
        printf("Not a valid clock selection.\n");
        return 1;
    }
    char sourceFrequency = getSourceFrequency(clockSource);
    if (!sourceFrequency) {
        printf("Not a valid source.\n");
        return 1;
    }
    if((clocksRunning >> clockNum) & 1) 
        disableClock(clockNum);
    clkMap[CLK_DIV_REG(clockNum)] = (CLK_PASSWD | setDiv(frequency, mash));
    usleep(10);
    clkMap[CLK_CTRL_REG(clockNum)] = (CLK_PASSWD | MASH(mash) | clockSource);
    usleep(10);
    printf("clock %d is set to run @ %d Hz.\n", clockNum, frequency);
    PRINT_REG("CTL", clkMap[CLK_CTRL_REG(clockNum)]);
    PRINT_REG("DIV", clkMap[CLK_DIV_REG(clockNum)]);
    clocksInitialized |= 1 << clockNum;
    if (VERBOSE) {
        printf("Clocks running = %d, clocks initialized = %d\n", clocksRunning, clocksInitialized);
    }
    return 0;
}

int startClock(char clockNum) {
    if ((clocksRunning >> clockNum) & 1) {
        printf("Clock %d is already running. You will have to disable it first.\n", clockNum);
        return 1;
    }
    if (!((clocksInitialized >> clockNum) & 1)) {
        printf("ERROR: clock %d hasn't been initialized.\n", clockNum);
        return 1;
    }
    if (VERBOSE) {
        unsigned divi = (clkMap[CLK_DIV_REG(clockNum)] >> 12) & 0xFFF;
        unsigned divf = clkMap[CLK_DIV_REG(clockNum)] & 0xFFF; 
        unsigned clockFreq = (unsigned) clockSourceFreq / (divi + (divf/4096.0));
        printf("Starting clock %d @ %d Hz...\n", clockNum, clockFreq);
    }
    clocksRunning |= (1 << clockNum);
    clkMap[CLK_CTRL_REG(clockNum)] |= (CLK_PASSWD | ENABLE);
    setPinMode(clockNum + 4, 4);
    return 0;
}