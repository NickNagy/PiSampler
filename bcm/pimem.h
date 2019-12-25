#ifndef PIMEM_H
#define PIMEM_H

#include <stdio.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <bcm_host.h>

unsigned * initMemMap(unsigned offset, unsigned size);
void clearMemMap(unsigned * map, unsigned size);

#endif