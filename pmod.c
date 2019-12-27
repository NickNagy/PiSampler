#include "pmod.h"

static char dataWidth = 16;

static void determineDataWidth() {

}

static char determineFrameLength() {

}

char getDataWidth() {
    return dataWidth; 
}

/*
mode:
    0 = PMOD is in master mode
    1 = PMOD is in slave mode

pcmMode:
    0 = PCM is in polled mode
    1 = PCM is in interrupt mode
    2 = PCM is in DMA mode

dataWidth:

*/
void initPMOD(bool mode, char pcmMode, char thresh) {//char dataWidth) {
    initClock(0, 112896000, 1, PLLD); // TODO: generalize to make more options for MCLK freq
    startClock(0);
    switch(mode) {
        case 0: {
            // frame length irrelevant for master mode
            if (DEBUG) printf("PMOD in master mode.\n");
            initPCM(pcmMode, 1, 0, PMOD_CHANNELS, 0, dataWidth, 1, thresh);
            break;
        }
        default: {
            // need to set PCM clk and PCM_FS clk on Pi...
            // also need to figure out a formula for frameSyncLength and dataWidth...
            if (DEBUG) printf("PMOD in slave mode.\n");
            determineDataWidth();
            initPCM(pcmMode, 0, 0, PMOD_CHANNELS, determineFrameLength(), dataWidth, 1, thresh);
            break;
        }
    }
}
