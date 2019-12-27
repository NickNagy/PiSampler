#ifndef PCM_H
#define PCM_H

#include "pimem.h"
#include "../globals.h"
#include <sys/time.h>

#define PCM_BASE_OFFSET 0x3000
#define PCM_BASE_MAPSIZE 9//0x24

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

// bitwise-AND with registers --> avoids changing RO bits
#define CLEAR_CTRL_BITS 0x7E6000

// bitwise-OR with CTRL register
#define RXONTXON 6
// bitwise-AND with CTRL register
#define RXOFFTXOFF 0xFFFFFFF9
#define RXOFFTXON  0xFFFFFFFD//4
#define RXONTXOFF  0xFFFFFFFB//2

static bool checkFrameAndChannelWidth(char frameLength, char dataWidth, char ch1Pos);

static bool checkInitParams(char mode, bool clockMode, char numChannels, char frameLength, char dataWidth, unsigned char thresh);

static int getSyncDelay();

static void initRXTXControlRegisters(bool clockMode, char numChannels, char dataWidth, char ch1Pos);

void initPCM(char mode, bool clockMode, bool fallingEdgeInput, char numChannels, char frameLength, char dataWidth, char ch1Pos, unsigned char thresh);

void startPCM();

#endif
