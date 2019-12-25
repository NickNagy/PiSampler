#ifndef PCM_H
#define PCM_H

#include "pimem.h"
#include <sys/time.h>

#define PCM_BASE_OFFSET 0x3000
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

#define TXCLR (1 << 3)
#define RXCLR (1 << 4)

static void checkFrameAndChannelWidth(char frameLength, char dataWidth);

static void initPolledMode();

static void initInterruptMode();

static void initDMAMode();

static int getSyncDelay();

void initPCM();

void startPCM();

#endif