#include <stdio.h>
#include "bcm/pimem.h"
#include "bcm/gpio.h"
#include "bcm/clk.h"
#include "bcm/pcm.h"
#include "globals.h"

#define MCLK 0
#define MCLK_FREQ 11289000

int main(int agrc, char ** argv) {
	pcmExternInterface pmod;
	
	// PMOD interface in master mode
	pmod.ch1Pos = 0;
	pmod.dataWidth = 16;
	pmod.frameLength = 64;
	pmod.numChannels = 2;
	pmod.inputOnFallingEdge = 0;
	pmod.isMasterDevice = 1;
	
	openFiles();
	initClock(MCLK, MCLK_FREQ, 1, PLLD);
	
	//initPCM(&pmod, 0, 1);
	sleep(1);
	
	startClock(MCLK);
	//startPCM();
	
	freeClockMem();
	closeFiles();
}
