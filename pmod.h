#ifndef PMOD_H
#define PMOD_H

#include "globals.h"
#include "bcm/gpio.h"
#include "bcm/clk.h"
#include "bcm/pcm.h"

#define PMOD_CHANNELS 2

static void determineDataWidth();

static char determineFrameLength();

char getDataWidth();

void initPMOD(bool mode, char pcmMode, char dataWidth);

#endif
