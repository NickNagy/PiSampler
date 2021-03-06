#ifndef PCM_H
#define PCM_H

#include "pimem.h"
#include "gpio.h"
#include "dma.h"
#include "clk.h"
#include "../globals.h"

#define PCM_BASE_OFFSET 0x203000
#define PCM_BASE_MAPSIZE 0x24

#define PCM_CTRL_REG   0
#define PCM_FIFO_REG   1
#define PCM_MODE_REG   2
#define PCM_RXC_REG    3
#define PCM_TXC_REG    4
#define PCM_DREQ_REG   5
#define PCM_INTEN_REG  6
#define PCM_INTSTC_REG 7
#define PCM_GRAY_REG   8

#define TXCLR 8
#define RXCLR 16
#define STBY  (1<<25)

#define PCM_CLK_CTL_IDX 5

// bitwise-AND with registers --> avoids changing RO bits
#define CLEAR_CTRL_BITS 0x7E6000

// bitwise-OR with CTRL register
#define RXON     2
#define TXON     4
#define RXONTXON 6
// bitwise-AND with CTRL register
#define RXOFFTXOFF 0xFFFFFFF9
#define RXOFF      0xFFFFFFFD
#define TXOFF      0xFFFFFFFB

#define RXFULL  pcmMap[PCM_CTRL_REG] & 0x40000
#define TXEMPTY pcmMap[PCM_CTRL_REG] & 0x20000

// DMA channels for FIFO
#define PCM_DMA_CHANNEL 5

typedef struct pcmExternInterface {
    uint16_t sampleFreq;
    uint8_t ch1Pos;
    uint8_t dataWidth;
    uint8_t frameLength;
    uint8_t numChannels;
    bool inputOnFallingEdge;
    bool isMasterDevice;
    bool isDoubleSpeedMode;
} pcmExternInterface;

static bool checkFrameAndChannelWidth(pcmExternInterface * ext);

static void checkInitParams(pcmExternInterface * ext, uint8_t thresh, uint8_t panicThresh);

static int32_t getSyncDelay();

static void initRXTXControlRegisters(pcmExternInterface * ext, bool packedMode);

static void initDMAMode(uint8_t dataWidth, uint8_t thresh, uint8_t panicThresh, bool packedMode);

void initPCM(pcmExternInterface * ext, uint8_t thresh, uint8_t panicThresh, bool packedMode);

void startPCM();

void freePCM();

#endif
