#ifndef PCM_H
#define PCM_H

#include "pimem.h"
#include "gpio.h"
#include "dma.h"
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

typedef struct pcmExternInterface {
    char ch1Pos;
    char dataWidth;
    char frameLength;
    char numChannels;
    bool inputOnFallingEdge;
    bool isMasterDevice;
} pcmExternInterface;

static bool checkFrameAndChannelWidth(pcmExternInterface * ext);

static bool checkInitParams(pcmExternInterface * ext, unsigned char thresh);

static int getSyncDelay();

static void initRXTXControlRegisters(pcmExternInterface * ext, bool packedMode);

static void initDMAMode(char dataWidth, unsigned char thresh, bool packedMode);

void initPCM(pcmExternInterface * ext, unsigned char thresh, bool packedMode);

void startPCM();

#endif
