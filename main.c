#include <stdio.h>
#include "bcm/pimem.h"
#include "bcm/gpio.h"
#include "bcm/clk.h"
#include "bcm/pcm.h"
#include "globals.h"

int main(int agrc, char ** argv) {
	
	int dmaCh = 5;
	
	volatile uint32_t * dmaMap = initMemMap(DMA_BASE_OFFSET, DMA_MAPSIZE);

	void * virtSrcPage, *physSrcPage;
	initVirtPhysPage(&virtSrcPage, &physSrcPage);

	void * virtDestPage, *physDestPage;
	initVirtPhysPage(&virtDestPage, &physDestPage);

	char *srcArr = (char *)virtSrcPage;
	srcArr[0] = 'h';
	srcArr[1] = 'e';
	srcArr[2] = 'l';
	srcArr[3] = 'l';
	srcArr[4] = 'o';
	srcArr[5] = ' ';
	srcArr[6] = 'w';
	srcArr[7] = 'o';
	srcArr[8] = 'r';
	srcArr[9] = 'l';
	srcArr[10] = 'd';
	srcArr[11] = '!';
	srcArr[12] = 0;
	
	void * virtCBPage, *physCBPage;
	initVirtPhysPage(&virtCBPage, &physCBPage);
	
	DMAControlBlock * cb = (DMAControlBlock *)virtCBPage;
	cb -> transferInfo = SRC_INC | DEST_INC;
	cb -> srcAddr = (uint32_t)physSrcPage;
	cb -> destAddr = (uint32_t)physDestPage;
	cb -> transferLength = 13;
	cb -> stride = 0;
	cb -> reserved = 0;
	cb -> nextControlBlockAddr = 0;
	
	//enable DMA channel
	dmaMap[DMA_GLOBAL_ENABLE_REG] |= (1 << dmaCh);
	
	/* initDMAControlBlock() may cause issues by re-defining ptr addresses */
	//(DMAControlBlock *)virtCBPage = initDMAControlBlock(transferInfo, (uint32_t)physSrcPage, (uint32_t)physDestPage, 13, 3);
	
	//DMAControlBlock * cb = initDMAControlBlock(transferInfo, (uint32_t)physSrcPage, (uint32_t)physDestPage, 13, 3);

	dmaMap[DMA_CS_REG(dmaCh)] = DMA_RESET;
	sleep(1);
	
	// clear debug flags
	dmaMap[DMA_DEBUG_REG(dmaCh)] = 7; //DMA_READ_ERROR | DMA_FIFO_ERROR | DMA_READ_LAST_NOT_SET_ERROR;

	// need to fix my initDMAControlBlock func --> we need the physical address!
	// alternatively, can just to VirtToPhys, but probably not as efficient
	dmaMap[DMA_CONBLK_AD_REG(dmaCh)] = (uint32_t)physCBPage; // temporarily...
	dmaMap[DMA_CS_REG(dmaCh)] = 1; // activate DMA channel...

	sleep(1);

	printf("destination reads: '%s'\n", (char*)virtDestPage);

	//clearVirtPhysPage(virtSrcPage, BCM_PAGESIZE);
	//clearVirtPhysPage(virtDestPage, BCM_PAGESIZE);
	munmap((void *)dmaMap, DMA_MAPSIZE);
}
