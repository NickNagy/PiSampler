#ifndef PIMEM_H
#define PIMEM_H

#include <fcntl.h>
#include <sys/mman.h>
#include <bcm_host.h>
#include "../globals.h"

#define BCM_PAGESIZE 4096

#define L2_COHERENT_BASE 0x40000000
#define DIRECT_UNCACHED_BASE 0xC0000000

#define PAGEMAP_LENGTH 8
#define PAGE_SHIFT 12 // <-- need to determine by checking kernel source!

// bit-wise AND with 64b page info to get bits 0-54
#define PAGE_INFO_MASK 0x7FFFFFFFFFFFFFFF

unsigned getBCMBase();
  
unsigned ceilToPage(unsigned size);

void * initMemMap(unsigned offset, unsigned size);

void clearMemMap(void * map, unsigned size);

void * initLockedMem(unsigned size);

void clearLockedMem(void * mem, unsigned size);

void * initVirtPhysPage(void ** virtAddr, void ** physAddr);

void * virtToPhys(void * virtAddr);

void * virtToUncachedPhys(void * virtAddr, bool useDirectUncached);

void * initUncachedMemView(void * virtAddr, unsigned size);

void clearUncachedMemView(void * mem, unsigned size);

#endif
