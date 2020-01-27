#include "clk.h"

static volatile uint32_t * clkMap = 0;
static char clocksInitialized;
static char clocksRunning;

// assumes mash = 0 or mash = 1
static int32_t setDiv(uint32_t targetFreq, uint32_t sourceFreq, bool mash) {
    float diviFloat = (float)sourceFreq / targetFreq;
    uint32_t divi = (uint32_t)diviFloat;
    if (mash) 
        return (divi << 12) | (uint32_t)(4096 * (diviFloat - divi));
    return divi << 12;
}

uint32_t getSourceFrequency(char src) {
    switch (src) {
        case OSC: return OSC_FREQ;
        case PLLC: return PLLC_FREQ;
        case PLLD: return PLLD_FREQ;
        default: return 0;
    }
}

bool clockIsBusy(int32_t clock_ctrl_reg) {
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

void initClock(char clockNum, uint32_t frequency, bool mash, char clockSource) {
    if (!clkMap)
        clkMap = initMemMap(CLK_CTRL_BASE_OFFSET, CLK_CTRL_BASE_MAPSIZE);

    if (!isValidClockSelection(clockNum))
        FATAL_ERROR("Not a valid clock selection.");
    DEBUG_VAL("clock source", clockSource);

    int32_t sourceFrequency = getSourceFrequency(clockSource);
    DEBUG_VAL("source frequency", sourceFrequency);
    if (!sourceFrequency)
        FATAL_ERROR("Not a valid source.");
        
    setPinMode(clockNum + 4, 4);

    if((clocksRunning >> clockNum) & 1) 
        disableClock(clockNum);

    clkMap[CLK_DIV_REG(clockNum)] = (CLK_PASSWD | setDiv(frequency, sourceFrequency, mash));
    usleep(100);
    clkMap[CLK_CTRL_REG(clockNum)] = (CLK_PASSWD | MASH(mash) | clockSource);
    usleep(100);
    printf("clock %d is set to run @ %d Hz.\n", clockNum, frequency);
    
    DEBUG_REG("CTL", clkMap[CLK_CTRL_REG(clockNum)]);
    DEBUG_REG("DIV", clkMap[CLK_DIV_REG(clockNum)]);
    clocksInitialized |= 1 << clockNum;
    if (VERBOSE)
        printf("Clocks running = %d, clocks initialized = %d\n", clocksRunning, clocksInitialized);
}

void startClock(char clockNum) {
    if ((clocksRunning >> clockNum) & 1) {
        printf("ERROR: clock %d is already running. You will have to disable it first.\n", clockNum);
        exit(1);
    }
    if (!((clocksInitialized >> clockNum) & 1)) {
        printf("ERROR: clock %d hasn't been initialized.\n", clockNum);
        exit(1);
    }
    clocksRunning |= (1 << clockNum);
    clkMap[CLK_CTRL_REG(clockNum)] |= (CLK_PASSWD | ENABLE);
    printf("Clock started.\n");
}
