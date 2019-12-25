#ifndef DMA_H
#define DMA_H

// "DMA directly connected to peripherals. Must be set up to use physical hardware addresses of peripherals"

#define DMA_BASE_OFFSET  0x7000
#define DMA_BASE_MAPSIZE 0x80 // each channel register set is 0x100 bytes, and I only need 2 channels

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

// Control status register bits
#define RESET 1<<31
#define ABORT 1<<30

#endif