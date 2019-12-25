#ifndef PMOD_H
#define PMOD_H

#include "gpio.h"
#include "clk.h"
#include "pcm.h"

void initPMOD(bool mode, char pcmMode, char dataWidth);

#endif