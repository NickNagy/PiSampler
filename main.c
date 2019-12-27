#include <stdio.h>
#include <pthread.h>
#include "pmod.h"
#include "bcm/gpio.h"
#include "bcm/clk.h"
#include "bcm/pcm.h"

void * runPMODInterface(void * rM) {
    char runMode = *((char *)rM);
    
}

int main() {
    pthread_t PMODThread;
    initPMOD();
}