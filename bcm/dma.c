#include "dma.h"

unsigned * initDMAMap(char numDMARegs) {
    return initMemMap(DMA_BASE_OFFSET, numDMARegs * (unsigned)DMA_SINGLE_REG_MAPSIZE);
}