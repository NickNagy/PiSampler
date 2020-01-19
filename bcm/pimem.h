#ifndef PIMEM_H
#define PIMEM_H

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <bcm_host.h>
#include "../globals.h"

#define BCM_PAGESIZE 4096

#define BUS_BASE 0x7E000000

#define L2_COHERENT_BASE 0x40000000 // seems to disagree w/ Mailbox interface??
#define DIRECT_UNCACHED_BASE 0xC0000000

#define PAGEMAP_LENGTH 8
#define PAGE_SHIFT 12 // <-- need to determine by checking kernel source!

// bit-wise AND with 64b page info to get bits 0-54
#define PAGE_INFO_MASK 0x7FFFFFFFFFFFFFFF

#define MAILBOX_MALLOC_TAG      0x0003000C // 3 ints
#define MAILBOX_MLOCK_TAG       0x0003000D
#define MAILBOX_MUNLOCK_TAG     0x0003000E
#define MAILBOX_FREE_TAG        0x0003000F

// for mem bypasses
#define MAILBOX_MEM_FLAG_DIRECT   0x4 // 0xC alias uncached
#define MAILBOX_MEM_FLAG_COHERENT 0x8 // 0x8 alias (L2 - coherent) * datasheet describes this as L2-only

// structs from: https://github.com/Wallacoloo/Raspberry-Pi-DMA-Example/issues/3
template<int payloadSize> 
struct MailboxMessage {
	MailboxMessage(uint32_t messageId): sizeInBytes(sizeof(*this)), requestCode(0), messageId(messageId), messageSizeInBytes(payloadSize << 2), dataSizeInBytes(payloadSize << 2), messageEnd(0) {}
    uint32_t sizeInBytes;
    uint32_t requestCode;
    uint32_t messageId;
    uint32_t messageSizeInBytes;
    uint32_t dataSizeInBytes;
    union {
        uint32_t payload[payloadSize];
        uint32_t result;
    };
    uint32_t messageEnd;
};

typedef struct VirtToPhysPages {
    void * virtAddr;
    uintptr_t busAddr;
    uint32_t size;
    uint32_t allocationHandle;
} VirtToPhysPages;

static bool openFiles();

void closeFiles();

uint32_t getBCMBase();

uint32_t ceilToPage(uint32_t size);

volatile uint32_t * initMemMap(uint32_t offset, uint32_t size);

void clearMemMap(void * map, uint32_t size);

/* pageInfo interface */

void * initLockedMem(uint32_t size);

void clearLockedMem(void * mem, uint32_t size);

void initVirtPhysPage(void ** virtAddr, void ** physAddr);

void clearVirtPhysPage(void * virtAddr);

uintptr_t virtToPhys(void * virtAddr);

uintptr_t virtToUncachedPhys(void * virtAddr, bool useDirectUncached);

//void * initUncachedMemView(void * virtAddr, uint32_t size, bool useDirectUncached);

//void clearUncachedMemView(void * mem, uint32_t size);

/* Mailbox interface */

uintptr_t busToPhys(void * busAddr, bool useDirectUncached);

static void mailboxWrite(void * message);

//static uint32_t sendMailboxMessage(uint32_t messageId, uint32_t * payload, uint32_t payloadSize);

static uint32_t sendMailboxMessage(uint32_t messageId, uint32_t payload);

static uint32_t sendMailboxMessage(uint32_t messageId, uint32_t payload0, uint32_t payload1, uint32_t payload2);

VirtToPhysPages initUncachedMemView(uint32_t size, bool useDirectUncached);

void clearUncachedMemView(VirtToPhysPages * mem);

#endif
