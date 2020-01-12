#include "dma.h"

static void * virtPage;
static void * physPage;
//static uint32_t numControlBlocks;

volatile uint32_t * initDMAMap(char numDMARegs) {
    return (volatile uint32_t *)initMemMap(DMA_BASE_OFFSET, numDMARegs * (uint32_t)DMA_SINGLE_REG_MAPSIZE);
}

/*
transferInfo: use macros defined in header file!

srcAddr: source address
destAddr: destination address
bytesToTransfer: transfer length
arePhysAddr:
    0 = both srcAddr and destAddr are virtual addresses
    1 = srcAddr is a virtual address, destAddr is a physical address
    2 = srcAddr is a virtual physical, destAddr is a virtual address
    3 = both srcAddr and destAddr are physical addresses
loop:
    the returned DMAControlBlock * should point back to itself

*/
DMAControlBlock * initDMAControlBlock(uint32_t transferInfo, uint32_t * srcAddr, uint32_t * destAddr, uint32_t bytesToTransfer, char arePhysAddr, bool loop) {
    if (!virtPage) {
        initVirtPhysPage(&virtPage, &physPage);
    }
    switch(arePhysAddr) {
        case 1: {
            srcAddr = (uint32_t *)virtToPhys(srcAddr);
            break;
        }
        case 2: {
            destAddr = (uint32_t *)virtToPhys(destAddr);
            break;
        }
        case 3: {
            srcAddr = (uint32_t *)virtToPhys(srcAddr);
            destAddr = (uint32_t *)virtToPhys(destAddr);
            break;
        }
        default: break;
    }

    DMAControlBlock * cb;// = (DMAControlBlock *)((char *)virtPage + numControlBlocks*sizeof(DMAControlBlock)); //don't want to overwrite control blocks
    //numControlBlocks++;
    //printf("control block initialized at virtual address %p\n", cb);

    cb -> transferInfo = transferInfo; // TODO
    cb -> srcAddr = (uint32_t)srcAddr;
    cb -> destAddr = (uint32_t)destAddr;
    cb -> transferLength = bytesToTransfer;
    cb -> stride = 0;
    cb -> reserved[1] = 0;
    cb -> reserved[0] = 0;

    if (loop) {
        cb -> nextControlBlockAddr = cb; // TODO: physical address of itself, or virtual??
    } else {
        cb -> nextControlBlockAddr = 0; // will terminate after operation is finished
    }

    return cb;
}
