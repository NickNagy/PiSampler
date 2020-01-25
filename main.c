#include <stdio.h>
#include "bcm/pimem.h"
#include "bcm/gpio.h"
#include "bcm/clk.h"
#include "bcm/pcm.h"
#include "globals.h"

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
	
	initClock(0, 11289000, 1, PLLD);
	
	initPCM(&pmod, 0, 1);
	
	sleep(1);
	
	startPCM();
	
	closeFiles();
}
