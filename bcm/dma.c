#include "dma.h"

static void * virtPage;
static void * physPage;
static unsigned numControlBlocks;

volatile unsigned * initDMAMap(char numDMARegs) {
    return (volatile unsigned *)initMemMap(DMA_BASE_OFFSET, numDMARegs * (unsigned)DMA_SINGLE_REG_MAPSIZE);
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
DMAControlBlock * initDMAControlBlock(unsigned transferInfo, unsigned * srcAddr, unsigned * destAddr, unsigned bytesToTransfer, char arePhysAddr, bool loop) {
    if (!virtPage) {
        initVirtPhysPage(&virtPage, &physPage);
    }
    switch(arePhysAddr) {
        case 1: {
            srcAddr = (unsigned *)virtToPhys(srcAddr);
            break;
        }
        case 2: {
            destAddr = (unsigned *)virtToPhys(destAddr);
            break;
        }
        case 3: {
            srcAddr = (unsigned *)virtToPhys(srcAddr);
            destAddr = (unsigned *)virtToPhys(destAddr);
            break;
        }
        default: break;
    }

    DMAControlBlock * cb = (DMAControlBlock *)((char *)virtPage + numControlBlocks*sizeof(DMAControlBlock)); //don't want to overwrite control blocks
    numControlBlocks++;
    printf("control block initialized at virtual address %p\n", cb);

    cb -> transferInfo = transferInfo; // TODO
    cb -> srcAddr = (unsigned)srcAddr;
    cb -> destAddr = (unsigned)destAddr;
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
