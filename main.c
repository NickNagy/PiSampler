#include <stdio.h>
#include "bcm/pimem.h"
#include "bcm/gpio.h"
#include "bcm/clk.h"
#include "bcm/pcm.h"// "pmod.h"
#include "globals.h"

#define POLL_MODE      0
#define INTERRUPT_MODE 1
#define DMA_MODE	   2

int main(int agrc, char ** argv) {
	/* initGPIO();
	pcmExternInterface pmodMaster;
	DMAControlBlock cb;
	pmodMaster.isMasterDevice = 1;
	pmodMaster.numChannels = 2;
	pmodMaster.ch1Pos = 1;
	pmodMaster.inputOnFallingEdge = 1;
	pmodMaster.dataWidth = 16;
	initClock(0, 11289600, 1, PLLD);
	startClock(0);
	initPCM(&pmodMaster, 1, DMA_MODE, 1, &cb);
	startPCM();
	while(1);*/
	DMAControlBlock cb;
	DMAControlBlock * cbPtr = &cb;
	if (DEBUG) printf("pointer before passed = %p\n", cbPtr);
	cbPtr = (DMAControlBlock *)getAlignedPointer((void*)cbPtr, 32);
	return 0;
}
