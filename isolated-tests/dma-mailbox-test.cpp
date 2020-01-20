#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <bcm_host.h>
#include <errno.h>

#define PAGE_SIZE 4096

// Message IDs for different mailbox GPU memory allocation messages
#define MEM_ALLOC_MESSAGE 0x3000c // This message is 3 u32s: numBytes, alignment and flags
#define MEM_FREE_MESSAGE 0x3000f // This message is 1 u32: handle
#define MEM_LOCK_MESSAGE 0x3000d // 1 u32: handle
#define MEM_UNLOCK_MESSAGE 0x3000e // 1 u32: handle

// Memory allocation flags
#define MEM_ALLOC_FLAG_DIRECT (1 << 2) // Allocate uncached memory that bypasses L1 and L2 cache on loads and stores

static uint32_t bcm_base = 0;
static uint32_t memfd = 0;
static uint32_t pagemapfd = 0;
static uint32_t vciofd = 0;

static volatile uint32_t *dmaMap = 0;

struct GpuMemory
{
  uint32_t allocationHandle;
  void *virtualAddr;
  uintptr_t busAddress;
  uint32_t sizeBytes;
};

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

/* 
initializes memfd and pagemapfd, which are used by multiple functions in this file
returns 0 if successful, 1 otherwise
*/
static bool openFiles() {
    if (!memfd) {
        if((memfd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
            ERROR_MSG("Failure to access /dev/mem."); 
            return 1;
        }
    }
    if (!pagemapfd) {
        if((pagemapfd = open("/proc/self/pagemap", 'r')) < 0) {
            ERROR_MSG("Failure to access /proc/self/pagemap.");
            return 1;
        }
    }
    if (!vciofd) {
        if ((vciofd = open("/dev/vcio", 0)) < 0) {
            ERROR_MSG("Failure to access /dev/vcio.");
            return 1;
        }
    }
    return 0;
}

void closeFiles() {
    if (memfd)
        close(memfd);
    if (pagemapfd)
        close(pagemapfd);
    if (vciofd)
        close(vciofd);
}

/* returns a map to memory of given size and @ address offset */
volatile uint32_t * initMemMap(uint32_t offset, uint32_t size) {
    if (offset % PAGE_SIZE) {
        printf("ERROR: the address offset must be a multiple of the page size, which is %d bytes.\n", PAGE_SIZE);
        return 0;
    }
    if (!memfd) {
        if (openFiles()) return 0;
    }
    if (!bcm_base) 
        bcm_base = bcm_host_get_peripheral_address();
    return (volatile uint32_t *)mmap(0, size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED|MAP_LOCKED, memfd, bcm_base + offset);
}

void clearMemMap(void * map, uint32_t size) {
    munmap(map, size);
}

// Sends a pointer to the given buffer over to the VideoCore mailbox. See https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
void SendMailbox(void *buffer)
{
  int vcio = open("/dev/vcio", 0);
  if (vcio < 0) FATAL_ERROR("Failed to open VideoCore kernel mailbox!");
  int ret = ioctl(vcio, _IOWR(/*MAJOR_NUM=*/100, 0, char *), buffer);
  close(vcio);
  if (ret < 0) FATAL_ERROR("SendMailbox failed in ioctl!");
}

/* rounds size to nearest multiple of BCM_PAGESIZE and returns */
uint32_t ceilToPage(uint32_t size) {
    return (size & (PAGE_SIZE - 1)) ? size + PAGE_SIZE - (size & (PAGE_SIZE-1)): size;
}


// Sends a mailbox message with 1xuint32 payload
uint32_t Mailbox(uint32_t messageId, uint32_t payload0)
{
  MailboxMessage<1> msg(messageId);
  msg.payload[0] = payload0;
  SendMailbox(&msg);
  return msg.result;
}

// Sends a mailbox message with 3xuint32 payload
uint32_t Mailbox(uint32_t messageId, uint32_t payload0, uint32_t payload1, uint32_t payload2)
{
  MailboxMessage<3> msg(messageId);
  msg.payload[0] = payload0;
  msg.payload[1] = payload1;
  msg.payload[2] = payload2;
  SendMailbox(&msg);
  return msg.result;
}

#define BUS_TO_PHYS(x) ((x) & ~0xC0000000)

// Allocates the given number of bytes in GPU side memory, and returns the virtual address and physical bus address of the allocated memory block.
// The virtual address holds an uncached view to the allocated memory, so writes and reads to that memory address bypass the L1 and L2 caches. Use
// this kind of memory to pass data blocks over to the DMA controller to process.
GpuMemory AllocateUncachedGpuMemory(uint32_t numBytes)
{
  GpuMemory mem;
  mem.sizeBytes = ceilToPage(numBytes);
  mem.allocationHandle = Mailbox(MEM_ALLOC_MESSAGE, /*size=*/mem.sizeBytes, /*alignment=*/PAGE_SIZE, /*flags=*/MEM_ALLOC_FLAG_DIRECT);
  mem.busAddress = Mailbox(MEM_LOCK_MESSAGE, mem.allocationHandle);
  mem.virtualAddr = mmap(0, mem.sizeBytes, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, BUS_TO_PHYS(mem.busAddress));
  if (mem.virtualAddr == MAP_FAILED) FATAL_ERROR("Failed to mmap GPU memory!");
  return mem;
}

void FreeUncachedGpuMemory(GpuMemory mem)
{
  munmap(mem.virtualAddr, mem.sizeBytes);
  Mailbox(MEM_UNLOCK_MESSAGE, mem.allocationHandle);
  Mailbox(MEM_FREE_MESSAGE, mem.allocationHandle);
}

void dma_test_0() {

}

void dma_test_1() {

}

int main() {

}