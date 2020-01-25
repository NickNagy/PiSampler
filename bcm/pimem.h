#ifndef PIMEM_H
#define PIMEM_H

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <bcm_host.h>
#include <errno.h>
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
typedef struct VirtToBusPages {
    uint32_t allocationHandle;
    void * virtAddr;
    uintptr_t busAddr;
    uint32_t size;
} VirtToBusPages;

// Defines the structure of a Mailbox message
template<int PayloadSize>
struct MailboxMessage
{
  MailboxMessage(uint32_t messageId):messageSize(sizeof(*this)), requestCode(0), messageId(messageId), messageSizeBytes(sizeof(uint32_t)*PayloadSize), dataSizeBytes(sizeof(uint32_t)*PayloadSize), messageEndSentinel(0) {}
  uint32_t messageSize;
  uint32_t requestCode;
  uint32_t messageId;
  uint32_t messageSizeBytes;
  uint32_t dataSizeBytes;
  union
  {
    uint32_t payload[PayloadSize];
    uint32_t result;
  };
  uint32_t messageEndSentinel;
};

bool openFiles();

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

uintptr_t virtToUncachedBus(void * virtAddr, bool useDirectUncached);

/* Mailbox interface (source: https://github.com/Wallacoloo/Raspberry-Pi-DMA-Example/issues/3)*/

uintptr_t busToPhys(void * busAddr, bool useDirectUncached);

static void mailboxWrite(void * message);

static uint32_t sendMailboxMessage(uint32_t messageId, uint32_t payload);

static uint32_t sendMailboxMessages(uint32_t messageId, uint32_t payload0, uint32_t payload1, uint32_t payload2);

VirtToBusPages initUncachedMemView(uint32_t size, bool useDirectUncached);

void clearUncachedMemView(VirtToBusPages * mem);

#endif
