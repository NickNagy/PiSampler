#ifndef PIMEM_H
#define PIMEM_H

#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <bcm_host.h>
#include "../globals.h"

#define BCM_PAGESIZE 4096

unsigned * initMemMap(unsigned offset, unsigned size);
void clearMemMap(unsigned * map, unsigned size);

#endif
