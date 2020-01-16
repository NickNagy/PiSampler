#ifndef PIMEM_H
#define PIMEM_H

#include <fcntl.h>
#include <sys/mman.h>
#include <bcm_host.h>
#include "../globals.h"

#define BCM_PAGESIZE 4096

#define BUS_BASE 0x7E000000

#define L2_COHERENT_BASE 0x40000000
#define DIRECT_UNCACHED_BASE 0xC0000000

#define PAGEMAP_LENGTH 8
#define PAGE_SHIFT 12 // <-- need to determine by checking kernel source!

// bit-wise AND with 64b page info to get bits 0-54
#define PAGE_INFO_MASK 0x7FFFFFFFFFFFFFFF

static bool openFiles();

uint32_t getBCMBase();

uint32_t ceilToPage(uint32_t size);

volatile uint32_t * initMemMap(uint32_t offset, uint32_t size);

void clearMemMap(void * map, uint32_t size);

void * initLockedMem(uint32_t size);

void clearLockedMem(void * mem, uint32_t size);

void initVirtPhysPage(void ** virtAddr, void ** physAddr);

void clearVirtPhysPage(void * virtAddr);

uintptr_t virtToPhys(void * virtAddr);

uintptr_t virtToUncachedPhys(void * virtAddr, bool useDirectUncached);

void * initUncachedMemView(void * virtAddr, uint32_t size, bool useDirectUncached);

void clearUncachedMemView(void * mem, uint32_t size);

#endif
