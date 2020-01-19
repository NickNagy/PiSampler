#include "dma.h"
#include "pimem.h"

static void * virtPage;
static void * physPage;
static uint32_t totalDefinedControlBlocks;

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
DMAControlPageWrapper * initDMAControlPage(uint32_t numControlBlocks) {
    DMAControlPageWrapper * cbWrapper;
    
    size_t size = ceilToPage(numControlBlocks * 32);
    
    VirtToBusPages vtp = initUncachedMemView(size, USE_DIRECT_UNCACHED);
    cbWrapper->pages = &vtp;

    //cbWrapper -> cbPage = (DMAControlBlock *)(pages->virtAddr);

    //DEBUG_VAL("address of page", &(cbWrapper -> cbPage));
    cbWrapper -> controlBlocksTotal = numControlBlocks;
    cbWrapper -> controlBlocksUsed = 0;
    return cbWrapper;
}

// doesn't actually delete the struct or ptr, but frees up the cbPage mem
void clearDMAControlPage(DMAControlPageWrapper * cbWrapper) {
    clearUncachedMemView(cbWrapper -> pages);
    cbWrapper -> controlBlocksTotal = 0;
    cbWrapper -> controlBlocksUsed = 0;
}

/*
set the nextControlBlockAddr of cb1 to point to cb2
*/
void linkDMAControlBlocks (DMAControlPageWrapper * cbWrapper, uint32_t cb1, uint32_t cb2) {
    if (cb1 > cbWrapper->controlBlocksTotal || cb2 > cbWrapper-> controlBlocksTotal) {
        ERROR_MSG("Control block does not exist in this page!");
        exit(1);
    }
    DMAControlBlock * cbVirt = (DMAControlBlock *)(cbWrapper -> pages -> virtAddr);
    DMAControlBlock * cbBus = (DMAControlBlock *)(cbWrapper -> pages -> busAddr);
    cbVirt[cb1].nextControlBlockAddr = (uint32_t)&(cbBus[cb2]);
}

/*
adds a control block to the nth position of cbPage, where n == controlBlocksUsed

make sure addresses passed in this function are PHYSICAL addresses!
*/
void initDMAControlBlock (DMAControlPageWrapper * cbWrapper, uint32_t transferInfo, uint32_t physSrcAddr, uint32_t physDestAddr, uint32_t bytesToTransfer) {
    if (cbWrapper -> controlBlocksTotal == cbWrapper -> controlBlocksUsed) {
        ERROR_MSG("This page is already full!");
        exit(1);
    }
    uint32_t idx = cbWrapper -> controlBlocksUsed;
    DMAControlBlock * cbVirt = (DMAControlBlock *)(cbWrapper -> pages -> virtAddr);
    cbVirt[idx].transferInfo = transferInfo;
    cbVirt[idx].srcAddr = physSrcAddr;
    cbVirt[idx].destAddr = physDestAddr;
    cbVirt[idx].transferInfo = bytesToTransfer;
    cbWrapper -> controlBlocksUsed++;
}

/*
inserts a new DMAControlBlock into the cbPage with the given transferInfo, srcAddr, destAddr, bytesToTransfer
inserted at position such that 
    cb[position - 1].nextControlBlockAddr = &cb[position], 
and 
    cb[position].nextControlBlockAddr = &cb[position+1]
*/
void insertDMAControlBlock(DMAControlPageWrapper * cbWrapper, uint32_t transferInfo, uint32_t physSrcAddr, uint32_t physDestAddr, uint32_t bytesToTransfer, uint32_t position) {
    if (position > cbWrapper -> controlBlocksUsed) {
        ERROR_MSG("Not enough control blocks have been initialized for this insert!");
        exit(1);
    }
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
    dmaMap[DMA_CS_REG(dmaCh)] = DMA_RESET;
	sleep(1);
	
	// clear debug flags
	dmaMap[DMA_DEBUG_REG(dmaCh)] = 7;

    dmaMap[DMA_CONBLK_AD_REG(dmaCh)] = (uint32_t)physCB;
}

/* start the dma channel */
void startDMAChannel(uint8_t dmaCh) {
    if (!((dmaMap[DMA_GLOBAL_ENABLE_REG] >> dmaCh) & 1)) {
        ERROR_MSG("You need to initialize this channel first!");
        exit(1);
    }
    dmaMap[DMA_CS_REG(dmaCh)] = 1; // activate channel
}
