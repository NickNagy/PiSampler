#include <stdio.h>
#include "bcm/pimem.h"
#include "bcm/gpio.h"
//#include "bcm/clk.h"
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
	pmod.isDoubleSpeedMode = 0;
	
	openFiles();
	initPCM(&pmod, 0x3F, 1, 1);

	startPCM();

	// free mem
	/*freePCM();
	freeDMA();
	freeClocks();
	freeGPIO();*/
	closePCM();
	closeFiles();
}
