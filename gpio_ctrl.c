/* Nick Nagy
Setting GPIO clocks using BCM2835 ARM Peripherals Guide.
*/

#include "gpio_ctrl.h"

bool clocksInitialized = 0;
int sourceFreq;
unsigned int * gpio;
unsigned int * clk_ctrl;

static int getSourceFrequency() {
    switch (SRC) {
        case OSC: return OSC_FREQ;
        case PLLC: return PLLC_FREQ;
        case PLLD: return PLLD_FREQ;
        case HDMI: return HDMI_FREQ;
        default: return 0;
    }
}

static bool clockIsBusy(int clock_ctrl_reg) {
    return (((clock_ctrl_reg)>>7) & 1);
}

int setDiv(unsigned int freq) {
    float diviFloat = (float)sourceFreq / freq;
    int divi = (int)diviFloat;
    if (MASH) 
        return (divi << 12) | (int)(4096 * (diviFloat - divi));
    return divi << 12;
}

void setPinHigh(unsigned char n) {
    gpio[GPIO_SET_REG] |= (1 << n);
}

void setPinLow(unsigned char n) {
    gpio[GPIO_CLR_REG] |= (1 << n);
}

static void initClocks() {
    if (!clocksInitialized) {
        gpio[0] |= CLK0_FSEL_BITS;
        // disable clock first
        while(clk_ctrl[CLK0_CTRL_REG] & BUSY) {
            clk_ctrl[CLK0_CTRL_REG] = (CLK_PASSWD | KILL); //& ~ENABLE);
        }
        clk_ctrl[CLK0_DIV_REG] = (CLK_PASSWD | setDiv(MCLK_FREQ));
        usleep(10);
        clk_ctrl[CLK0_CTRL_REG] = (CLK_PASSWD | MASH | SRC); //CLK_CTRL_INIT_BITS;
        usleep(10);
        clk_ctrl[CLK0_CTRL_REG] |= (CLK_PASSWD | ENABLE);
        printf("clock 0 is set to run @ %d Hz. ctrl reg @ %p = %x, div reg @ %p = %x\n", MCLK_FREQ, &clk_ctrl[CLK0_CTRL_REG], clk_ctrl[CLK0_CTRL_REG], &clk_ctrl[CLK0_DIV_REG], clk_ctrl[CLK0_DIV_REG]);
        clk_ctrl[CLK0_CTRL_REG] |= (CLK_PASSWD | KILL);
        clocksInitialized = 1;
    }
}

int main() {
    unsigned int bcm_base = bcm_host_get_peripheral_address();
    int fdgpio = open("/dev/gpiomem", O_RDWR);
    if (fdgpio < 0) {
        printf("Failure to access /dev/gpiomem.\n"); 
    }
    clk_ctrl = (unsigned int *)mmap((unsigned int*)(bcm_base + CLK_CTRL_BASE_OFFSET), CLK_CTRL_BASE_PAGESIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fdgpio, 0);
    gpio = (unsigned int *)mmap((unsigned int*)(bcm_base + GPIO_BASE_OFFSET), GPIO_BASE_PAGESIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fdgpio, 0);
    sourceFreq = getSourceFrequency();
    printf("running system off of source #%d at frequency %dHz\n", SRC, sourceFreq);
    int divTest = setDiv(MCLK_FREQ);
    printf("div = %x\n", divTest);
    initClocks();
    printf("clocks initialized.\n");
    munmap(gpio, GPIO_BASE_PAGESIZE);
    munmap(clk_ctrl, CLK_CTRL_BASE_PAGESIZE);
    return 0;
}
