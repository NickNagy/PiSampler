#include "pmod.h"

/*
mode:
    0 = PMOD is in master mode
    1 = PMOD is in slave mode

pcmMode:
    0 = PCM is in polled mode
    1 = PCM is in interrupt mode
    2 = PCM is in DMA mode
*/
void initPMOD(bool mode, char pcmMode, char dataWidth) {
    switch(mode) {
        case 0: {

        }
        default: {
            // need clock source from PI
            initClock(0, 11289000, 1); // master clock
            // ...
            startClock(0);
        }
    }
}