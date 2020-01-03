#include <stdio.h>
#include "bcm/pimem.h"
#include "bcm/gpio.h"
#include "bcm/clk.h"
#include "bcm/pcm.h"// "pmod.h"
#include "globals.h"

#define POLL_MODE      0
#define INTERRUPT_MODE 1
#define DMA_MODE	   2

// possibly only runs if DMA is initialized before GPIO?
int main(int agrc, char ** argv) {
	//initGPIO();
	pcmExternInterface pmodMaster;
	pmodMaster.isMasterDevice = 1;
	pmodMaster.numChannels = 2;
	pmodMaster.ch1Pos = 1;
	pmodMaster.inputOnFallingEdge = 1;
	pmodMaster.dataWidth = 16;
	initPCM(&pmodMaster, 1, DMA_MODE, 1);
	initGPIO();
	// set all PCM GPIO regs to their ALT0 func
    for (int i = 18; i <= 21; i++) {
        setPinMode(i, 4);
        if (DEBUG) printf("pin %d set to ALT0\n", i);
    }
	initClock(0, 11289600, 1, PLLD);
	startClock(0);
	startPCM();
	return 0;
}
