#include <stdio.h>
#include "bcm/pimem.h"
#include "bcm/gpio.h"
#include "bcm/clk.h"
#include "pmod.h"
#include "globals.h"

int main() {
	initGPIO();
	initPMOD(0, 0, 0);
	return 0;
}
