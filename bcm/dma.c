#include "dma.h"
#include "pimem.h"

static volatile uint32_t * dmaMap = 0;

/* 
NOTE: I can no longer vouch for this function --> I would call initMemMap directly and use DMA_MAPSIZE for size,
as this will also load your DEBUG and global ENABLE regs
*/
/* volatile uint32_t * initDMAMap(char numDMARegs) {
    return initMemMap(DMA_BASE_OFFSET, numDMARegs * (uint32_t)DMA_SINGLE_REG_MAPSIZE);
}*/

/* 
TODO: might want to change to be a void() func that passes a CB wrapper as an input

initializes and returns a ptr to a DMAControlPageWrapper struct.
The struct holds a DMAControlBlock * pointing to contiguous uncached memory, the size of which is determined by numControlBlocks
It also stores two unsigned ints: controlBlocksTotal (== numControlBlocks), and controlBlocksUsed (which is updated as DMAControlBlocks are initialized in this wrapper)
*/
DMAControlPageWrapper initDMAControlPage(uint32_t numControlBlocks) {
    DMAControlPageWrapper cbWrapper;
    
    size_t size = ceilToPage(numControlBlocks * 32);
    
    cbWrapper.pages = initUncachedMemView(size, USE_DIRECT_UNCACHED);
    
    cbWrapper.controlBlocksTotal = numControlBlocks;
    cbWrapper.controlBlocksUsed = 0;
    
    DEBUG_MSG("successful return");
    
    return cbWrapper;
}

// doesn't actually delete the struct or ptr, but frees up the cbPage mem
void clearDMAControlPage(DMAControlPageWrapper * cbWrapper) {
    clearUncachedMemView(&(cbWrapper -> pages));
    cbWrapper -> controlBlocksTotal = 0;
    cbWrapper -> controlBlocksUsed = 0;
}

/*
set the nextControlBlockAddr of cb1 to point to cb2
*/
void linkDMAControlBlocks (DMAControlPageWrapper * cbWrapper, uint32_t cb1, uint32_t cb2) {
    if (cb1 > cbWrapper->controlBlocksUsed || cb2 > cbWrapper-> controlBlocksUsed)
        FATAL_ERROR("Control block does not exist in this page!")
    DMAControlBlock * cbVirt = (DMAControlBlock *)(cbWrapper -> pages.virtAddr);
    DMAControlBlock * cbBus = (DMAControlBlock *)(cbWrapper -> pages.busAddr);
    cbVirt[cb1].nextControlBlockAddr = (uint32_t)&(cbBus[cb2]);
}

/*
adds a control block to the nth position of cbPage, where n == controlBlocksUsed

make sure addresses passed in this function are PHYSICAL addresses!
*/
void initDMAControlBlock (DMAControlPageWrapper * cbWrapper, uint32_t transferInfo, uint32_t physSrcAddr, uint32_t physDestAddr, uint32_t bytesToTransfer) {
    if (cbWrapper -> controlBlocksTotal == cbWrapper -> controlBlocksUsed)
        FATAL_ERROR("This page is already full!");
    uint32_t idx = cbWrapper -> controlBlocksUsed;
    DMAControlBlock * cbVirt = (DMAControlBlock *)(cbWrapper -> pages.virtAddr);
    cbVirt[idx].transferInfo = transferInfo;
    cbVirt[idx].srcAddr = physSrcAddr;
    cbVirt[idx].destAddr = physDestAddr;
    cbVirt[idx].transferLength = bytesToTransfer;
    cbWrapper -> controlBlocksUsed++;
}

/*
NOTE: this function doesn't make sense for my current setup -> need to choose a consistent implementation for CB wrappers!

inserts a new DMAControlBlock into the cbPage with the given transferInfo, srcAddr, destAddr, bytesToTransfer
inserted at position such that 
    cb[position - 1].nextControlBlockAddr = &cb[position], 
and 
    cb[position].nextControlBlockAddr = &cb[position+1]
*/
void insertDMAControlBlock(DMAControlPageWrapper * cbWrapper, uint32_t transferInfo, uint32_t physSrcAddr, uint32_t physDestAddr, uint32_t bytesToTransfer, uint32_t position) {
    if (position > cbWrapper -> controlBlocksUsed)
        FATAL_ERROR("Not enough control blocks have been initialized for this insert!")
    initDMAControlBlock(cbWrapper, transferInfo, physSrcAddr, physDestAddr, bytesToTransfer);
    linkDMAControlBlocks(cbWrapper, position-1, position);
    linkDMAControlBlocks(cbWrapper, position, position+1);
}

/*
Set the DMA control regs for dmaCh and set the starting control block address to the physical uncached address of cb
*/
void initDMAChannel(DMAControlBlock * physCB, uint8_t dmaCh) {
    if (!dmaMap) {
        dmaMap = initMemMap(DMA_BASE_OFFSET, DMA_MAPSIZE);
    }
    //enable channel
    dmaMap[DMA_GLOBAL_ENABLE_REG] |= (1 << dmaCh);
    dmaMap[DMA_CS_REG(dmaCh)] |= DMA_ABORT;
    dmaMap[DMA_CS_REG(dmaCh)] = DMA_DISDEBUG | (0xFF << 16) | DMA_RESET; // set to max AXI priority levels
	sleep(1);
	
	// clear debug flags
	dmaMap[DMA_DEBUG_REG(dmaCh)] = 7;

    dmaMap[DMA_CONBLK_AD_REG(dmaCh)] = (uint32_t)physCB;
}

/* start the dma channel */
void startDMAChannel(uint8_t dmaCh) {
    if (!((dmaMap[DMA_GLOBAL_ENABLE_REG] >> dmaCh) & 1))
        FATAL_ERROR("You need to initialize this channel first!")
    dmaMap[DMA_CS_REG(dmaCh)] = 1; // activate channel
}

uint32_t debugDMA(uint8_t dmaCh) {
    return dmaMap[DMA_DEBUG_REG(dmaCh)];
}

/* make sure to also clear DMA control pages separately */
void freeDMA() {
    clearMemMap((void*)dmaMap, DMA_MAPSIZE);
}
