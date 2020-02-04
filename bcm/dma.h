#ifndef DMA_H
#define DMA_H

#include "pimem.h"

// "DMA directly connected to peripherals. Must be set up to use physical hardware addresses of peripherals"

#define DMA_BASE_OFFSET  0x007000
#define DMA_MAPSIZE      0x1000
#define DMA_SINGLE_REG_MAPSIZE 0x100

#define VC_BUS_BASE 0xC0000000

#define DMA_CHANNEL(x) x*64

#define DMA_CS_REG(x)        DMA_CHANNEL(x)
#define DMA_CONBLK_AD_REG(x) DMA_CHANNEL(x) + 1
#define DMA_TI_REG(X)        DMA_CHANNEL(x) + 2
#define DMA_SOURCE_AD_REG(x) DMA_CHANNEL(x) + 3
#define DMA_DEST_AD_REG(x)   DMA_CHANNEL(x) + 4
#define DMA_TXFR_LEN_REG(x)  DMA_CHANNEL(x) + 5
#define DMA_STRIDE_REG(x)    DMA_CHANNEL(x) + 6
#define DMA_NEXTCONBK_REG(x) DMA_CHANNEL(x) + 7
#define DMA_DEBUG_REG(x)     DMA_CHANNEL(x) + 8

#define DMA_GLOBAL_ENABLE_REG   0xFF0>>2

// Control status register bits
#define DMA_RESET 1<<31
#define DMA_ABORT 1<<30

// Debug register bits
#define DMA_READ_ERROR 4
#define DMA_FIFO_ERROR 2
#define DMA_READ_LAST_NOT_SET_ERROR 1

/* More unadressed control status bits to handle...

bool waitForOutstandingWrites: (x << 28)
    0 = ?
    1 = after DMA transfer, will wait until last outstanding write response has been receieved;
        while waiting, will load next CB address, clear active flag
        will defer setting INT or END flags until last outstanding write response received

char priorityLevels: (x << 16)
    top 4 bits = PANIC_PRIORITY: priority of AXI bus transactions
                                 used when PANIC bit of peripheral address == 1
    bottom 4 bits = PRIORITY:    priority of normal AXI bus transactions
                                 used when PANIC bit of peripheral address == 0

*/

/* ****TRANSFER_INFO REGISTER****
27 - 31: reserved
26: NO_WIDE_BURSTS: manual suggests to set to 0
21 - 25: WAITS: set the number of wait cycles after each read/write operation
16 - 20: PERMAP:
    0 = continuous, un-paced transfer
    1 - 31 = peripheral # whose ready signal is used to control rate of transfers,
             and whose panic signal(s) will be output on DMA AXI bus
12 - 15: BURST_LENGTH: number of words in a "burst". 0 = single-word transfers
11: SRC_IGNORE: 1 = don't perform src reads.
10: SRC_DREQ: 
    0 = DREQ has no effect
    1 = DREQ selected by PERMAP will "gate" the src reads
9: SRC_WIDTH:
    0 = 32b source read width
    1 = 128b source read width
8: SRC_INC:
    1:
        (SRC_WIDTH == 0) will increment src address by 4 bytes
        (SRC_WIDTH == 1) will increment src address by 32 bytes
7: DEST_IGNORE
6: DEST_DREQ
5: DEST_WIDTH
4: DEST_INC
3: WAIT_RESP:
    0 = don't wait for write response, continue as soon as data is sent
    1 = wait for write response to be received before proceeding
2: 0 (don't care)
1: TDMODE:
    0 = linear mode: interpret TXFR_LEN register as single transfer of length = {YLENGTH, XLENGTH}
    1 = 2D mode: interpret TXFR_LEN register as YLENGTH # of XLENGTH-sized transfers. Add strides to addr after each transfer
0: INTEN: 1 = generate an interrupt when transfer described by current CB completes
*/
#define RXPERMAP 3     
#define TXPERMAP 2

#define PERMAP(x)      x<<16
#define SRC_IGNORE     1<<11
#define SRC_DREQ       1<<10
#define SRC_WIDTH(x)   x<<9
#define SRC_INC        1<<8
#define DEST_IGNORE    1<<7
#define DEST_DREQ      1<<6
#define DEST_WIDTH(x)  x<<5
#define DEST_INC       1<<4
#define WAIT_RESP(x)   x<<3
#define TD_MODE(x)     x<<1

#define USE_DIRECT_UNCACHED 1

/* ***** CONTROL BLOCK ******
transferLength: (in bytes)!
    top 16 bits: yTransferLength: 
        (in 2D mode) indicates how many xTransferLength transfers performed.
        (in normal mode) becomes the top bits of xTransferLength
    bottom 16 bits: xTransferLength

srcStride: signed (2's comp) byte increment to apply to 
    the source address at the end of each row in 2D mode
*/

// TODO
/*#define PRINT_CTRL_BLK(name, cb) printf("Address of %s = %p\n\tTransfer info = %x\n\tSource address = %x\n\tDestination address = %x\n\tTransfer length = %x\n\tSource stride = %x\n\tDest stride = %x\n\tNext CB address = %x\n",
                           name, cb, cb->transferInfo, cb->srcAddr, cb->destAddr, cb->transferLength, cb->srcStride, cb->destStride, cb->nextControlBlockAddr);
#define DEBUG_CTRL_BLK(name, cb) if (DEBUG) PRINT_CTRL_BLK(name, cb)*/

typedef struct DMAControlBlock {
    uint32_t transferInfo;
    uint32_t srcAddr;
    uint32_t destAddr;
    uint32_t transferLength;
    uint32_t stride;
    uint32_t nextControlBlockAddr;
    uint64_t reserved;
} DMAControlBlock;

typedef struct DMAControlPageWrapper {
    //DMAControlBlock * cbPage;
    VirtToBusPages pages;
    uint32_t controlBlocksTotal;
    uint32_t controlBlocksUsed;
} DMAControlPageWrapper;

volatile uint32_t * initDMAMap(char numDMARegs);

DMAControlPageWrapper initDMAControlPage(uint32_t numControlBlocks);

void clearDMAControlPage(DMAControlPageWrapper * cbWrapper);

void linkDMAControlBlocks (DMAControlPageWrapper * cbWrapper, uint32_t cb1, uint32_t cb2);

void initDMAControlBlock (DMAControlPageWrapper * cbWrapper, uint32_t transferInfo, uint32_t physSrcAddr, uint32_t physDestAddr, uint32_t bytesToTransfer);

void insertDMAControlBlock(DMAControlPageWrapper * cbWrapper, uint32_t transferInfo, uint32_t physSrcAddr, uint32_t physDestAddr, uint32_t bytesToTransfer, uint32_t position);

void initDMAChannel(DMAControlBlock * physCB, uint8_t dmaCh);

void startDMAChannel(uint8_t dmaCh);

uint32_t debugDMA(uint8_t dmaCh);

#endif
